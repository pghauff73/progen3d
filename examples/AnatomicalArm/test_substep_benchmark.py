import tempfile
import unittest
from pathlib import Path
from substep_benchmark import measure, benchmark

class SubstepBenchmarkTests(unittest.TestCase):
    def test_measure_real_solver(self):
        result=measure(2,frames=3)
        self.assertEqual(result["substeps"],2)
        self.assertEqual(result["solver_steps"],1+(3-1)*2)
        self.assertGreaterEqual(result["worst_clearance_step"],0)
        self.assertLess(result["worst_clearance_step"],result["solver_steps"])
        self.assertGreaterEqual(result["maximum_edge_strain"],0)
        self.assertGreater(result["elapsed_s"],0)
        self.assertTrue(result["passed"],result)
        self.assertIn("worst_clearance_frame",result)
        self.assertIn("worst_clearance_ring",result)
        self.assertGreaterEqual(result["minimum_elbow_band_gap_m"],result["minimum_clearance_m"]-1e-9)

    def test_outputs(self):
        with tempfile.TemporaryDirectory() as folder:
            report=benchmark(folder,frames=3,substeps=(1,2))
            self.assertEqual(len(report["results"]),2)
            self.assertTrue((Path(folder)/"benchmark.json").exists())
            self.assertTrue((Path(folder)/"benchmark.csv").exists())

    def test_invalid_settings(self):
        with self.assertRaises(ValueError):
            measure(0)

if __name__=="__main__":
    unittest.main()
