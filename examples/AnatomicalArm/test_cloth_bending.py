import math
import unittest
from cloth_bending import opposite_vertex_bends, solve_distance_constraints

class BendingTests(unittest.TestCase):
    def test_shared_edge(self):
        self.assertEqual(opposite_vertex_bends([(0,1,2),(0,2,3)]),[(1,3)])

    def test_convergence(self):
        vertices=[[0.,0.,0.],[1.,0.,0.],[0.,1.,0.],[1.,1.,1.]]
        rest=math.dist(vertices[1],vertices[3])
        vertices[3][2]+=1
        initial=abs(math.dist(vertices[1],vertices[3])-rest)
        solve_distance_constraints(vertices,[(1,3)],[rest],[0,1,0,1],0,1/60,20)
        self.assertLess(abs(math.dist(vertices[1],vertices[3])-rest),initial)

    def test_invalid_dt(self):
        with self.assertRaises(ValueError):
            solve_distance_constraints([[0,0,0],[1,0,0]],[(0,1)],[1],[1,1],0,0)

if __name__=="__main__":
    unittest.main()
