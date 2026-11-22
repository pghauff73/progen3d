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

    def test_export(self):
        with tempfile.TemporaryDirectory() as directory:
            result=simulate(directory,frames=3,fps=30)
            self.assertTrue(result["passed"],result["records"])
            self.assertTrue((Path(directory)/"simulation.json").exists())
            self.assertTrue((Path(directory)/"frame_0002.obj").exists())

    def test_invalid_angle(self):
        with self.assertRaises(ValueError):
            CoupledArm().step(150)

if __name__=="__main__":
    unittest.main()
