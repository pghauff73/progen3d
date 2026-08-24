from __future__ import annotations

import json
import math
import os
from pathlib import Path
import re
import sys
import unittest

os.environ.setdefault("MCSM_DISABLE_VTK", "1")

RELEASE_ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = RELEASE_ROOT / "source"
if str(SOURCE_ROOT) not in sys.path:
    sys.path.insert(0, str(SOURCE_ROOT))

import numpy as np
import trimesh
import modern_car_mcsmv2 as mcsm  # noqa: E402


class MCSMv22UnitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.variants = mcsm.base.derive_v2_variants()
        cls.variant = cls.variants["reference"]
        cls.model = mcsm.base.ModernCarModelV2(cls.variant)
        cls.suspension = mcsm.SuspensionKinematicModel(cls.model)
        cls.semantic = mcsm.SemanticUVSurface.build(cls.model)
        cls.partition = mcsm.build_surface_partition(cls.semantic)
        cls.closures = mcsm.build_closure_system(cls.model, cls.partition)

    def test_01_version_and_schema(self) -> None:
        self.assertEqual(mcsm.MCSM_VERSION, "2.2.0")
        self.assertTrue(mcsm.MODEL_SCHEMA.startswith("MCSMv2.2"))

    def test_02_unit_vector_normalizes(self) -> None:
        value = mcsm._unit([3.0, 4.0, 0.0])
        self.assertAlmostEqual(float(np.linalg.norm(value)), 1.0, places=12)
        self.assertTrue(np.allclose(value, [0.6, 0.8, 0.0]))

    def test_03_zero_vector_uses_fallback(self) -> None:
        value = mcsm._unit([0.0, 0.0, 0.0], fallback=[0.0, 1.0, 0.0])
        self.assertTrue(np.allclose(value, [0.0, 1.0, 0.0]))

    def test_04_axis_rotation_preserves_homogeneous_form(self) -> None:
        transform = mcsm.axis_rotation_matrix([0, 0, 0], [0, 0, 1], math.pi / 2)
        self.assertTrue(np.allclose(transform[3], [0, 0, 0, 1]))
        self.assertAlmostEqual(float(np.linalg.det(transform[:3, :3])), 1.0, places=12)

    def test_05_transformed_points_rotate_correctly(self) -> None:
        transform = mcsm.axis_rotation_matrix([0, 0, 0], [0, 0, 1], math.pi / 2)
        point = mcsm.transformed_points(np.asarray([[1.0, 0.0, 0.0]]), transform)[0]
        self.assertTrue(np.allclose(point, [0.0, 1.0, 0.0], atol=1e-10))

    def test_06_tyre_mesh_is_watertight(self) -> None:
        tyre = mcsm.create_tyre_mesh(0.335, 0.235, major_sections=24, minor_sections=10)
        self.assertTrue(tyre.is_watertight)
        self.assertTrue(tyre.is_winding_consistent)
        self.assertGreater(len(tyre.faces), 0)

    def test_07_tyre_mesh_has_expected_outer_radius(self) -> None:
        tyre = mcsm.create_tyre_mesh(0.335, 0.235, major_sections=32, minor_sections=12)
        radial = np.sqrt(np.square(tyre.vertices[:, 0]) + np.square(tyre.vertices[:, 2]))
        self.assertAlmostEqual(float(np.max(radial)), 0.335, delta=0.003)

    def test_08_suspension_hardpoint_validation(self) -> None:
        report = self.suspension.validate()
        self.assertTrue(report["pass"])
        self.assertEqual(report["hardpoint_count"], 32)
        self.assertLessEqual(report["maximum_mirror_residual_m"], 1e-12)

    def test_09_front_wheel_pose_keeps_declared_steer(self) -> None:
        pose = self.suspension.wheel_pose("front", "left", 17.0, 0.02)
        self.assertAlmostEqual(pose.steer_deg, 17.0)
        self.assertAlmostEqual(np.linalg.det(np.asarray(pose.transform)[:3, :3]), 1.0, places=10)

    def test_10_rear_wheel_pose_zeroes_steer(self) -> None:
        pose = self.suspension.wheel_pose("rear", "right", 22.0, -0.02)
        self.assertAlmostEqual(pose.steer_deg, 0.0)

    def test_11_pose_grid_count(self) -> None:
        self.assertEqual(len(self.suspension.pose_grid("front", "left", 5, 3)), 15)
        self.assertEqual(len(self.suspension.pose_grid("rear", "left", 5, 3)), 3)

    def test_12_semantic_projection_succeeds(self) -> None:
        report = self.semantic.projection_report
        self.assertTrue(report["pass"])
        self.assertEqual(report["failure_count"], 0)
        self.assertLess(report["maximum_abs_field_residual_m"], 1e-8)

    def test_13_semantic_uv_dimensions(self) -> None:
        self.assertEqual(len(self.semantic.mesh.vertices), len(self.semantic.uv))
        self.assertTrue(np.all(self.semantic.uv >= 0.0))
        self.assertTrue(np.all(self.semantic.uv <= 1.0))

    def test_14_surface_partition_is_exclusive(self) -> None:
        report = self.partition.graph
        self.assertTrue(report["pass"])
        self.assertAlmostEqual(report["coverage_fraction"], 1.0)
        self.assertEqual(report["unassigned_face_count"], 0)
        self.assertEqual(report["resolved_multiple_owner_face_count"], 0)

    def test_15_surface_partition_has_six_apertures(self) -> None:
        self.assertEqual(len(self.partition.aperture_meshes), 6)
        self.assertEqual(set(self.partition.aperture_meshes), {
            "windshield", "rear_glass",
            "front_side_glass_left", "front_side_glass_right",
            "rear_side_glass_left", "rear_side_glass_right",
        })

    def test_16_surface_partition_has_six_closures(self) -> None:
        self.assertEqual(len(self.partition.closure_meshes), 6)
        self.assertEqual(set(self.partition.closure_meshes), {
            "front_door_left", "front_door_right",
            "rear_door_left", "rear_door_right", "hood", "rear_hatch",
        })

    def test_17_closure_system_counts(self) -> None:
        self.assertEqual(len(self.closures.hinges), 6)
        self.assertEqual(len(self.closures.glass_systems), 4)

    def test_18_hinge_state_zero_is_identity(self) -> None:
        for hinge in self.closures.hinges.values():
            self.assertTrue(np.allclose(hinge.transform(0.0), np.eye(4), atol=1e-12))

    def test_19_hinge_rotation_is_rigid(self) -> None:
        for hinge in self.closures.hinges.values():
            rotation = hinge.transform(1.0)[:3, :3]
            self.assertAlmostEqual(float(np.linalg.det(rotation)), 1.0, places=9)

    def test_20_helical_glass_zero_is_identity(self) -> None:
        for system in self.closures.glass_systems.values():
            self.assertTrue(np.allclose(system.local_transform(0.0), np.eye(4), atol=1e-12))

    def test_21_helical_glass_full_state_moves_down(self) -> None:
        for system in self.closures.glass_systems.values():
            transform = system.local_transform(1.0)
            pivot = np.asarray([system.pivot])
            moved = mcsm.transformed_points(pivot, transform)[0]
            self.assertLess(moved[2], system.pivot[2])
            self.assertAlmostEqual(float(np.linalg.det(transform[:3, :3])), 1.0, places=9)

    def test_22_direct_closure_sweep_validation(self) -> None:
        self.assertTrue(mcsm.validate_closure_sweeps(self.closures)["pass"])

    def test_23_direct_glass_motion_validation(self) -> None:
        self.assertTrue(mcsm.validate_glass_motion(self.closures)["pass"])


