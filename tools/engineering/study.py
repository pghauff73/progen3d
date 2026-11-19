"""Parser and checked scalar model for the version-one engineering sidecar."""
from dataclasses import dataclass, field
from hashlib import sha256
from pathlib import Path
import ast
import json
import math
import re

from .quantities import (
    ABSOLUTE, INTERVAL, ONE, TIME, Quantity, QuantityKind, QuantityRegistry,
    StudyError, combine, literal, parse_unit,
)

IDENT = r"[A-Za-z_]\w*"
KIND = r"[A-Za-z_][\w.:/-]*"
NUMBER = r"(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
LITERAL = re.compile(rf"(?<![\w.])({NUMBER})\s*\[([^\[\]\n]+)\]")


@dataclass(frozen=True)
class Expression:
    tree: ast.AST
    line: int

    @classmethod
    def parse(cls, text, line):
        parts, previous = [], 0
        for match in LITERAL.finditer(text):
            parts.append(text[previous:match.start()].replace("^", "**"))
            parts.append(f"quantity({match[1]}, {match[2]!r})")
            previous = match.end()
        parts.append(text[previous:].replace("^", "**"))
        try:
            return cls(ast.parse("".join(parts).strip(), mode="eval").body, line)
        except SyntaxError as exc:
            raise StudyError("invalid expression") from exc

    def evaluate(self, values):
        def visit(node):
            if isinstance(node, ast.Constant) and type(node.value) in (int, float):
                return Quantity(float(node.value))
            if isinstance(node, ast.Name) and node.id in values:
                return values[node.id]
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "quantity":
                if (len(node.args) == 2 and not node.keywords
                        and all(isinstance(arg, ast.Constant) for arg in node.args)
                        and type(node.args[0].value) in (int, float)
                        and isinstance(node.args[1].value, str)):
                    return literal(float(node.args[0].value), node.args[1].value)
            if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
                minus = isinstance(node.op, ast.USub)
                if minus and isinstance(node.operand, ast.Call):
                    visit(node.operand)
                    number, unit = (argument.value for argument in node.operand.args)
                    return literal(-number, unit)
                quantity = visit(node.operand)
                if minus and quantity.kind == ABSOLUTE:
                    raise StudyError("cannot negate an absolute temperature")
                return Quantity((-1 if minus else 1) * quantity.value, quantity.dimensions, quantity.kind)
            if isinstance(node, ast.BinOp):
                a, b = visit(node.left), visit(node.right)
                if isinstance(node.op, ast.Add):
                    return a.add(b)
                if isinstance(node.op, ast.Sub):
                    return a.add(b, subtract=True)
                if isinstance(node.op, ast.Mult):
                    return a.multiply(b)
                if isinstance(node.op, ast.Div):
                    return a.multiply(b, divide=True)
                if isinstance(node.op, ast.Pow):
                    if a.dimensions != ONE and any(isinstance(n, (ast.Name, ast.Call)) for n in ast.walk(node.right)):
                        raise StudyError("dimensional exponent must be a constant")
                    return a.power(b)
            if isinstance(node, ast.Name):
                raise StudyError(f"unknown or not yet declared quantity {node.id}")
            raise StudyError("unsupported expression")
        try:
            return visit(self.tree)
        except (StudyError, ArithmeticError) as exc:
            raise StudyError(f"line {self.line}: {exc}") from exc


@dataclass(frozen=True)
class DerivedQuantity:
    name: str
    kind: str
    expression: Expression


@dataclass(frozen=True)
class DifferentialEquation:
    label: str
    state: str
    coefficient: Expression
    rate: Expression


@dataclass(frozen=True)
class Observation:
    name: str
    interval: float
    unit_text: str
    unit: object


@dataclass(frozen=True)
class Event:
    name: str
    direction: int
    guard: Expression | None
    message: str
    assignment: tuple | None = None
    scheduled_s: float | None = None


