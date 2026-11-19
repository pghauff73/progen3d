"""Finite-volume heat transfer and quasi-static opening of an end-bonded coupon."""
from bisect import bisect_right
from dataclasses import dataclass
import math

from .cohesive import CohesiveLaw
from .domain_inputs import nonnegative, positive, required_source, scalar
from .quantities import StudyError


@dataclass(frozen=True)
class CouponLayer:
    name: str
    length: float
    cell_count: int
    conductivity: float
    heat_capacity: float
    modulus: float
    expansion: float
    initial_temperature: float
    source: str

    @classmethod
    def from_input(cls, layer, reference):
        name = layer["name"]
        cells = layer["cells"]
        if not isinstance(name, str) or not name.strip():
            raise StudyError("each layer needs a name")
        if type(cells) is not int or not 1 <= cells <= 1000:
            raise StudyError("each layer requires 1–1000 thermal cells")
        initial = scalar(layer.get("initial_K", reference), "initial_K")
        if initial < 0:
            raise StudyError("initial temperature cannot be below absolute zero")
        return cls(name, positive(layer, "length_m"), cells,
                   positive(layer, "conductivity_W_m_K"),
                   positive(layer, "volumetric_capacity_J_m3_K"),
                   positive(layer, "young_modulus_Pa"),
                   scalar(layer["alpha_per_K"], "alpha_per_K"),
                   initial, required_source(layer))


@dataclass(frozen=True)
class ProgramPoint:
    time: float
    left: float
    right: float
    extension: float

    @classmethod
    def from_input(cls, entry):
        return cls(nonnegative(entry, "time_s"), nonnegative(entry, "left_K"),
                   nonnegative(entry, "right_K"), scalar(entry["grip_extension_m"], "grip_extension_m"))


@dataclass(frozen=True)
class ThermalSection:
    start: ProgramPoint
    end: ProgramPoint
    solution: object
    rates: object

    @property
    def extension_rate(self):
        return (self.end.extension - self.start.extension) / (self.end.time - self.start.time)

    def extension(self, time):
        return self.start.extension + (time-self.start.time)*self.extension_rate


@dataclass(frozen=True)
class CouponResponse:
    interface_displacement: float
    stress: float
    damage: float
    dissipated_energy: float


class BondedAxis:
    def __init__(self, law, compliance, compression_stiffness):
        self.law = law
        self.compliance = compliance
        self.compression_stiffness = compression_stiffness

    def response(self, drive, maximum_drive):
        maximum_opening = self.law.maximum_opening(maximum_drive, self.compliance)
        damage = self.law.damage(maximum_opening)
        stiffness = self.compression_stiffness if drive < 0 else self.law.stiffness * (1-damage)
        displacement = drive / (1+self.compliance*stiffness)
        return CouponResponse(displacement, stiffness*displacement, damage,
                              self.law.dissipated_energy(maximum_opening))


def solve_thermal(layers, area, resistance, program, max_step):
    import numpy as np
    from scipy.integrate import solve_ivp
    from scipy.sparse import lil_matrix

    widths = np.concatenate([np.full(layer.cell_count, layer.length/layer.cell_count) for layer in layers])
    half_resistances = np.concatenate([
        np.full(layer.cell_count, layer.length/layer.cell_count/(2*layer.conductivity)) for layer in layers
    ])
    capacities = area*widths*np.repeat([layer.heat_capacity for layer in layers],
                                       [layer.cell_count for layer in layers])
    layer_boundary = layers[0].cell_count-1
    interior_resistance = half_resistances[:-1]+half_resistances[1:]
    interior_resistance[layer_boundary] += resistance
    conductances = area/interior_resistance
    left_g, right_g = area/half_resistances[[0, -1]]
    n = len(widths)
    matrix = lil_matrix((n+2, n+2))
    for i, conductance in enumerate(conductances):
        for a, b in ((i, i+1), (i+1, i)):
            matrix[a, a] -= conductance/capacities[a]
            matrix[a, b] += conductance/capacities[a]
    for cell, ledger, conductance in ((0, n, left_g), (n-1, n+1, right_g)):
        matrix[cell, cell] -= conductance/capacities[cell]
        matrix[ledger, cell] = -conductance
    matrix = matrix.tocsc()
    initial = np.repeat([layer.initial_temperature for layer in layers], [layer.cell_count for layer in layers])
    state = np.append(initial, [0.0, 0.0])
    sections = []
    for start, end in zip(program, program[1:]):
        def rates(time, values, first=start, last=end):
            weight = (time-first.time)/(last.time-first.time)
            left = first.left + weight*(last.left-first.left)
            right = first.right + weight*(last.right-first.right)
            result = matrix@values
            result[0] += left_g*left/capacities[0]
            result[n-1] += right_g*right/capacities[-1]
            result[n] += left_g*left
            result[n+1] += right_g*right
            return result

        result = solve_ivp(rates, (start.time, end.time), state, method="BDF", jac=matrix,
                           dense_output=True, rtol=1e-10, atol=1e-9, max_step=max_step)
        if not result.success:
            raise StudyError(f"thermal solve failed after {start.time:g} s: {result.message}")
        state = result.y[:, -1]
        sections.append(ThermalSection(start, end, result, rates))
    return sections, widths, capacities, initial, half_resistances, conductances[layer_boundary]


