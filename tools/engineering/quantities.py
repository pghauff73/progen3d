"""Units, quantity kinds, and arithmetic for opt-in engineering studies."""
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
import csv
import math
import re


class StudyError(ValueError):
    """A diagnostic that can be presented to a study author."""


AXES = ("mass", "length", "time", "current", "temperature", "substance", "luminous_intensity")
ONE = (0,) * len(AXES)
TIME = (0, 0, 1, 0, 0, 0, 0)
TEMPERATURE = (0, 0, 0, 0, 1, 0, 0)
ABSOLUTE = "ThermodynamicTemperature"
INTERVAL = "TemperatureDifference"
ANGLE = "PlaneAngle"


def combine(left, right, sign=1):
    return tuple(a + sign * b for a, b in zip(left, right))


@dataclass(frozen=True)
class QuantityKind:
    identifier: str
    label: str
    dimensions: tuple | None
    lifecycle: str
    source: str


@dataclass(frozen=True)
class Unit:
    dimensions: tuple = ONE
    scale: float = 1.0
    offset: float = 0.0
    kind: str = ""

    def to_si(self, value):
        return value * self.scale + self.offset

    def from_si(self, value):
        return (value - self.offset) / self.scale


@dataclass(frozen=True)
class Quantity:
    value: float
    dimensions: tuple = ONE
    kind: str = ""

    def __post_init__(self):
        if not math.isfinite(self.value):
            raise StudyError("nonfinite quantity")

    def add(self, other, subtract=False):
        if self.dimensions != other.dimensions:
            raise StudyError("addition requires equal dimensions")
        if self.kind == ABSOLUTE and other.kind == ABSOLUTE:
            if not subtract:
                raise StudyError("cannot add absolute temperatures")
            return Quantity(self.value - other.value, TEMPERATURE, INTERVAL)
        if self.kind == ABSOLUTE and other.kind == INTERVAL:
            return Quantity(self.value + (-1 if subtract else 1) * other.value, TEMPERATURE, ABSOLUTE)
        if self.kind == INTERVAL and other.kind == ABSOLUTE and not subtract:
            return other.add(self)
        if self.kind and other.kind and self.kind != other.kind:
            raise StudyError(f"incompatible quantity kinds {self.kind} and {other.kind}")
        if ABSOLUTE in (self.kind, other.kind) and self.kind != other.kind:
            raise StudyError("temperature arithmetic requires an explicit interval")
        return Quantity(self.value + (-1 if subtract else 1) * other.value,
                        self.dimensions, self.kind or other.kind)

    def multiply(self, other, divide=False):
        if ABSOLUTE in (self.kind, other.kind):
            raise StudyError("arithmetic on an absolute temperature requires an explicit law")
        if divide and other.value == 0:
            raise StudyError("division by zero")
        kind = self.kind if other.dimensions == ONE and not other.kind else ""
        if not divide and self.dimensions == ONE and not self.kind:
            kind = other.kind
        return Quantity(self.value / other.value if divide else self.value * other.value,
                        combine(self.dimensions, other.dimensions, -1 if divide else 1), kind)

    def power(self, exponent):
        if self.kind == ABSOLUTE or exponent.kind or exponent.dimensions != ONE:
            raise StudyError("invalid quantity exponent")
        ratio = Fraction(exponent.value).limit_denominator(1000)
        if float(ratio) != exponent.value or abs(ratio) > 32:
            raise StudyError("exponent must be a rational value of magnitude at most 32")
        if self.value < 0 and ratio.denominator != 1:
            raise StudyError("fractional power of a negative scalar is unsupported")
        if self.value == 0 and ratio < 0:
            raise StudyError("negative power of zero")
        return Quantity(self.value ** float(ratio), tuple(a * ratio for a in self.dimensions))


class QuantityRegistry:
    def __init__(self, extensions=()):
        catalogue = Path(__file__).with_name("data") / "qudt-3.5.3.csv"
        with catalogue.open(encoding="utf-8") as stream:
            self.kinds = {}
            for row in csv.DictReader(stream):
                dimensions = tuple(Fraction(row[a]) for a in AXES) if row[AXES[0]] else None
                kind = QuantityKind(row["identifier"], row["label"], dimensions,
                                    row["lifecycle"], row["source_URI"])
                if kind.identifier in self.kinds:
                    raise StudyError(f"duplicate kind {kind.identifier}")
                self.kinds[kind.identifier] = kind
        self.aliases = {
            "qudt:" + key: key for key in self.kinds
        } | {kind.source: key for key, kind in self.kinds.items()}
        for kind in extensions:
            if ":" not in kind.identifier or kind.identifier in self.kinds or kind.identifier in self.aliases:
                raise StudyError(f"custom quantity needs a unique namespaced ID: {kind.identifier}")
            self.kinds[kind.identifier] = kind

    def kind(self, identifier):
        key = self.aliases.get(identifier, identifier)
        if key not in self.kinds:
            raise StudyError(f"unknown quantity kind {identifier}")
        return self.kinds[key]

    def assign(self, value, identifier):
        kind = self.kind(identifier)
        target = kind.identifier
        if kind.dimensions is None or value.dimensions != kind.dimensions:
            raise StudyError(f"dimension mismatch for {identifier}")
        if target in ("Temperature", ABSOLUTE):
            target = ABSOLUTE
        if value.kind and value.kind != target:
            raise StudyError(f"cannot assign {value.kind} to {target}")
        if target == ABSOLUTE and value.value < 0:
            raise StudyError("absolute temperature below 0 K")
        return Quantity(value.value, value.dimensions, target)


