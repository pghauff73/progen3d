import unittest
from mesh_diagnostics import inspect_mesh, inspect_assembly
from arm_soft_sleeve import assembly

class MeshDiagnosticsTests(unittest.TestCase):
    def test_valid_open_square(self):
        vertices=[(0,0,0),(1,0,0),(1,1,0),(0,1,0)]
        result=inspect_mesh(vertices,[(0,1,2),(0,2,3)])
        self.assertTrue(result["passed"],result)
        self.assertEqual(result["boundary_edges"],4)

    def test_reversed_shared_edge(self):
        vertices=[(0,0,0),(1,0,0),(1,1,0),(0,1,0)]
        result=inspect_mesh(vertices,[(0,1,2),(0,3,2)])
        self.assertFalse(result["passed"])
        self.assertEqual(result["winding_errors"],1)

    def test_degenerate(self):
        result=inspect_mesh([(0,0,0),(1,0,0),(2,0,0)],[(0,1,2)])
        self.assertFalse(result["passed"])

    def test_arm_pose_quality(self):
        for angle in (0,30,60,90,120,145):
            with self.subTest(angle=angle):
                result=inspect_assembly(assembly(angle))
                self.assertTrue(result["passed"],result)

if __name__=="__main__":
    unittest.main()
