import unittest
from select_substeps import select

def row(n,ms,strain=0.1,gap=0.0004,passed=True):
    return {"substeps":n,"ms_per_frame":ms,"maximum_edge_strain":strain,
            "minimum_clearance_m":gap,"passed":passed}

class SubstepSelectionTests(unittest.TestCase):
    def test_fastest_eligible(self):
        report={"results":[row(1,2,strain=0.4),row(2,5),row(4,8)]}
        result=select(report)
        self.assertTrue(result["passed"])
        self.assertEqual(result["recommended_substeps"],2)

    def test_no_eligible_configuration(self):
        result=select({"results":[row(1,2,gap=0)]})
        self.assertFalse(result["passed"])
        self.assertIsNone(result["recommended_substeps"])

    def test_nonfinite_measurements_rejected(self):
        with self.assertRaises(ValueError):
            select({"results":[row(1,float("nan"))]})

    def test_duplicate_substeps_rejected(self):
        with self.assertRaises(ValueError):
            select({"results":[row(2,3),row(2,4)]})

    def test_invalid_thresholds_rejected(self):
        for limits in ((-1,0.0003,33.333),(0.25,-1,33.333),
                       (0.25,0.0003,0),(0.25,0.0003,float("nan"))):
            with self.subTest(limits=limits):
                with self.assertRaises(ValueError):
                    select({"results":[row(1,1)]},*limits)

    def test_failed_run_rejected(self):
        result=select({"results":[row(1,1,passed=False),row(2,5)]})
        self.assertEqual(result["recommended_substeps"],2)

if __name__=="__main__":
    unittest.main()
