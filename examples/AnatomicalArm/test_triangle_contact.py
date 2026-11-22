import math
import unittest
from triangle_contact import TriangleSurface, closest_triangle, length, sub

class TriangleContactTests(unittest.TestCase):
    def setUp(self):
        self.surface=TriangleSurface(
            [(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])

    def test_interior_closest_point(self):
        p=closest_triangle((0.2,0.2,1),(0,0,0),(1,0,0),(0,1,0))
        self.assertAlmostEqual(p[0],0.2)
        self.assertAlmostEqual(p[1],0.2)
        self.assertAlmostEqual(p[2],0)

    def test_edge_and_vertex_regions(self):
        self.assertEqual(closest_triangle((-1,-1,0),(0,0,0),(1,0,0),(0,1,0)),(0,0,0))
        q=closest_triangle((0.7,0.7,0),(0,0,0),(1,0,0),(0,1,0))
        self.assertAlmostEqual(q[0]+q[1],1)

    def test_surface_clearance(self):
        q=self.surface.project((0.2,0.2,0.0001),0.002)
        self.assertAlmostEqual(q[2],0.002)
        self.assertEqual(self.surface.project((0.2,0.2,0.1),0.002),(0.2,0.2,0.1))

    def test_degenerate_faces(self):
        with self.assertRaises(ValueError):
            TriangleSurface([(0,0,0),(1,0,0),(2,0,0)],[(0,1,2)])

    def test_invalid_clearance(self):
        with self.assertRaises(ValueError):
            self.surface.project((0,0,0),-1)

if __name__=="__main__":
    unittest.main()
