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

    def test_knee_mesh_continuity(self):
        from whole_body import leg_mesh
        for knee_angle in (0,30,60,90,120,140):
            a=(0.0,0.0,0.0)
            b=(0.0,-0.43,0.0)
            rad=math.radians(knee_angle)
            c=(0.43*math.sin(rad),-0.43-0.43*math.cos(rad),0.0)
            verts,faces=leg_mesh(a,b,c)
            self.assertEqual(len(verts),61*20)
            self.assertEqual(len(faces),60*20*2)
            self.assertTrue(all(math.isfinite(v) for p in verts for v in p))
            self.assertTrue(all(0<=i<len(verts) for f in faces for i in f))

    def test_invalid_limits(self):
        for kwargs in ({"elbow_left":146},{"knee_right":141},
                       {"torso_lean":36},{"hip_left":111},
                       {"shoulder_right":float("nan")}):
            with self.subTest(kwargs=kwargs):
                with self.assertRaises(ValueError):
                    scene(**kwargs)

if __name__=="__main__":
    unittest.main()
