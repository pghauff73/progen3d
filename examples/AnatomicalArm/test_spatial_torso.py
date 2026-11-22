import unittest
from spatial_skeleton import spatial_skeleton
from spatial_torso import torso_envelope
from spatial_body import spatial_body,render_meshes
from mesh_diagnostics import inspect_mesh

class SpatialTorsoTests(unittest.TestCase):
    def test_torso_mesh(self):
        joints=spatial_skeleton({"torso_xyz":(20,10,-15),
                                 "pelvis_xyz":(5,-5,10)})
        skin=torso_envelope(joints)
        garment=torso_envelope(joints,clearance=0.006)
        self.assertEqual(len(skin[0]),33*32)
        self.assertEqual(len(skin[1]),32*32*2)
        self.assertTrue(inspect_mesh(*skin)["passed"])
        self.assertNotEqual(skin[0][0],garment[0][0])

    def test_render_layer_excludes_duplicate_limb_geometry(self):
        meshes,_=spatial_body()
        skin=render_meshes(meshes,"skin")
        clothed=render_meshes(meshes,"clothed")
        self.assertEqual(len(skin),5)
        self.assertEqual(len(clothed),5)
        self.assertIn("torso_skin",skin)
        self.assertIn("torso_garment",clothed)
        self.assertNotIn("left_shoulder_elbow_skin",skin)
        self.assertEqual(len(render_meshes(meshes,"diagnostic")),len(meshes))

    def test_invalid_layer(self):
        with self.assertRaises(ValueError):
            render_meshes({},layer="invalid")

if __name__=="__main__":
    unittest.main()