@dataclass
class Study:
    name: str
    registry: QuantityRegistry
    bindings: dict = field(default_factory=dict)
    parameters: dict = field(default_factory=dict)
    initials: dict = field(default_factory=dict)
    derived: list = field(default_factory=list)
    equations: dict = field(default_factory=dict)
    observations: list = field(default_factory=list)
    events: list = field(default_factory=list)
    tolerances: dict = field(default_factory=dict)
    start: float = 0.0
    end: float = 0.0
    method: str = ""
    relative_tolerance: float = 1e-8
    geometry_length: float | None = None
    step: float | None = None
    locations: dict = field(default_factory=dict)
    fingerprint: str = ""

    def environment(self, time, state):
        values = dict(self.parameters)
        values["t"] = Quantity(float(time), TIME, "Time")
        for name, value in zip(self.initials, state, strict=True):
            initial = self.initials[name]
            values[name] = self.registry.assign(Quantity(value, initial.dimensions, initial.kind), initial.kind)
        for item in self.derived:
            values[item.name] = self.registry.assign(item.expression.evaluate(values), item.kind)
        return values

    def rates(self, time, state):
        values = self.environment(time, state)
        return [self.equations[name].rate.evaluate(values).value
                / self.equations[name].coefficient.evaluate(values).value for name in self.initials]


def scene_catalog(path):
    try:
        data = json.loads(Path(path).read_text(), parse_constant=lambda _: None)
        revision = data["revision"]
        objects = data["objects"]
        if (not isinstance(revision, str) or not revision or not isinstance(objects, dict)
                or any(not key or not isinstance(value, dict) for key, value in objects.items())):
            raise ValueError("invalid scene catalog")
        return data
    except (OSError, ValueError, KeyError, TypeError) as exc:
        raise StudyError(f"cannot read scene catalog: {exc}") from exc


def require_scene_face(object_id, obj, face):
    count = obj.get("geometry_primitive_count", 0)
    if type(count) is not int or count < 0:
        raise StudyError(f"invalid primitive count on {object_id}")
    if count > 1:
        raise StudyError(f"ambiguous face {face} on {object_id}: object binds multiple primitives")
    faces = obj.get("faces", [])
    if not isinstance(faces, list) or face not in faces:
        raise StudyError(f"missing face {face} on {object_id}")


def require_scene_interface(object_id, obj, interface_id):
    names = obj.get("interfaces")
    if not isinstance(names, list) or interface_id not in names:
        raise StudyError(f"missing interface {interface_id} on {object_id}")
    details = obj.get("interface_details")
    if not isinstance(details, dict) or not isinstance(details.get(interface_id), dict):
        raise StudyError(f"missing interface details for {object_id}:{interface_id}; export a fresh manifest")
    return details[interface_id]


