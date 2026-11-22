import math
import unittest
from spatial_body import spatial_body,segment_surface,audit
from mesh_diagnostics import inspect_mesh

class SpatialBodyTests(unittest.TestCase):
    def test_bilateral_limb_surfaces(self):
        meshes,joints=spatial_body({
            "left_shoulder_xyz":(25,30,45),
            "right_shoulder_xyz":(-20,15,-35),
            "left_hip_xyz":(20,-10,25),
            "left_elbow_flex":110,
            "right_knee_flex":125})
        self.assertEqual(len(meshes),8)
        self.assertTrue(audit()["passed"])
        for name,mesh in meshes.items():
            with self.subTest(name=name):
                self.assertEqual(len(mesh[0]),24*20)
                self.assertEqual(len(mesh[1]),23*20*2)
                self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_arbitrary_direction(self):
        for end in ((1,0,0),(0,1,0),(0,0,1),(0.3,0.4,0.5)):
            with self.subTest(end=end):
                mesh=segment_surface((0,0,0),end,0.1,0.05)
                self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_zero_length_rejected(self):
        with self.assertRaises(ValueError):
            segment_surface((0,0,0),(0,0,0),0.1,0.1)

if __name__=="__main__":
    unittest.main()