class MCSMv22ReleaseAcceptanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.validation = json.loads((RELEASE_ROOT / "validation.json").read_text(encoding="utf-8"))

    def test_24_all_four_variants_present(self) -> None:
        self.assertEqual(set(self.validation), {"reference", "track", "aero", "crossover"})

    def test_25_all_variants_reach_v3(self) -> None:
        for record in self.validation.values():
            self.assertEqual(record["assurance"]["achieved_level"], "V3")
            self.assertTrue(record["release_gate_pass"])

    def test_26_all_variants_have_complete_partition(self) -> None:
        for record in self.validation.values():
            report = record["surface_partition"]
            self.assertTrue(report["pass"])
            self.assertEqual(report["coverage_fraction"], 1.0)
            self.assertEqual(len(report["apertures"]), 6)

    def test_27_all_variants_pass_actual_tyre_sweeps(self) -> None:
        for record in self.validation.values():
            report = record["actual_tyre_pose_sweeps"]
            self.assertTrue(report["pass"])
            self.assertEqual(report["pose_count"], 36)
            self.assertEqual(report["wheel_count"], 4)
            self.assertGreaterEqual(
                report["minimum_clearance_m"] + 0.004,
                report["declared_clearance_m"],
            )

    def test_28_all_variants_pass_closure_and_glass_motion(self) -> None:
        for record in self.validation.values():
            self.assertTrue(record["closure_kinematics"]["pass"])
            self.assertTrue(record["helical_glass_kinematics"]["pass"])
            self.assertEqual(len(record["closure_kinematics"]["closures"]), 6)
            self.assertEqual(len(record["helical_glass_kinematics"]["glass_systems"]), 4)

    def test_29_all_variants_pass_hardpoint_model(self) -> None:
        for record in self.validation.values():
            hardpoints = record["suspension_hardpoint_model"]
            self.assertTrue(hardpoints["pass"])
            self.assertEqual(hardpoints["hardpoint_count"], 32)

    def test_30_alignment_is_within_declared_gate(self) -> None:
        for record in self.validation.values():
            alignment = record["semantic_surface_implicit_alignment"]
            self.assertTrue(alignment["pass"])
            self.assertLessEqual(alignment["combined_p95_m"], alignment["distance_tolerance_m"])
            self.assertLessEqual(alignment["normal_angle_p95_deg"], alignment["normal_tolerance_deg"])

    def test_31_body_stl_files_are_watertight(self) -> None:
        for key in self.validation:
            path = RELEASE_ROOT / "models" / f"modern_car_v2_{key}_body.stl"
            mesh = trimesh.load_mesh(path, process=True)
            self.assertTrue(mesh.is_watertight, path.name)
            self.assertTrue(mesh.is_winding_consistent, path.name)

    def test_32_reference_glb_assets_load(self) -> None:
        names = [
            "modern_car_v2_reference.glb",
            "modern_car_v2_reference_engineering.glb",
            "modern_car_v2_reference_surface_partition.glb",
            "modern_car_v2_reference_suspension_kinematics.glb",
            "modern_car_v2_reference_closures_closed.glb",
            "modern_car_v2_reference_closures_open.glb",
            "modern_car_v2_variations_comparison.glb",
        ]
        for name in names:
            scene = trimesh.load(RELEASE_ROOT / "models" / name, force="scene")
            self.assertGreater(len(scene.geometry), 0, name)

    def test_33_kinematic_json_records_are_present(self) -> None:
        for key in self.validation:
            for suffix in ("suspension_hardpoints", "wheel_pose_sweeps", "closure_system"):
                path = RELEASE_ROOT / "kinematics" / f"{key}_{suffix}.json"
                self.assertTrue(path.is_file(), str(path))
                payload = json.loads(path.read_text(encoding="utf-8"))
                self.assertIsInstance(payload, dict)

    def test_34_target_grammar_is_structurally_balanced(self) -> None:
        text = (RELEASE_ROOT / "MCSMv2_Modern_Car_Family.p3d").read_text(encoding="utf-8")
        code = "\n".join(line.split("#", 1)[0] for line in text.splitlines())
        stack: list[str] = []
        pairs = {")": "(", "]": "[", "}": "{"}
        for char in code:
            if char in "([{":
                stack.append(char)
            elif char in ")]}":
                self.assertTrue(stack)
                self.assertEqual(stack.pop(), pairs[char])
        self.assertEqual(stack, [])
        self.assertIsNone(re.search(r"\bObject\s*\(", code))


if __name__ == "__main__":
    unittest.main(verbosity=2)
