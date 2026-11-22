import tempfile
import unittest
from pathlib import Path
from dependency_audit import audit

class DependencyAuditTests(unittest.TestCase):
    def test_detects_cycle(self):
        with tempfile.TemporaryDirectory() as folder:
            Path(folder,"a.py").write_text("from b import thing\n")
            Path(folder,"b.py").write_text("import a\n")
            result=audit(folder)
            self.assertFalse(result["passed"])
            self.assertEqual(len(result["cycles"]),1)

    def test_acyclic_imports(self):
        with tempfile.TemporaryDirectory() as folder:
            Path(folder,"a.py").write_text("import b\n")
            Path(folder,"b.py").write_text("value=1\n")
            result=audit(folder)
            self.assertTrue(result["passed"])
            self.assertEqual(result["dependencies"]["a"],["b"])

    def test_syntax_error(self):
        with tempfile.TemporaryDirectory() as folder:
            Path(folder,"bad.py").write_text("def invalid(:\n")
            result=audit(folder)
            self.assertFalse(result["passed"])
            self.assertIn("bad",result["syntax_errors"])

if __name__=="__main__":
    unittest.main()
