import math
import unittest
from spatial_skeleton import (euler_xyz,rotate,SpatialTransform,
                              spatial_skeleton,length_errors)

class SpatialSkeletonTests(unittest.TestCase):
    def test_identity_rotation(self):
        p=(0.1,-0.2,0.3)
        for a,b in zip(rotate(euler_xyz(),p),p):
            self.assertAlmostEqual(a,b,places=12)

    def test_right_angle_rotation(self):
        x,y,z=rotate(euler_xyz(rz=90),(1,0,0))
        self.assertAlmostEqual(x,0,places=12)
        self.assertAlmostEqual(y,1,places=12)
        self.assertAlmostEqual(z,0,places=12)

    def test_segment_lengths_with_3d_pose(self):
        pose={"torso_xyz":(15,-12,20),"pelvis_xyz":(-8,5,10),
              "left_shoulder_xyz":(30,20,50),
              "right_shoulder_xyz":(-25,15,-40),
              "left_elbow_flex":135,"right_elbow_flex":80,
              "left_hip_xyz":(20,10,25),"right_hip_xyz":(-20,5,-25),
              "left_knee_flex":120,"right_knee_flex":95}
        joints=spatial_skeleton(pose)
        self.assertLess(max(length_errors(joints).values()),1e-10)

    def test_parent_rotation_moves_child(self):
        a=spatial_skeleton()
        b=spatial_skeleton({"torso_xyz":(20,0,0)})
        self.assertNotEqual(a["left_shoulder"].origin,b["left_shoulder"].origin)
        self.assertEqual(a["left_hip"].origin,b["left_hip"].origin)

    def test_scapular_coupling_preserves_bone_lengths(self):
        neutral=spatial_skeleton()
        raised=spatial_skeleton({"left_arm_elevation":150,
                                 "right_arm_elevation":90})
        self.assertIn("left_scapula",raised)
        self.assertIn("right_scapula",raised)
        self.assertNotEqual(neutral["left_shoulder"].origin,
                            raised["left_shoulder"].origin)
        self.assertLess(max(length_errors(raised).values()),1e-10)

    def test_scapula_neutral_under_30_degrees(self):
        a=spatial_skeleton({"left_arm_elevation":0})
        b=spatial_skeleton({"left_arm_elevation":25})
        self.assertEqual(a["left_scapula"],b["left_scapula"])

    def test_invalid_pose(self):
        for pose in ({"left_knee_flex":145},{"right_elbow_flex":-1},
                     {"torso_xyz":(0,float("nan"),0)},
                     {"left_shoulder_xyz":(0,0,120)},
                     {"not_a_joint":10}):
            with self.subTest(pose=pose):
                with self.assertRaises(ValueError):
                    spatial_skeleton(pose)

if __name__=="__main__":
    unittest.main()
