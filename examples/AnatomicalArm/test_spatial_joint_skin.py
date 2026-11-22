import unittest
from spatial_joint_skin import joint_envelope
from spatial_body import spatial_body
from mesh_diagnostics import inspect_mesh

class SpatialJointSkinTests(unittest.TestCase):
    def test_continuous_joint_surface(self):
        mesh=joint_envelope((0,0,0),(0,-0.3,0),(0.2,-0.45,0.1),
                            0.047,0.037,0.022)
        self.assertEqual(len(mesh[0]),61*20)
        self.assertEqual(len(mesh[1]),60*20*2)
        report=inspect_mesh(*mesh)
        self.assertTrue(report["passed"],report)
        self.assertEqual(report["boundary_edges"],40)

    def test_body_contains_continuous_limbs(self):
        meshes,_=spatial_body({"left_elbow_flex":110,"right_knee_flex":100,
                               "left_shoulder_xyz":(20,15,25)})
        for side in ("left","right"):
            for limb in ("arm","leg"):
                self.assertIn(side+"_"+limb+"_continuous_skin",meshes)

    def test_reversal_rejected(self):
        with self.assertRaises(ValueError):
            joint_envelope((0,0,0),(0,-1,0),(0,0,0),0.1,0.1,0.1)

if __name__=="__main__":
    unittest.main()
