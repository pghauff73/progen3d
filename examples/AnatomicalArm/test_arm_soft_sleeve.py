import math
import tempfile
import unittest
from pathlib import Path
from arm_soft_sleeve import RINGS, SIDES, assembly, export, surface, validate

class SoftSleeveTests(unittest.TestCase):
    def test_pose_sweep(self):
        for angle in (0,30,60,90,120,145):
            with self.subTest(angle=angle):
                result=validate(angle)
                self.assertTrue(result["passed"],result["errors"])
                self.assertGreater(result["minimum_radial_clearance_m"],0.0002)

    def test_surface_counts(self):
        for angle in (0,90,145):
            for garment in (False,True):
                verts,faces=surface(angle,garment)
                self.assertEqual(len(verts),RINGS*SIDES)
                self.assertEqual(len(faces),(RINGS-1)*SIDES*2)
                self.assertTrue(all(math.isfinite(x) for v in verts for x in v))

    def test_skin_and_sleeve_are_distinct(self):
        model=assembly(90)
        self.assertIn("sleeve",model)
        self.assertNotEqual(model["skin"][0],model["sleeve"][0])

    def test_export_report(self):
        with tempfile.TemporaryDirectory() as folder:
            result=export(folder,(0,90,145))
            self.assertTrue(result["passed"],result["poses"])
            self.assertTrue((Path(folder)/"validation.json").is_file())
            self.assertTrue((Path(folder)/"arm_sleeve_090.obj").is_file())

    def test_angle_limits(self):
        for angle in (-1,146):
            with self.assertRaises(ValueError):
                surface(angle)

if __name__=="__main__":
    unittest.main()
