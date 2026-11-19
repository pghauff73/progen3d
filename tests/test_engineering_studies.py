import copy
import json
import math
import tempfile
import unittest
from pathlib import Path

from tools.engineering.coating_stack import coating_stack
from tools.engineering.desktop import run_project
from tools.engineering.domains import analyse
from tools.engineering.quantities import (
    ABSOLUTE, INTERVAL, QuantityRegistry, StudyError, literal, parse_unit, sound_level_ratio,
)
from tools.engineering.simulation import replay, simulate
from tools.engineering.study import Expression, load_extensions, parse_study, scene_catalog
from tools.engineering.view import render_view

EXAMPLES = Path("examples/EngineeringStudies")


class EngineeringStudies(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.file = Path(directory.name) / "study.p3physics"
        self.coupon = (EXAMPLES / "coupon.p3physics").read_text()
        self.scene = scene_catalog(EXAMPLES / "coupon.scene.json")

    def compile(self, source=None):
        self.file.write_text(source if source is not None else self.coupon)
        return parse_study(self.file, self.scene)

    def test_registry_and_units(self):
        registry = QuantityRegistry()
        self.assertEqual(Expression.parse("2 [m^2] * 3", 1).evaluate({}).value, 6)
        self.assertEqual(len(registry.kinds), 1230)
        self.assertEqual(sum(item.lifecycle == "deprecated" for item in registry.kinds.values()), 98)
        self.assertEqual(sum(item.dimensions is None for item in registry.kinds.values()), 6)
        kind = registry.kind("http://qudt.org/vocab/quantitykind/Torque")
        self.assertEqual(kind, registry.kind("qudt:Torque"))
        self.assertAlmostEqual(literal(180, "deg").value, math.pi)
        self.assertAlmostEqual(literal(1, "mm").value, .001)
        self.assertEqual(parse_unit("m^(1/2) * m^(1/2) / s^2").dimensions,
                         parse_unit("m/s^2").dimensions)
        with self.assertRaisesRegex(StudyError, "explicit convention"):
            sound_level_ratio(10, literal(1, "Pa"), "")
        self.assertAlmostEqual(sound_level_ratio(20, literal(2, "Pa"), "amplitude").value, 20)

    def test_kinds_and_affine(self):
        registry = QuantityRegistry()
        for target, source, unit in (("Torque", "Energy", "J"), ("Stress", "Pressure", "Pa")):
            with self.assertRaisesRegex(StudyError, f"cannot assign {source}"):
                registry.assign(registry.assign(literal(1, unit), source), target)
        room = literal(20, "degC")
        self.assertEqual(room.kind, ABSOLUTE)
        self.assertAlmostEqual(room.value, 293.15)
        self.assertEqual(literal(313.15, "K").add(room, subtract=True).kind, INTERVAL)
        with self.assertRaisesRegex(StudyError, "absolute temperatures"):
            room.add(room)

    def test_coating_units_provenance_and_layer_order(self):
        fixture = json.loads((EXAMPLES / "cylindrical-fit-coated.json").read_text())
        materials = fixture["materials"]
        registry = QuantityRegistry()
        spec = fixture["shaft"]["coating"]
        primer = spec["layers"][0]
        spec["layers"] = [primer]
        primer["thickness"].update(value=1, unit="mm")
        result = coating_stack(spec, materials, registry)
        self.assertAlmostEqual(result["thermal_resistance_K_W"], 0.5)
        self.assertAlmostEqual(result["heat_flow_W"], 2.0)
        conductivity = result["materials"]["Primer"]["ThermalConductivity"]
        self.assertEqual(conductivity["source"], "Analytical example")
        self.assertEqual(conductivity["si_uncertainty"], 0.01)
        self.assertAlmostEqual(conductivity["temperature_range"]["min_K"], 288.15)
        self.assertAlmostEqual(conductivity["temperature_range"]["max_K"], 313.15)

        primer["thickness"].update(value=0.001, unit="m")
        spec["area"].update(value=0.01, unit="m^2")
        spec["temperature_difference"]["unit"] = "delta_degC"
        equivalent = coating_stack(spec, materials, registry)
        for key in ("fit_thickness_m", "thermal_resistance_K_W", "heat_flow_W"):
            self.assertAlmostEqual(equivalent[key], result[key])
        materials["Primer"]["appearance"] = {"colour": "red", "roughness": 0.9}
        self.assertEqual(coating_stack(spec, materials, registry), equivalent)

        base = json.loads((EXAMPLES / "cylindrical-fit-coated.json").read_text())["shaft"]["coating"]
        base["layers"].reverse()
        saved = json.loads(json.dumps(base))
        report = coating_stack(saved, materials, registry)
        self.assertEqual([layer["material"] for layer in report["layers"]], ["Topcoat", "Primer"])
        self.assertAlmostEqual(report["thermal_resistance_K_W"], 0.01)
        saved["layers"][0]["affects_fit"] = False
        self.assertAlmostEqual(coating_stack(saved, materials, registry)["fit_thickness_m"], 5e-6)

    def test_coating_rejects_missing_and_invalid_physical_inputs(self):
        fixture = json.loads((EXAMPLES / "cylindrical-fit-coated.json").read_text())
        materials = fixture["materials"]
        registry = QuantityRegistry()
        spec = fixture["shaft"]["coating"]
        for changes, message in (
            ({"unit": "s"}, "dimension mismatch for Thickness"),
            ({"source": " "}, "requires a source"),
            ({"value": 0}, "coating thickness must be positive"),
            ({"uncertainty": -0.01}, "must be nonnegative"),
        ):
            with self.subTest(changes=changes), self.assertRaisesRegex(StudyError, message):
                edited = copy.deepcopy(spec)
                edited["layers"][0]["thickness"].update(changes)
                coating_stack(edited, materials, registry)
        del materials["Primer"]["properties"]["ThermalConductivity"]
        with self.assertRaisesRegex(StudyError, "needs ThermalConductivity"):
            coating_stack(spec, materials, registry)
        del spec["temperature_difference"]
        del spec["layers"][1]["interface_resistance"]
        for layer in spec["layers"]:
            layer["affects_thermal"] = False
        self.assertNotIn("thermal_resistance_K_W", coating_stack(spec, materials, registry))

    def test_semantic_rejections(self):
        replacements = [
            ("4 [W/K]", "4 [W]", "dimension mismatch"),
            ("T - ambient", "T + ambient", "absolute temperatures"),
            ('object("primitive:0")', 'object("primitive:0") revision "obsolete"', "stale scene"),
            ('object("primitive:0")', 'object("primitive:0") face "missing"', "missing face"),
            ("  observe T", "  equation duplicate : der(T) = Qout\n  observe T", "duplicate equation"),
            ("  equation energy_balance : C * der(T) = -Qout\n", "", "exactly one"),
            ("60 [degC]", "(-20 [degC])**2", "invalid quantity exponent"),
            ("4 [W/K]", "quantity(4, 'W/K').__class__", "unsupported expression"),
        ]
        for old, new, error in replacements:
            with self.subTest(new=new), self.assertRaisesRegex(StudyError, error):
                self.compile(self.coupon.replace(old, new))

    def test_coupon_event_and_replay(self):
        study = self.compile()
        trace = simulate(study)
        self.assertLess(abs(trace.events[0]["time_s"] - 50*math.log(2)), 1e-6)
        self.assertAlmostEqual(trace.state(50)[0], 293.15+40/math.e, places=6)
        saved = json.loads(json.dumps(trace.checkpoint(32.25)))
        self.assertEqual(trace.results(), replay(study, saved).results())
        modified = self.compile(self.coupon.replace("0.5 [s]", "1 [s]"))
        self.assertEqual(trace.results()["accepted_steps_s"], simulate(modified).results()["accepted_steps_s"])
        with self.assertRaisesRegex(StudyError, "different model"):
            replay(modified, saved)

    def test_fixed_step_refinement(self):
        errors = []
        for step in (5, 2.5):
            study = self.compile(self.coupon.replace("method BDF", f"method RK4 step {step} [s]"))
            errors.append(abs(simulate(study).state(50)[0] - (293.15+40/math.e)))
        self.assertGreater(errors[0] / errors[1], 13)

    def test_scheduled_and_root_resets(self):
        study = self.compile("""study Changes version 1 {
  parameter speed : Velocity = 1 [m/s]
  state x : Length = 0 [m]
  equation motion : der(x) = speed
  observe x every 0.1 [s] display [m]
  event kick at 0.5 [s] { set x = 2 [m] }
  event catch when rising(x - 2.5 [m]) { set x = 3 [m] }
  integrate from 0 [s] to 1.5 [s] method RK4 step 0.3 [s]
}
""")
        trace = simulate(study)
        scheduled, root = trace.events
        self.assertEqual((scheduled["name"], root["name"]), ("kick", "catch"))
        self.assertAlmostEqual(trace.state(.5, "before")[0], .5)
        self.assertEqual(trace.state(.5), [2])
        self.assertAlmostEqual(root["time_s"], 1, places=8)
        self.assertAlmostEqual(trace.state(1.5)[0], 3.5, places=8)

    def test_heat_network_and_conduction(self):
        case = json.loads((EXAMPLES / "coolant.json").read_text())
        coupled = analyse(case)
        self.assertLess(abs(coupled["energy_residual_J"]), 1e-7)
        self.assertLess(coupled["energy_change_J"], 0)
        del case["coolant"]
        isolated = analyse(case)
        self.assertLess(abs(isolated["energy_change_J"]), 1e-7)
        errors = []
        for n in (12, 24, 48):
            theta = [math.sin(math.pi*(i+.5)/n) for i in range(n)]
            result = analyse({"version": 1, "model": "wall_conduction", "length_m": 1, "area_m2": 1,
                              "conductivity_W_m_K": 1, "volumetric_capacity_J_m3_K": 100,
                              "duration_s": 2, "cells": n, "left_K": 300, "right_K": 300,
                              "initial_K": [300+10*x for x in theta]})
            decay = math.exp(-2*math.pi**2/100)
            errors.append(max(abs(T-300-10*q*decay) for T, q in zip(result["final_K"], theta)))
        self.assertGreater(errors[0] / errors[1], 3.7)
        self.assertGreater(errors[1] / errors[2], 3.7)

    def test_expansion_and_laminar_flow(self):
        member = dict(young_modulus_Pa=70e9, area_m2=.01, alpha_per_K=23e-6, delta_K=40)
        case = {"version": 1, "model": "expansion", "length_m": .2, "members": [member]}
        self.assertAlmostEqual(analyse(case)["elongation_m"], .000184)
        self.assertLess(analyse(dict(case, prescribed_strain=0))["stress_Pa"][0], 0)
        case["members"].append(dict(member, alpha_per_K=10e-6))
        result = analyse(case)
        self.assertLess(result["stress_Pa"][0], 0)
        self.assertGreater(result["stress_Pa"][1], 0)
        self.assertAlmostEqual(result["reaction_N"], 0)
        pipe = {"version": 1, "model": "laminar_pipe", "diameter_m": .01,
                "dynamic_viscosity_Pa_s": .001, "length_m": 2,
                "volume_flow_m3_s": 1e-6, "density_kg_m3": 1000}
        self.assertAlmostEqual(analyse(pipe)["pressure_drop_Pa"], 8.1487330863)
        with self.assertRaisesRegex(StudyError, "Reynolds"):
            analyse(dict(pipe, volume_flow_m3_s=.1))

    def test_index_one_circuit(self):
        case = json.loads((EXAMPLES / "circuit.json").read_text())
        data = analyse(case)
        self.assertAlmostEqual(data["final_node_V"][2], 5*(1-math.exp(-1)), places=7)
        self.assertAlmostEqual(data["resistor_power_W"][0], (2.5/math.e)**2/50, places=8)
        self.assertLess(data["algebraic_max_residual_A"], 1e-9)
        with self.assertRaisesRegex(StudyError, "inconsistent"):
            analyse(dict(case, algebraic_initial_V=[1]))
        floating = copy.deepcopy(case)
        floating["algebraic_nodes"] = 2
        with self.assertRaisesRegex(StudyError, "floating"):
            analyse(floating)

    def test_flux_mapping_and_multirate_rollback(self):
        flux = analyse({
            "version": 1, "model": "surface_flux_transfer", "width_m": 2,
            "source_edges_m": [0, 1, 4], "target_edges_m": [0, 2, 3, 4],
            "source_flux_W_m2": [10, 2],
        })
        self.assertEqual(flux["target_flux_W_m2"], [6, 2, 2])
        self.assertEqual(flux["transfer_residual_W"], 0)
        case = json.loads((EXAMPLES / "split-coolant.json").read_text())
        trial = analyse(case)
        self.assertGreater(trial["rejected_exchange_steps"], 0)
        self.assertLess(abs(trial["energy_residual_J"]), 1e-7)
        benchmark = analyse(json.loads((EXAMPLES / "coolant.json").read_text()))["final_K"][0]
        errors = [abs(analyse(dict(case, exchange_step_s=h))["wall_final_K"]-benchmark)
                  for h in (1, .5)]
        self.assertGreater(errors[0] / errors[1], 1.8)

    def test_ui_execution_entry_matches_cli_model(self):
        result = run_project(EXAMPLES / "coupon.p3physics", EXAMPLES / "coupon.scene.json")
        self.assertEqual(result, simulate(self.compile()).results())

    def test_cohesive_damage_is_irreversible(self):
        example = json.loads((EXAMPLES / "cohesive-opening.json").read_text())
        data = analyse(example)
        damage = [value for _, value in data["observations"]["damage"]["samples"]]
        self.assertEqual(damage, sorted(damage))
        self.assertEqual(damage[1], 0)
        self.assertEqual(damage[2], damage[3])
        self.assertEqual(damage[-1], 1)
        traction = [value for _, value in data["observations"]["traction"]["samples"]]
        self.assertAlmostEqual(traction[1], 1e6)
        self.assertEqual(traction[-1], 0)
        with self.assertRaisesRegex(StudyError, "parameter source"):
            analyse(dict(example, parameter_source=""))

    def test_custom_kind_and_view(self):
        kinds = load_extensions(EXAMPLES / "project-kinds.json")
        trace = simulate(parse_study(EXAMPLES / "coating.p3physics", extensions=kinds))
        result = trace.results()
        self.assertAlmostEqual(result["observations"]["thickness"]["samples"][-1][1], 22)
        output = self.file.with_suffix(".html")
        render_view([result], output)
        self.assertIn('CoatingThicknessExample', output.read_text())
        with self.assertRaisesRegex(StudyError, "no verified provider"):
            analyse({"version": 1, "model": "unknown"})


if __name__ == "__main__":
    unittest.main()
