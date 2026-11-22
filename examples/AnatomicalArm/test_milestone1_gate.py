"""Unit tests for the Milestone 1 evidence collector (no full simulation needed)."""
import sys
import tempfile
import unittest
from milestone1_gate import execute

class Milestone1GateTests(unittest.TestCase):
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
