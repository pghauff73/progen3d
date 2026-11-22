import math
import unittest
from whole_body import scene,validate

class WholeBodyTests(unittest.TestCase):
    def test_bilateral_body(self):
        model=scene()
        self.assertTrue(validate(model)["passed"])
        for name in ("torso_skin","torso_garment","left_arm_skin",
                     "right_arm_skin","left_leg_skin","right_leg_skin",
                     "left_femur","right_tibia","left_shoulder_joint"):
            self.assertIn(name,model)

    def test_articulated_pose(self):
        rest=scene()
        moved=scene(shoulder_left=55,shoulder_right=-20,
                    elbow_left=120,elbow_right=65,
                    hip_left=35,hip_right=-15,
                    knee_left=100,knee_right=45,torso_lean=12)
        self.assertTrue(validate(moved)["passed"])
        self.assertNotEqual(rest["left_leg_skin"][0],moved["left_leg_skin"][0])
        self.assertNotEqual(rest["right_arm_skin"][0],moved["right_arm_skin"][0])

    def test_mirror_winding(self):
        model=scene()
        # A mirrored arm must reverse triangle order to preserve surface normals.
        self.assertEqual(model["left_arm_skin"][1][0][0],
                         model["right_arm_skin"][1][0][0])
        self.assertEqual(model["left_arm_skin"][1][0][1],
                         model["right_arm_skin"][1][0][2])

    def test_invalid_limits(self):
        for kwargs in ({"elbow_left":146},{"knee_right":141},
                       {"torso_lean":36},{"hip_left":111},
                       {"shoulder_right":float("nan")}):
            with self.subTest(kwargs=kwargs):
                with self.assertRaises(ValueError):
                    scene(**kwargs)

if __name__=="__main__":
    unittest.main()
