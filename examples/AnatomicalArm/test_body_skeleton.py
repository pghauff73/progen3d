import math
import unittest
from body_skeleton import skeleton,segment_lengths
from whole_body import scene,validate

class BodySkeletonTests(unittest.TestCase):
    def test_segment_lengths_invariant(self):
        for lean,tilt,hip,knee in ((0,0,0,0),(15,10,35,95),(-20,-15,-40,140)):
            with self.subTest(lean=lean,tilt=tilt):
                joints=skeleton(torso_lean=lean,pelvis_tilt=tilt,
                                hip_left=hip,hip_right=-hip,
                                knee_left=knee,knee_right=knee)
                for thigh,shin in segment_lengths(joints).values():
                    self.assertAlmostEqual(thigh,0.43,places=9)
                    self.assertAlmostEqual(shin,0.43,places=9)

    def test_torso_moves_shoulders(self):
        a=skeleton()
        b=skeleton(torso_lean=25)
        self.assertNotEqual(a["left_shoulder"].origin,b["left_shoulder"].origin)
        self.assertEqual(a["left_hip"].origin,b["left_hip"].origin)

    def test_pelvis_moves_hips(self):
        a=skeleton()
        b=skeleton(pelvis_tilt=20)
        self.assertNotEqual(a["left_hip"].origin,b["left_hip"].origin)

    def test_body_articulated(self):
        model=scene(torso_lean=20,pelvis_tilt=12,
                    shoulder_left=35,hip_left=25,knee_left=90)
        self.assertTrue(validate(model)["passed"])

    def test_invalid_joint_values(self):
        for kwargs in ({"torso_lean":40},{"pelvis_tilt":30},
                       {"knee_left":float("nan")}):
            with self.assertRaises(ValueError):
                skeleton(**kwargs)

if __name__=="__main__":
    unittest.main()