class CouponHistory:
    def __init__(self, sections, expansion_weights, reference):
        self.sections = sections
        self.ends = [section.end.time for section in sections]
        self.expansion_weights = expansion_weights
        self.reference = reference

    def section(self, time):
        return self.sections[min(bisect_right(self.ends, time), len(self.sections)-1)]

    def extension(self, temperatures):
        return float(self.expansion_weights @ (temperatures[:-2]-self.reference))

    def drive(self, time):
        section = self.section(time)
        return section.extension(time)-self.extension(section.solution.sol(time))

    def breakpoints(self):
        from scipy.optimize import brentq

        # Check both halves of each accepted step and include every program knot.
        # Halving max_step is the temporal convergence check for unresolved extrema.
        points = {0.0}
        for section in self.sections:
            def slope(time):
                rates = section.rates(time, section.solution.sol(time))
                return section.extension_rate-float(self.expansion_weights @ rates[:-2])

            for start, end in zip(section.solution.t, section.solution.t[1:]):
                points.add(float(end))
                middle = (start+end)/2
                for a, b in ((start, middle), (middle, end)):
                    fa, fb = slope(a), slope(b)
                    if fa == 0 and fb != 0:
                        points.add(float(a))
                    elif fb == 0 and fa != 0:
                        points.add(float(b))
                    elif fa*fb < 0:
                        points.add(float(brentq(slope, a, b)))
        return sorted(points)

    def crossing(self, points, threshold):
        from scipy.optimize import brentq

        if self.drive(points[0]) >= threshold:
            return points[0]
        for a, b in zip(points, points[1:]):
            after = self.drive(b)-threshold
            if after >= 0:
                return float(brentq(lambda t: self.drive(t)-threshold, a, b))
        return None


