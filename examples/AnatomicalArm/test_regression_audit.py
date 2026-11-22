import unittest
from regression_audit import audit

class RegressionAuditTests(unittest.TestCase):
    def test_repeatable_short_motion(self):
        result=audit(frames=3)
        self.assertTrue(result["deterministic"])
        self.assertTrue(result["passed"],result)
        self.assertGreaterEqual(result["maximum_edge_strain"],0)

    def test_invalid_sampling(self):
        with self.assertRaises(ValueError):
            audit(frames=1)

if __name__=="__main__":
    unittest.main()