UNIT = {
    "1": Unit(), "%": Unit(scale=.01), "kg": Unit((1, 0, 0, 0, 0, 0, 0)),
    "m": Unit((0, 1, 0, 0, 0, 0, 0)), "s": Unit(TIME),
    "A": Unit((0, 0, 0, 1, 0, 0, 0)), "K": Unit(TEMPERATURE),
    "mol": Unit((0, 0, 0, 0, 0, 1, 0)), "cd": Unit((0, 0, 0, 0, 0, 0, 1)),
    "degC": Unit(TEMPERATURE, 1, 273.15, ABSOLUTE),
    "delta_degC": Unit(TEMPERATURE, kind=INTERVAL),
    "degF": Unit(TEMPERATURE, 5/9, 273.15 - 32*5/9, ABSOLUTE),
    "delta_degF": Unit(TEMPERATURE, 5/9, kind=INTERVAL),
    "sr": Unit(kind="SolidAngle"),
    "rad": Unit(kind=ANGLE), "deg": Unit(scale=math.pi / 180, kind=ANGLE),
    "N": Unit((1, 1, -2, 0, 0, 0, 0)), "Pa": Unit((1, -1, -2, 0, 0, 0, 0)),
    "J": Unit((1, 2, -2, 0, 0, 0, 0)), "W": Unit((1, 2, -3, 0, 0, 0, 0)),
    "V": Unit((1, 2, -3, -1, 0, 0, 0)), "C": Unit((0, 0, 1, 1, 0, 0, 0)),
    "ohm": Unit((1, 2, -3, -2, 0, 0, 0)), "F": Unit((-1, -2, 4, 2, 0, 0, 0)),
    "Hz": Unit((0, 0, -1, 0, 0, 0, 0)), "Bq": Unit((0, 0, -1, 0, 0, 0, 0)),
    "Gy": Unit((0, 2, -2, 0, 0, 0, 0)), "Sv": Unit((0, 2, -2, 0, 0, 0, 0)),
    "S": Unit((-1, -2, 3, 2, 0, 0, 0)), "Wb": Unit((1, 2, -2, -1, 0, 0, 0)),
    "T": Unit((1, 0, -2, -1, 0, 0, 0)), "H": Unit((1, 2, -2, -2, 0, 0, 0)),
    "lm": Unit((0, 0, 0, 0, 0, 0, 1)), "lx": Unit((0, -2, 0, 0, 0, 0, 1)),
    "kat": Unit((0, 0, -1, 0, 0, 1, 0)),
    "min": Unit(TIME, 60), "h": Unit(TIME, 3600),
}
UNIT["g"] = Unit(UNIT["kg"].dimensions, .001)
PREFIXES = {"G": 1e9, "M": 1e6, "k": 1e3, "c": .01, "m": .001, "u": 1e-6, "n": 1e-9}
PREFIXABLE = {"m", "s", "g", "A", "Pa", "J", "W", "N", "V", "F", "ohm", "Hz", "mol"}
FACTOR = re.compile(r"(1|[A-Za-z_%]+)(?:\^(\(-?[0-9]+/[0-9]+\)|-?[0-9]+))?")


def unit_atom(symbol):
    if symbol == "dB":
        raise StudyError("dB requires a reference and amplitude or power convention")
    if symbol in UNIT:
        return UNIT[symbol]
    if symbol[:1] in PREFIXES and symbol[1:] in PREFIXABLE:
        base = UNIT[symbol[1:]]
        return Unit(base.dimensions, PREFIXES[symbol[0]] * base.scale)
    raise StudyError(f"unknown executable unit {symbol}")


def parse_unit(expression):
    text = expression.strip().replace("µ", "u").replace("Ω", "ohm").replace("°C", "degC").replace("°F", "degF").replace("·", "*")
    if text in UNIT or text == "dB":
        return unit_atom(text)
    dimensions = ONE
    scale = 1.0
    position = 0
    sign = 1
    while position < len(text):
        match = FACTOR.match(text, position)
        if not match:
            raise StudyError(f"invalid unit expression {expression}")
        unit = unit_atom(match[1])
        if unit.offset or unit.kind in (ABSOLUTE, INTERVAL):
            raise StudyError("offset and explicit interval units must stand alone")
        try:
            power = sign * Fraction((match[2] or "1").strip("()"))
            if abs(power) > 32:
                raise StudyError("unit exponent magnitude exceeds 32")
            scale *= unit.scale ** float(power)
        except (ZeroDivisionError, OverflowError) as exc:
            raise StudyError(f"invalid scale or exponent in {expression}") from exc
        dimensions = combine(dimensions, tuple(a * power for a in unit.dimensions))
        position = match.end()
        if position == len(text):
            return Unit(dimensions, scale)
        delimiter = re.match(r"\s*([*/])\s*|\s+(?=[A-Za-z_%1])", text[position:])
        if not delimiter or position + delimiter.end() == len(text):
            raise StudyError(f"invalid unit expression {expression}")
        sign = -1 if delimiter[1] == "/" else 1
        position += delimiter.end()
    raise StudyError("missing unit")


def literal(number, unit_text):
    unit = parse_unit(unit_text)
    kind = ABSOLUTE if unit_text.strip() == "K" else unit.kind
    return Quantity(unit.to_si(number), unit.dimensions, kind)


def sound_level_ratio(level, reference, convention):
    if reference.value <= 0 or convention not in ("amplitude", "power"):
        raise StudyError("logarithmic level requires a positive reference and explicit convention")
    return Quantity(reference.value * 10 ** (level / (20 if convention == "amplitude" else 10)),
                    reference.dimensions, reference.kind)