def solve_coupon(data):
    import numpy as np

    area = positive(data, "area_m2")
    reference = nonnegative(data, "reference_K")
    if not isinstance(data["layers"], list) or len(data["layers"]) != 2:
        raise StudyError("the end-bonded study requires two layers in series")
    layers = [CouponLayer.from_input(layer, reference) for layer in data["layers"]]
    if layers[0].name == layers[1].name:
        raise StudyError("the layers require distinct names")
    bond = data["bond"]
    law = CohesiveLaw.from_input(bond)
    resistance = nonnegative(bond, "thermal_resistance_m2_K_W")
    fixture = data.get("fixture", {})
    fixture_compliance = (nonnegative(fixture, "compliance_m_Pa")
                          if "compliance_m_Pa" in fixture else 0.0)
    if fixture_compliance:
        required_source(fixture)
    compliance = fixture_compliance + sum(layer.length/layer.modulus for layer in layers)
    mechanics = BondedAxis(law, compliance, positive(bond, "compression_stiffness_Pa_m"))
    program = [ProgramPoint.from_input(point) for point in data["program"]]
    if len(program) < 2 or program[0].time != 0 or any(
            a.time >= b.time for a, b in zip(program, program[1:])):
        raise StudyError("program times must start at zero and increase")
    period = positive(data, "observation_interval_s")
    max_step = positive(data, "max_step_s")
    duration = program[-1].time
    if duration / period > 100_000 or duration / max_step > 100_000:
        raise StudyError("coupon run requests more than 100000 intervals")

    sections, widths, capacities, initial, half_resistance, bond_g = solve_thermal(
        layers, area, resistance, program, max_step)
    weights = widths*np.repeat([layer.expansion for layer in layers], [layer.cell_count for layer in layers])
    history = CouponHistory(sections, weights, reference)
    critical = history.breakpoints()
    max_drive = max(history.drive(t) for t in critical)
    # Reject a snap-back loading path before constructing a partial result.
    peak_opening = law.maximum_opening(max_drive, compliance)
    events = []
    threshold = law.onset+compliance*law.strength
    tangent = 1-compliance*law.strength/(law.failure-law.onset)
    event_thresholds = [("damage_onset", threshold)]
    if tangent > 0:
        event_thresholds.append(("complete_separation", law.failure))
    for name, level in event_thresholds:
        if (timestamp := history.crossing(critical, level)) is not None:
            events.append({"name": name, "time_s": timestamp, "message": "Tensile opening crossed threshold"})
    sample_times = {min(duration, i*period) for i in range(int(duration/period)+1)}
    sample_times.update(point.time for point in program)
    sample_times.update(event["time_s"] for event in events)
    observations = {name: {"unit": unit, "samples": []} for name, unit in (
        ("left_layer_average", "K"), ("right_layer_average", "K"),
        ("left_bond_face", "K"), ("right_bond_face", "K"),
        ("free_extension", "m"), ("grip_extension", "m"), ("interface_displacement", "m"),
        ("nominal_stress", "Pa"), ("axial_force", "N"), ("damage", "1"),
        ("bond_dissipation", "J"), ("heat_rate_across_bond", "W"),
    )}
    maximum_drive = 0.0
    next_critical = 0
    left = layers[0].cell_count-1
    for time in sorted(sample_times):
        while next_critical < len(critical) and critical[next_critical] <= time:
            maximum_drive = max(maximum_drive, history.drive(critical[next_critical]))
            next_critical += 1
        section = history.section(time)
        temperature = section.solution.sol(time)
        drive = history.drive(time)
        response = mechanics.response(drive, max(maximum_drive, drive))
        heat_rate = bond_g*(temperature[left]-temperature[left+1])
        left_mean = float(widths[:left+1]@temperature[:left+1]/layers[0].length)
        right_mean = float(widths[left+1:]@temperature[left+1:-2]/layers[1].length)
        values = (
            left_mean, right_mean, temperature[left]-heat_rate/area*half_resistance[left],
            temperature[left+1]+heat_rate/area*half_resistance[left+1], history.extension(temperature),
            section.extension(time), response.interface_displacement, response.stress, area*response.stress,
            response.damage, area*response.dissipated_energy, heat_rate,
        )
        for series, value in zip(observations.values(), values):
            series["samples"].append([time, float(value)])

    final = sections[-1].solution.y[:, -1]
    internal = float(capacities@(final[:-2]-initial))
    incoming = float(sum(final[-2:]))
    return {
        "layer_names": [layer.name for layer in layers],
        "parameter_sources": {layer.name: layer.source for layer in layers},
        "bond_parameter_source": law.source, "thermal_cells": len(widths),
        "accepted_heat_steps": sum(len(section.solution.t)-1 for section in sections),
        "maximum_opening_m": peak_opening, "final_damage": law.damage(peak_opening),
        "onset_drive_m": threshold, "softening_drive_tangent": tangent,
        "heat_left_J": float(final[-2]), "heat_right_J": float(final[-1]),
        "thermal_energy_change_J": internal, "thermal_energy_residual_J": internal-incoming,
        "events": events, "observations": observations,
    }



def bonded_coupon(data):
    comparison = data.get("convergence")
    if comparison is None:
        return solve_coupon(data)
    levels = comparison["max_levels"]
    if type(levels) is not int or not 2 <= levels <= 8:
        raise StudyError("convergence requires 2–8 levels")
    limits = {
        "opening_delta_m": positive(comparison, "opening_tolerance_m"),
        "damage_delta": positive(comparison, "damage_tolerance"),
        "event_delta_s": positive(comparison, "event_tolerance_s"),
    }
    work = dict(data, layers=[dict(layer) for layer in data["layers"]])
    attempts = []
    done = False
    for _ in range(levels):
        if any(layer["cells"] > 1000 for layer in work["layers"]):
            break
        result = solve_coupon(work)
        summary = {
            "cells_per_layer": [layer["cells"] for layer in work["layers"]],
            "max_step_s": work["max_step_s"],
            "peak_opening_m": result["maximum_opening_m"], "final_damage": result["final_damage"],
            "events_s": {event["name"]: event["time_s"] for event in result["events"]},
            "energy_residual_J": result["thermal_energy_residual_J"],
        }
        if attempts:
            previous = attempts[-1]
            same_events = summary["events_s"].keys() == previous["events_s"].keys()
            summary["events_match"] = same_events
            summary["event_delta_s"] = (max((
                abs(t-previous["events_s"][name]) for name, t in summary["events_s"].items()
            ), default=0.0) if same_events else None)
            summary["opening_delta_m"] = abs(summary["peak_opening_m"]-previous["peak_opening_m"])
            summary["damage_delta"] = abs(summary["final_damage"]-previous["final_damage"])
            done = same_events and all(summary[key] <= limit for key, limit in limits.items())
        attempts.append(summary)
        if done:
            break
        for layer in work["layers"]:
            layer["cells"] *= 2
        work["max_step_s"] /= 2
    if not attempts:
        raise StudyError("no valid convergence level")
    result["convergence"] = {"converged": done, "limits": limits, "levels": attempts}
    return result
