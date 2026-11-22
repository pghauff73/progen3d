import math
import unittest
from arm_milestone_a import FOREARM, UPPER, landmarks, parts

class ArmMilestoneATests(unittest.TestCase):
    def test_lengths(self):
        for angle in (0,45,90,120,145):
            shoulder,elbow,wrist=landmarks(angle)
            self.assertAlmostEqual(math.dist(shoulder,elbow),UPPER)
            self.assertAlmostEqual(math.dist(elbow,wrist),FOREARM)

    def test_skin_continuous_indexed_surface(self):
        for angle in (0,45,90,120,145):
            vertices,faces=parts(angle)["skin"]
            self.assertEqual(len(vertices),56*20)
            self.assertEqual(len(faces),55*20*2)
            self.assertTrue(all(0 <= i < len(vertices) for face in faces for i in face))
            self.assertTrue(all(math.isfinite(c) for p in vertices for c in p))

    def test_components_present(self):
        self.assertEqual(set(parts(90)),{"skin","humerus","radius","ulna","biceps","triceps"})

    def test_limits(self):
        for angle in (-1,146):
            with self.assertRaises(ValueError):
                landmarks(angle)

if __name__=="__main__":
    unittest.main()
