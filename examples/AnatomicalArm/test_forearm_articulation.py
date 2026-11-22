import math
import unittest
from forearm_articulation import forearm_landmarks,forearm_bone_meshes
from spatial_skeleton import spatial_skeleton
from spatial_body import spatial_body,render_meshes
from mesh_diagnostics import inspect_mesh

class ForearmArticulationTests(unittest.TestCase):
    def test_radius_moves_ulna_remains_fixed(self):
        joints=spatial_skeleton()
        a=forearm_landmarks(joints,"left",0)
        b=forearm_landmarks(joints,"left",75)
        self.assertEqual(a["ulna"],b["ulna"])
        self.assertEqual(a["radius"][0],b["radius"][0])
        self.assertNotEqual(a["radius"][1],b["radius"][1])

    def test_bilateral_geometry(self):
        joints=spatial_skeleton({"left_elbow_flex":90,
                                 "right_elbow_flex":65})
        for side in ("left","right"):
            for angle in (-80,0,80):
                for name,mesh in forearm_bone_meshes(joints,side,angle).items():
                    with self.subTest(name=name,angle=angle):
                        self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_diagnostic_only(self):
        meshes,_=spatial_body({"left_forearm_pronation":45})
        self.assertIn("left_radius_bone",meshes)
        self.assertIn("left_ulna_bone",meshes)
        self.assertNotIn("left_radius_bone",render_meshes(meshes,"skin"))
        self.assertIn("left_radius_bone",render_meshes(meshes,"diagnostic"))

    def test_limits(self):
        for angle in (-86,86,float("nan")):
            with self.subTest(angle=angle):
                with self.assertRaises(ValueError):
                    spatial_skeleton({"left_forearm_pronation":angle})
        with self.assertRaises(ValueError):
            forearm_landmarks(spatial_skeleton(),"invalid")

if __name__=="__main__":
    unittest.main()
