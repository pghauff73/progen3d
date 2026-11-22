import unittest
from junction_sweep import sweep

class JunctionSweepTests(unittest.TestCase):
    def test_three_pose_sweep(self):
        result=sweep(steps=3,threshold_m=0.015)
        self.assertEqual(len(result["records"]),12)
        self.assertIn(result["worst_gap"]["junction"],
                      ("left_shoulder","right_shoulder","left_hip","right_hip"))
        self.assertGreaterEqual(result["worst_gap"]["gap_m"],0)
        self.assertTrue(all(0<=r["worst_rim_vertex"]<20 for r in result["records"]))

    def test_invalid_steps(self):
        for steps in (0,2,True,3.5):
            with self.subTest(steps=steps):
                with self.assertRaises(ValueError):
                    sweep(steps=steps)

    def test_invalid_threshold(self):
        with self.assertRaises(ValueError):
            sweep(steps=3,threshold_m=float("nan"))

if __name__=="__main__":
    unittest.main()
