import math
import unittest
from xpbd_cloth import XPBDCloth, edges_from_faces, sphere_projector, distance

class XPBDTests(unittest.TestCase):
    def test_edges_unique(self):
        self.assertEqual(len(edges_from_faces([(0,1,2),(0,2,3)])),5)

    def test_pinned_vertices(self):
        v=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)]
        cloth=XPBDCloth(v,[(0,1,2)],pinned=(0,))
        for _ in range(12):
            cloth.step(1/120)
        self.assertEqual(tuple(cloth.positions[0]),v[0])
        self.assertTrue(all(math.isfinite(x) for p in cloth.positions for x in p))

    def test_constraint_convergence(self):
        v=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)]
        cloth=XPBDCloth(v,[(0,1,2)],compliance=0,pinned=(0,))
        cloth.positions[1][0]=1.5
        initial=abs(distance(cloth.positions[0],cloth.positions[1])-1)
        cloth.step(1/60,iterations=30,gravity=(0,0,0))
        final=abs(distance(cloth.positions[0],cloth.positions[1])-1)
        self.assertLess(final,initial)

    def test_sphere_projection(self):
        projector=sphere_projector((0,0,0),2)
        self.assertAlmostEqual(distance(projector((0.1,0,0)),(0,0,0)),2)
        self.assertEqual(projector((3,0,0)),(3,0,0))

    def test_invalid_parameters(self):
        with self.assertRaises(ValueError):
            XPBDCloth([(0,0,0)],[],mass=0)
        with self.assertRaises(ValueError):
            XPBDCloth([(0,0,0)],[]).step(0)

if __name__=="__main__":
    unittest.main()
