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
        self.assertEqual(len(meshes),30)
        self.assertTrue(audit()["passed"])
        for name,mesh in meshes.items():
            with self.subTest(name=name):
                if name.startswith("torso_"):
                    expected=(33*32,32*32*2)
                elif name.endswith("_continuous_skin"):
                    expected=(61*20,60*20*2)
                elif name.endswith(("_humeral_trochlea","_ulnar_notch","_radial_head")):
                    expected=(12*28,11*28*2)
                elif name.endswith(("_radius_bone","_ulna_bone")):
                    expected=(24*20,23*20*2)
                elif name.split("_",1)[-1] in ("deltoid","trapezius","pectoralis"):
                    expected=(21*16,20*16*2)
                else:
                    expected=(24*20,23*20*2)
                self.assertEqual((len(mesh[0]),len(mesh[1])),expected)
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