def parse_study(path, catalog=None, extensions=()):
    extensions = tuple(extensions)
    source = Path(path).read_text(encoding="utf-8")
    lines = []
    for n, line in enumerate(source.splitlines(), 1):
        line = line.strip()
        if line and not line.startswith("//"):
            lines.append((n, line))
    if len(lines) < 3 or lines[-1][1] != "}":
        raise StudyError("expected a complete study block")
    header = re.fullmatch(rf"study\s+({IDENT})\s+version\s+1\s*\{{", lines[0][1])
    if not header:
        raise StudyError("expected study <name> version 1")
    study = Study(header[1], QuantityRegistry(extensions))
    registry_source = (Path(__file__).with_name("data") / "qudt-3.5.3.csv").read_bytes()
    study.fingerprint = sha256(registry_source + b"\n" + (source + "\n" + json.dumps(catalog, sort_keys=True)
                                + "\n" + repr(tuple(extensions))).encode()).hexdigest()
    definition_names = {"t"}

    def declare(name):
        if name in definition_names:
            raise StudyError(f"duplicate declaration {name}")
        definition_names.add(name)

    for n, line in lines[1:-1]:
        try:
            if m := re.fullmatch(r"geometry_length\s*=\s*(.+)", line):
                scale = Expression.parse(m[1], n).evaluate({})
                if study.geometry_length is not None or scale.dimensions != parse_unit("m").dimensions or scale.value <= 0:
                    raise StudyError("geometry_length requires one positive length")
                study.geometry_length = scale.value
            elif m := re.fullmatch(
                rf'bind\s+({IDENT})\s*=\s*object\("([^"]+)"\)(?:\s+revision\s+"([^"]+)")?'
                r'(?:\s+(face|interface)\s+"([^"]+)")?', line
            ):
                alias, object_id, expected, selector, reference = m.groups()
                declare(alias)
                if not catalog or object_id not in catalog["objects"]:
                    raise StudyError(f"unresolved scene object {object_id}")
                obj = catalog["objects"][object_id]
                if expected is not None and expected != catalog["revision"]:
                    raise StudyError(f"stale scene revision for {object_id}")
                binding = {"object": object_id, "revision": catalog["revision"], "face": None}
                if selector == "face":
                    require_scene_face(object_id, obj, reference)
                    binding["face"] = reference
                elif selector == "interface":
                    require_scene_interface(object_id, obj, reference)
                    binding["interface"] = reference
                study.bindings[alias] = binding
            elif m := re.fullmatch(rf"(parameter|state|derived)\s+({IDENT})\s*:\s*({KIND})(?:\s+at\s+({IDENT}))?\s*=\s*(.+)", line):
                role, name, kind, binding, text = m.groups()
                declare(name)
                if binding and (role != "state" or binding not in study.bindings):
                    raise StudyError(f"unresolved or unsupported quantity location {binding}")
                expr = Expression.parse(text, n)
                if role == "derived":
                    study.derived.append(DerivedQuantity(name, kind, expr))
                else:
                    value = study.registry.assign(expr.evaluate(study.parameters), kind)
                    (study.parameters if role == "parameter" else study.initials)[name] = value
                if binding:
                    study.locations[name] = binding
            elif m := re.fullmatch(rf"equation\s+({IDENT})\s*:\s*(?:(.+?)\s*\*\s*)?der\(({IDENT})\)\s*=\s*(.+)", line):
                label, coeff, name, rate = m.groups()
                declare(label)
                if name in study.equations:
                    raise StudyError(f"duplicate equation for {name}")
                study.equations[name] = DifferentialEquation(label, name, Expression.parse(coeff or "1", n), Expression.parse(rate, n))
            elif m := re.fullmatch(rf"observe\s+({IDENT})\s+every\s+(.+?)\s+display\s+\[([^\]]+)\]", line):
                interval = study.registry.assign(Expression.parse(m[2], n).evaluate(study.parameters), "Time").value
                if interval <= 0 or any(p.name == m[1] for p in study.observations):
                    raise StudyError("observation requires one positive interval per quantity")
                study.observations.append(Observation(m[1], interval, m[3], parse_unit(m[3])))
            elif m := re.fullmatch(rf"event\s+({IDENT})\s+(?:when\s+(rising|falling|either)\((.+)\)|at\s+(.+?))\s*\{{\s*(.+?)\s*\}}", line):
                name, direction, guard, scheduled, action = m.groups()
                declare(name)
                due = None
                if scheduled:
                    due = study.registry.assign(Expression.parse(scheduled, n).evaluate(study.parameters), "Time").value
                if emit := re.fullmatch(r'emit\("([^"]*)"\)', action):
                    message, assignment = emit[1], None
                elif reset := re.fullmatch(rf"set\s+({IDENT})\s*=\s*(.+)", action):
                    message, assignment = f"set {reset[1]}", (reset[1], Expression.parse(reset[2], n))
                else:
                    raise StudyError("unsupported event action")
                study.events.append(Event(
                    name, {"rising": 1, "falling": -1, "either": 0, None: 0}[direction],
                    Expression.parse(guard, n) if guard else None, message, assignment, due))
            elif m := re.fullmatch(r"integrate\s+from\s+(.+?)\s+to\s+(.+?)\s+method\s+(RK4|RK45|BDF)(?:\s+step\s+(.+))?", line):
                if study.method:
                    raise StudyError("duplicate integration interval")
                study.start, study.end = (study.registry.assign(Expression.parse(m[k], n).evaluate(study.parameters), "Time").value for k in (1, 2))
                study.method = m[3]
                study.step = (study.registry.assign(Expression.parse(m[4], n).evaluate(study.parameters), "Time").value
                              if m[4] else None)
            elif m := re.fullmatch(rf"rtol\s+({NUMBER})\s*(.*)", line):
                study.relative_tolerance = float(m[1])
                for term in re.finditer(rf"atol\(({IDENT})\)\s+({NUMBER})\s*\[([^\]]+)\]\s*|(.+)", m[2]):
                    if term[4] or term[1] in study.tolerances:
                        raise StudyError("invalid or duplicate absolute tolerance")
                    unit = parse_unit(term[3])
                    if unit.offset or float(term[2]) <= 0:
                        raise StudyError("absolute tolerance requires a positive interval")
                    study.tolerances[term[1]] = Quantity(float(term[2]) * unit.scale, unit.dimensions)
            else:
                raise StudyError("unknown study declaration")
        except StudyError as exc:
            detail = str(exc)
            raise StudyError(f"{path}:{detail}" if detail.startswith("line ") else f"{path}:line {n}: {detail}") from exc
    if study.bindings and study.geometry_length is None:
        raise StudyError("bound studies require geometry_length")
    if not study.method or not study.end > study.start:
        raise StudyError("integration requires an increasing interval")
    if study.step is not None and study.step <= 0:
        raise StudyError("integration step must be positive")
    if not 0 < study.relative_tolerance < 1:
        raise StudyError("relative tolerance must be between zero and one")
    if not study.initials or set(study.equations) != set(study.initials):
        raise StudyError("every state needs exactly one differential equation")
    values = study.environment(study.start, [v.value for v in study.initials.values()])
    for name, tol in study.tolerances.items():
        if name not in study.initials or initial_dimension(study, name) != tol.dimensions:
            raise StudyError(f"incompatible absolute tolerance for {name}")
    for equation in study.equations.values():
        coefficient = equation.coefficient.evaluate(study.parameters)
        rate = equation.rate.evaluate(values)
        if coefficient.value == 0 or combine(coefficient.dimensions,
                combine(initial_dimension(study, equation.state), TIME, -1)) != rate.dimensions:
            raise StudyError(f"line {equation.rate.line}: invalid coefficient or dimension mismatch in {equation.label}")
    for probe in study.observations:
        if probe.name not in values or probe.unit.dimensions != values[probe.name].dimensions:
            raise StudyError(f"incompatible display unit for {probe.name}")
        if probe.unit.kind and probe.unit.kind != values[probe.name].kind:
            raise StudyError(f"incompatible display quantity kind for {probe.name}")
    for event in study.events:
        if event.scheduled_s is not None and not study.start < event.scheduled_s <= study.end:
            raise StudyError(f"scheduled event {event.name} must follow the start and lie within the interval")
        if event.assignment:
            target, value = event.assignment
            if target not in study.initials:
                raise StudyError(f"event {event.name} cannot set non-state {target}")
            study.registry.assign(value.evaluate(values), study.initials[target].kind)
        if event.guard is not None and event.guard.evaluate(values).kind == ABSOLUTE:
            raise StudyError(f"event {event.name} requires a difference from a threshold")
    return study


def initial_dimension(study, name):
    return study.initials[name].dimensions


def load_extensions(path):
    if path is None:
        return ()
    try:
        entries = json.loads(Path(path).read_text(encoding="utf-8"))
        from fractions import Fraction
        kinds = []
        for item in entries:
            dimensions = tuple(Fraction(x) for x in item["dimensions"])
            if len(dimensions) != 7:
                raise ValueError("expected seven rational exponents")
            kinds.append(QuantityKind(item["id"], item["label"], dimensions, "custom", item["source"]))
        return tuple(kinds)
    except (OSError, ValueError, TypeError, KeyError, ZeroDivisionError) as exc:
        raise StudyError(f"invalid quantity extension file: {exc}") from exc
