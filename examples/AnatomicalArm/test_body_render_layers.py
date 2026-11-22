import unittest
from whole_body import scene
from body_render_layers import render_subset,check_duplicate_geometry

class BodyRenderLayerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model=scene()

    def test_skin_layer(self):
        selected=render_subset(self.model,"skin")
        self.assertIn("torso_skin",selected)
        self.assertIn("left_shoulder_skin_patch",selected)
        self.assertIn("right_hip_skin_patch",selected)
        self.assertNotIn("left_arm_skin",selected)
        self.assertNotIn("left_shoulder_skin_collar",selected)
        self.assertNotIn("torso_garment",selected)

    def test_clothing_layer(self):
        selected=render_subset(self.model,"clothed")
        self.assertIn("torso_garment",selected)
        self.assertIn("left_shoulder_clothed_patch",selected)
        self.assertNotIn("left_arm_sleeve",selected)
        self.assertNotIn("left_shoulder_garment_collar",selected)

    def test_diagnostic_layer(self):
        self.assertEqual(len(render_subset(self.model,"diagnostic")),len(self.model))

    def test_duplicate_checker(self):
        mesh=([(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])
        self.assertEqual(check_duplicate_geometry({"a":mesh,"b":mesh})["duplicate_triangles"],1)

    def test_invalid_layer(self):
        with self.assertRaises(ValueError):
            render_subset(self.model,"unknown")

if __name__=="__main__":
    unittest.main()
