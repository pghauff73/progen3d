"""Unit tests for the Milestone 1 evidence collector (no full simulation needed)."""
import sys
import tempfile
import unittest
from milestone1_gate import execute

class Milestone1GateTests(unittest.TestCase):
    def test_junction_gap_is_advisory_not_proof_of_watertightness(self):
        result=junction_advisory({"passed":False,
                                  "threshold_m":0.015,
                                  "worst_gap":{"gap_m":0.021}})
        self.assertFalse(result["within_provisional_gap_threshold"])
        self.assertFalse(result["is_release_blocking"])
        self.assertEqual(result["worst_gap"]["gap_m"],0.021)

    def test_evidence_summary_does_not_invent_hours(self):
        checks=[
            {"name":"compile","passed":True,"returncode":0,"elapsed_seconds":0.4},
            {"name":"mesh","passed":False,"returncode":2,
             "elapsed_seconds":1.2,"stderr_tail":"invalid winding"}
        ]
        report=summarize(checks)
        self.assertEqual(report["total_checks"],2)
        self.assertEqual(report["passed_checks"],1)
        self.assertEqual(report["failed_checks"][0]["check"],"mesh")
        self.assertIn("invalid winding",report["failed_checks"][0]["error_excerpt"])
        self.assertAlmostEqual(report["measured_process_seconds"],1.6)
        self.assertIsNone(report["engineering_hours_verified"])

    def test_successful_command(self):
        result=execute("success",[sys.executable,"-c","print('verified')"])
        self.assertTrue(result["passed"],result)
        self.assertIn("verified",result["stdout_tail"])
        self.assertGreaterEqual(result["elapsed_seconds"],0)

    def test_failed_command(self):
        result=execute("failure",[sys.executable,"-c","raise SystemExit(7)"])
        self.assertFalse(result["passed"])
        self.assertEqual(result["returncode"],7)

    def test_missing_executable(self):
        result=execute("missing",["/definitely/not/a/real/executable"])
        self.assertFalse(result["passed"])
        self.assertIn("error",result)

    def test_timeout(self):
        result=execute("timeout",[sys.executable,"-c",
                      "import time; time.sleep(2)"],timeout=0.01)
        self.assertFalse(result["passed"])
        self.assertIn("timeout",result["error"])

if __name__=="__main__":
    unittest.main()
