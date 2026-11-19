"""Checks against analytic and independently constructed bonded-coupon cases."""
import json
from pathlib import Path
import tempfile
import unittest

from tools.engineering.cohesive import CohesiveLaw
from tools.engineering.coupon_measurements import compare_measurements, fit_cohesive
from tools.engineering.domains import analyse
from tools.engineering.quantities import StudyError
from tools.engineering.study import scene_catalog

EXAMPLES = Path("examples/EngineeringStudies")


def input_case():
    model = json.loads((EXAMPLES / "bonded-coupon.json").read_text())
    model.pop("geometry")
    model.pop("convergence")
    return model


def sample(result, quantity, instant=None):
    rows = result["observations"][quantity]["samples"]
    if instant is None:
        return rows[-1][1]
    return dict(rows)[instant]


class TestBondedCoupon(unittest.TestCase):
    def test_steady_layer_resistances_and_energy(self):
        case = input_case()
        case["program"] = [dict(time_s=t, left_K=350, right_K=310, grip_extension_m=0)
                           for t in (0, 2000)]
        case["observation_interval_s"] = 100
        case["max_step_s"] = 30
        result = analyse(case)
        resistance = .02/30 + .02/12 + .00025
        expected_flux = (350-310)/resistance
        self.assertAlmostEqual(sample(result, "heat_rate_across_bond"), case["area_m2"]*expected_flux, places=8)
        face_jump = sample(result, "left_bond_face") - sample(result, "right_bond_face")
        self.assertAlmostEqual(face_jump, expected_flux*.00025, places=8)
        self.assertLess(abs(result["thermal_energy_residual_J"]), 1e-7)

    def test_isothermal_series_solution_and_events(self):
        case = input_case()
        case["program"] = [dict(time_s=t, left_K=330, right_K=330, grip_extension_m=u)
                           for t, u in ((0, 0), (5, 80e-6), (10, 20e-6), (15, 125e-6), (20, -30e-6))]
        case["observation_interval_s"] = 1
        result = analyse(case)
        compliance = .02/70e9+.02/100e9
        onset = 2e6/60e9
        self.assertAlmostEqual(result["events"][0]["time_s"], (onset+2e6*compliance)/(80e-6/5), places=8)
        self.assertAlmostEqual(result["events"][1]["time_s"], 10+(110e-6-20e-6)/(105e-6/5), places=8)
        initial_stress = 60e9*16e-6/(1+60e9*compliance)
        self.assertAlmostEqual(sample(result, "nominal_stress", 1), initial_stress, places=6)
        self.assertAlmostEqual(sample(result, "free_extension"), 0, places=12)
        self.assertEqual(sample(result, "damage", 5), sample(result, "damage", 10))
        self.assertEqual(sample(result, "damage"), 1)
        self.assertLess(sample(result, "nominal_stress"), 0)
        self.assertAlmostEqual(sample(result, "bond_dissipation"), 110*.0004)

    def test_thermally_driven_event_and_mesh_convergence(self):
        case = json.loads((EXAMPLES / "bonded-coupon.json").read_text())
        scene = scene_catalog(EXAMPLES / "bonded-coupon.scene.json")
        result = analyse(case, scene)
        self.assertEqual(result["geometry_binding"]["initial_bond_gap_m"], 0)
        self.assertTrue(result["convergence"]["converged"])
        self.assertEqual(len(result["convergence"]["levels"]), 4)
        self.assertEqual(result["thermal_cells"], 128)
        self.assertGreater(result["events"][0]["time_s"], 120)
        self.assertLess(result["events"][0]["time_s"], 122)
        self.assertGreater(result["final_damage"], .98)
        self.assertLess(result["final_damage"], 1)
        damage = [value for _, value in result["observations"]["damage"]["samples"]]
        self.assertEqual(damage, sorted(damage))
        self.assertLess(abs(result["thermal_energy_residual_J"]), 1e-7)

    def test_unresolved_geometry_and_unstable_softening(self):
        bound = json.loads((EXAMPLES / "bonded-coupon.json").read_text())
        scene = scene_catalog(EXAMPLES / "bonded-coupon.scene.json")
        with self.assertRaisesRegex(StudyError, "requires a native scene"):
            analyse(bound)
        for revision in (None, "", " ", 47):
            with self.subTest(revision=revision):
                missing_pin = dict(bound, geometry=dict(bound["geometry"], revision=revision))
                with self.assertRaisesRegex(StudyError, "requires a pinned scene revision"):
                    analyse(missing_pin, scene)
        missing_pin = dict(bound, geometry=bound["geometry"].copy())
        del missing_pin["geometry"]["revision"]
        with self.assertRaisesRegex(StudyError, "requires a pinned scene revision"):
            analyse(missing_pin, scene)
        with self.assertRaisesRegex(StudyError, "revision is stale"):
            analyse(bound, dict(scene, revision="changed"))
        scene["objects"]["primitive:1"]["transform_column_major"][12] += 1
        with self.assertRaisesRegex(StudyError, "adjoining faces"):
            analyse(bound, scene)
        case = input_case()
        case["fixture"] = {"compliance_m_Pa": 1e-10, "parameter_source": "synthetic flexible rig"}
        case["program"] = [dict(time_s=t, left_K=330, right_K=330, grip_extension_m=u)
                           for t, u in ((0, 0), (2, .001))]
        with self.assertRaisesRegex(StudyError, "snap-back"):
            analyse(case)

    def test_monitor_sampling_does_not_control_damage(self):
        case = input_case()
        a = analyse(case)
        b = analyse(dict(case, observation_interval_s=112))
        self.assertEqual(a["events"], b["events"])
        self.assertEqual(a["final_damage"], b["final_damage"])
        self.assertAlmostEqual(sample(a, "damage"), sample(b, "damage"))

    def test_cohesive_fit_and_measurement_units(self):
        fit = fit_cohesive(EXAMPLES / "cohesive-fit-synthetic.csv", .0004, "Synthetic fit fixture")
        self.assertAlmostEqual(fit["stiffness_Pa_m"]/45e9, 1)
        self.assertAlmostEqual(fit["peak_traction_Pa"], 1.8e6)
        self.assertAlmostEqual(fit["fracture_energy_J_m2"], 117)
        inserted = CohesiveLaw.from_input(fit)
        self.assertAlmostEqual(inserted.failure, .00013)
        with tempfile.TemporaryDirectory() as directory:
            csv = Path(directory) / "measurements.csv"
            csv.write_text("time_s,channel,value,unit,standard_uncertainty\n"
                           "1,temperature,31.85,degC,1\n"
                           "2,displacement,10,um,2\n")
            trace = {"study": "fixture", "observations": {
                "temperature": {"unit": "K", "samples": [[0, 300], [2, 312]]},
                "displacement": {"unit": "m", "samples": [[0, 0], [2, 12e-6]]},
            }}
            comparison = compare_measurements(trace, csv, "Synthetic gauge test")
            self.assertAlmostEqual(comparison["channels"]["temperature"]["rmse"], 1)
            self.assertAlmostEqual(comparison["channels"]["displacement"]["bias"], 2e-6)
            self.assertAlmostEqual(comparison["rows"][1]["standardised_residual"], 1)
            csv.write_text("time_s,channel,value,unit\n1,temperature,31.85,delta_degC\n")
            with self.assertRaisesRegex(StudyError, "incompatible unit"):
                compare_measurements(trace, csv, "Synthetic gauge test")


if __name__ == "__main__":
    unittest.main()
