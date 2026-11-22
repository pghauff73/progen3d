import unittest
from body_cycle_audit import audit,cycle_pose

class BodyCycleAuditTests(unittest.TestCase):
    def test_periodic_pose(self):
        for joint,value in cycle_pose(0).items():
            self.assertAlmostEqual(value,cycle_pose(1)[joint],places=10)
        self.assertAlmostEqual(cycle_pose(0.5)["elbow_left"],145)
        self.assertAlmostEqual(cycle_pose(0.5)["knee_left"],140)

    def test_short_cycle_skin(self):
        result=audit(frames=3,layer="skin")
        self.assertTrue(result["passed"],result["records"])
        self.assertLess(result["max_segment_length_error_m"],1e-9)

    def test_short_cycle_clothed(self):
        result=audit(frames=3,layer="clothed")
        self.assertTrue(result["passed"],result["records"])

    def test_invalid_frames(self):
        with self.assertRaises(ValueError):
            audit(frames=2)

if __name__=="__main__":
    unittest.main()
