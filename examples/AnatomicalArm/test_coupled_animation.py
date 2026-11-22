import tempfile
import unittest
from pathlib import Path
from coupled_animation import CoupledArm, angle_at, simulate

class CoupledArmTests(unittest.TestCase):
    def test_angle_schedule(self):
        self.assertAlmostEqual(angle_at(0),0)
        self.assertAlmostEqual(angle_at(1),145)
        self.assertAlmostEqual(angle_at(2),0)

    def test_moving_contact(self):
        arm=CoupledArm()
        for angle in (0,30,60,90,120,145,120,60,0):
            with self.subTest(angle=angle):
                result=arm.step(angle,dt=1/60,iterations=8)
                self.assertTrue(result["passed"],result)
                self.assertGreaterEqual(result["maximum_edge_strain"],0)
                self.assertGreaterEqual(result["mean_edge_strain"],0)
                self.assertIsInstance(result["worst_clearance_ring"],int)
                self.assertGreaterEqual(result["worst_clearance_ring"],1)
                self.assertIsNotNone(result["minimum_elbow_band_gap_m"])
                self.assertGreaterEqual(result["minimum_elbow_band_gap_m"],result["minimum_radial_gap_m"]-1e-9)

    def test_export(self):
        with tempfile.TemporaryDirectory() as directory:
            result=simulate(directory,frames=3,fps=30)
            self.assertTrue(result["passed"],result["records"])
            self.assertTrue((Path(directory)/"simulation.json").exists())
            self.assertTrue((Path(directory)/"frame_0002.obj").exists())

    def test_triangle_contact_mode(self):
        arm=CoupledArm(contact_mode='triangle',friction=0.15)
        result=arm.step(15,dt=1/60,iterations=2)
        self.assertEqual(result['contact_mode'],'triangle')
        self.assertTrue(result['passed'],result)

    def test_invalid_contact_parameters(self):
        with self.assertRaises(ValueError):
            CoupledArm(contact_mode='unsupported')
        with self.assertRaises(ValueError):
            CoupledArm(friction=1.1)

    def test_substep_animation_export(self):
        with tempfile.TemporaryDirectory() as directory:
            result=simulate(directory,frames=3,fps=30,substeps=4)
            self.assertEqual(result["substeps"],4)
            self.assertEqual(result["solver_steps"],9)
            self.assertEqual(len(result["step_records"]),9)
            self.assertGreaterEqual(result["worst_clearance_step"],0)
            self.assertLess(result["worst_clearance_step"],9)
            self.assertLessEqual(result["minimum_clearance_m"],min(r["minimum_radial_gap_m"] for r in result["records"])+1e-9)
            self.assertEqual(len(result["records"]),3)
            self.assertAlmostEqual(result["records"][-1]["angle"],angle_at(2/30))
            self.assertTrue(result["passed"],result["records"])

    def test_invalid_substeps(self):
        with tempfile.TemporaryDirectory() as directory:
            for value in (0,-1,33,1.5):
                with self.subTest(value=value):
                    with self.assertRaises(ValueError):
                        simulate(directory,frames=2,substeps=value)

    def test_invalid_angle(self):
        with self.assertRaises(ValueError):
            CoupledArm().step(150)

if __name__=="__main__":
    unittest.main()
