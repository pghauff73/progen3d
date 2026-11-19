"""Ordered surface coatings with dimension-checked material data."""
import math

from .domain_inputs import nonnegative, scalar
from .quantities import INTERVAL, QuantityRegistry, StudyError, literal, parse_unit


MATERIAL_PROPERTY_UNITS = {
    "MassDensity": "kg/m^3",
    "ThermalConductivity": "W/m/K",
    "SpecificHeatCapacity": "J/kg/K",
    "ThermalCoefficientOfLinearExpansion": "1/K",
    "ModulusOfElasticity": "Pa",
}


def measurement(data, registry, kind, si_unit, label):
    if not isinstance(data, dict) or "value" not in data or not isinstance(data.get("unit"), str):
        raise StudyError(f"{label} requires a value and unit")
    source = data.get("source")
    if not isinstance(source, str) or not source.strip():
        raise StudyError(f"{label} requires a source")
    value = scalar(data["value"], label)
    unit = data["unit"].strip()
    # A kelvin interval uses the same scale as a Celsius interval without the absolute origin.
    quantity = literal(value, "delta_degC" if kind == INTERVAL and unit == "K" else unit)
    checked = registry.assign(quantity, kind)
    result = {
        "kind": kind, "value": value, "unit": unit, "source": source.strip(),
        "si_value": checked.value, "si_unit": si_unit,
    }
    if "uncertainty" in data:
        uncertainty = nonnegative(data, "uncertainty")
        result["uncertainty"] = uncertainty
        result["si_uncertainty"] = scalar(uncertainty * parse_unit(unit).scale, f"{label} uncertainty")
    if "temperature_range" in data:
        limits = data["temperature_range"]
        low = scalar(limits["min"], "temperature range minimum")
        high = scalar(limits["max"], "temperature range maximum")
        range_unit = limits["unit"]
        if not isinstance(range_unit, str):
            raise StudyError(f"{label} temperature range requires a unit")
        low_k = registry.assign(literal(low, range_unit), "ThermodynamicTemperature").value
        high_k = registry.assign(literal(high, range_unit), "ThermodynamicTemperature").value
        if low_k > high_k:
            raise StudyError(f"{label} temperature range minimum exceeds maximum")
        result["temperature_range"] = {"min": low, "max": high, "unit": range_unit,
                                       "min_K": low_k, "max_K": high_k}
    return result


def material_properties(materials, name, registry):
    if name not in materials:
        raise StudyError(f"unknown coating material {name}")
    properties = materials[name]["properties"]
    if not isinstance(properties, dict):
        raise StudyError(f"{name} material properties must be keyed by quantity kind")
    checked = {}
    for kind, value in properties.items():
        if kind not in MATERIAL_PROPERTY_UNITS:
            raise StudyError(f"unsupported physical coating property {kind}")
        record = measurement(value, registry, kind, MATERIAL_PROPERTY_UNITS[kind], f"{name} {kind}")
        if kind != "ThermalCoefficientOfLinearExpansion" and record["si_value"] <= 0:
            raise StudyError(f"{name} {kind} must be positive")
        checked[kind] = record
    return checked


def coating_stack(spec, materials, registry):
    if not isinstance(spec, dict) or not isinstance(materials, dict):
        raise StudyError("coating requires a stack and a named material catalogue")
    layers = spec.get("layers")
    if not isinstance(layers, list) or not layers:
        raise StudyError("coating requires an ordered list of layers")
    area = None
    if "area" in spec:
        area = measurement(spec["area"], registry, "Area", "m^2", "coating area")
        if area["si_value"] <= 0:
            raise StudyError("coating area must be positive")
    fit_thicknesses, resistances, records, checked_materials = [], [], [], {}
    for layer in layers:
        name = layer["material"]
        if not isinstance(name, str) or not name.strip():
            raise StudyError("coating layer requires a material name")
        if name not in checked_materials:
            checked_materials[name] = material_properties(materials, name, registry)
        properties = checked_materials[name]
        thickness = measurement(layer["thickness"], registry, "Thickness", "m", "coating thickness")
        if thickness["si_value"] <= 0:
            raise StudyError("coating thickness must be positive")
        fit, thermal = layer["affects_fit"], layer["affects_thermal"]
        if type(fit) is not bool or type(thermal) is not bool:
            raise StudyError("coating contribution flags must be true or false")
        record = {"material": name, "thickness": thickness, "affects_fit": fit,
                  "affects_thermal": thermal}
        if fit:
            fit_thicknesses.append(thickness["si_value"])
        if thermal:
            conductivity = properties.get("ThermalConductivity")
            if conductivity is None:
                raise StudyError(f"{name} needs ThermalConductivity for an active thermal layer")
            if area is None:
                raise StudyError("active thermal coating layers require a coating area")
            resistance = thickness["si_value"] / conductivity["si_value"]
            if "interface_resistance" in layer:
                interface = measurement(layer["interface_resistance"], registry, "ThermalInsulance",
                                        "m^2*K/W", "coating interface resistance")
                if interface["si_value"] < 0:
                    raise StudyError("coating interface resistance must be nonnegative")
                record["interface_resistance"] = interface
                resistance += interface["si_value"]
            record["resistance_K_W"] = scalar(resistance / area["si_value"], "coating resistance")
            resistances.append(record["resistance_K_W"])
        elif "interface_resistance" in layer:
            raise StudyError("coating interface resistance requires an active thermal layer")
        records.append(record)
    result = {"layers": records, "materials": checked_materials,
              "fit_thickness_m": math.fsum(fit_thicknesses)}
    if area is not None:
        result["area"] = area
    if resistances:
        result["thermal_resistance_K_W"] = math.fsum(resistances)
    if "temperature_difference" in spec:
        if not resistances or result["thermal_resistance_K_W"] <= 0:
            raise StudyError("coating heat flow requires a positive thermal resistance")
        delta = measurement(spec["temperature_difference"], registry, INTERVAL, "K",
                            "coating temperature difference")
        result["temperature_difference"] = delta
        result["heat_flow_W"] = scalar(delta["si_value"] / result["thermal_resistance_K_W"], "heat flow")
    return result


def fit_coatings(data):
    parts = {name: data[name]["coating"] for name in ("shaft", "bore") if "coating" in data[name]}
    if not parts:
        return {}
    registry = QuantityRegistry()
    return {name: coating_stack(spec, data.get("materials"), registry) for name, spec in parts.items()}
