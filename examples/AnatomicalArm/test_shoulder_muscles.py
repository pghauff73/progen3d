import math
import unittest
from spatial_skeleton import spatial_skeleton
from shoulder_muscles import shoulder_muscles,spindle
from spatial_body import spatial_body
from mesh_diagnostics import inspect_mesh

class ShoulderMuscleTests(unittest.TestCase):
    def test_bilateral_muscle_envelopes(self):
        muscles=shoulder_muscles(spatial_skeleton())
        self.assertEqual(len(muscles),6)
        for name,mesh in muscles.items():
            with self.subTest(name=name):
                self.assertEqual(len(mesh[0]),21*16)
                self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_elevation_moves_muscle(self):
        rest=shoulder_muscles(spatial_skeleton())
        raised=shoulder_muscles(spatial_skeleton({"left_arm_elevation":140}))
        self.assertNotEqual(rest["left_deltoid"][0],raised["left_deltoid"][0])

    def test_spatial_body_contains_muscles(self):
        meshes,_=spatial_body()
        self.assertIn("left_deltoid",meshes)
        self.assertIn("right_pectoralis",meshes)

    def test_invalid_belly(self):
        with self.assertRaises(ValueError):
            spindle((0,0,0),(0,1,0),belly=0)

if __name__=="__main__":
    unittest.main()
