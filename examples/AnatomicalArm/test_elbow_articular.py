import math
import unittest
from spatial_skeleton import spatial_skeleton
from elbow_articular import annular_patch,elbow_articular_meshes,elbow_landmark_clearance
from spatial_body import spatial_body,render_meshes
from mesh_diagnostics import inspect_mesh

class ElbowArticularTests(unittest.TestCase):
    def test_bilateral_articular_meshes(self):
        joints=spatial_skeleton({"left_elbow_flex":90,"right_elbow_flex":110})
        for side in ("left","right"):
            surfaces=elbow_articular_meshes(joints,side)
            self.assertEqual(len(surfaces),3)
            for name,mesh in surfaces.items():
                with self.subTest(name=name):
                    self.assertEqual(len(mesh[0]),12*28)
                    self.assertEqual(len(mesh[1]),11*28*2)
                    self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_articular_components_follow_elbow(self):
        a=elbow_articular_meshes(spatial_skeleton(),"left")
        b=elbow_articular_meshes(spatial_skeleton({"left_shoulder_xyz":(20,0,0)}),"left")
        self.assertNotEqual(a["left_humeral_trochlea"][0],
                            b["left_humeral_trochlea"][0])

    def test_diagnostic_only(self):
        meshes,_=spatial_body()
        self.assertIn("left_humeral_trochlea",meshes)
        self.assertNotIn("left_humeral_trochlea",render_meshes(meshes,"skin"))
        self.assertIn("left_humeral_trochlea",render_meshes(meshes,"diagnostic"))

    def test_clearance_is_explicitly_not_contact_proof(self):
        report=elbow_landmark_clearance(spatial_skeleton(),"right")
        self.assertGreater(report["radial_ulnar_center_distance_m"],0)
        self.assertFalse(report["contact_verified"])

    def test_invalid_geometry(self):
        with self.assertRaises(ValueError):
            annular_patch((0,0,0),(0,0,0),0.01,0.005,0,math.pi)
        with self.assertRaises(ValueError):
            elbow_articular_meshes(spatial_skeleton(),"invalid")

if __name__=="__main__":
    unittest.main()
