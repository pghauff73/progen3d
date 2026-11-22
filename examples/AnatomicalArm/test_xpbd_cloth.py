import math
import unittest
from xpbd_cloth import XPBDCloth, edges_from_faces, sphere_projector, distance

class XPBDTests(unittest.TestCase):
    def test_edges_unique(self):
        self.assertEqual(len(edges_from_faces([(0,1,2),(0,2,3)])),5)

    def test_free_particle_keeps_constant_velocity_without_load(self):
        origin=(0.3,-0.2,0.1)
        velocity=(0.12,-0.08,0.03)
        dt=1/120
        cloth=XPBDCloth([origin],[],damping=1.0)
        cloth.previous[0]=[origin[k]-velocity[k]*dt for k in range(3)]
        for step in range(1,33):
            cloth.step(dt,gravity=(0,0,0))
            for axis in range(3):
                self.assertAlmostEqual(cloth.positions[0][axis],
                                       origin[axis]+velocity[axis]*dt*step,places=12)

    def test_zero_load_damping_matches_geometric_velocity_decay(self):
        dt=1/120
        initial_velocity=0.24
        for damping in (0.0,0.8):
            with self.subTest(damping=damping):
                cloth=XPBDCloth([(0.,0.,0.)],[],damping=damping)
                cloth.previous[0][0]=-initial_velocity*dt
                previous_speed=initial_velocity
                for step in range(1,25):
                    previous_position=cloth.positions[0][0]
                    cloth.step(dt,gravity=(0,0,0))
                    speed=(cloth.positions[0][0]-previous_position)/dt
                    self.assertAlmostEqual(speed,initial_velocity*damping**step,places=12)
                    self.assertLessEqual(abs(speed),previous_speed+1e-12)
                    previous_speed=abs(speed)
                expected_distance=initial_velocity*dt*damping*(1-damping**24)/(1-damping)
                self.assertAlmostEqual(cloth.positions[0][0],expected_distance,places=12)

    def test_rest_state_does_not_move_without_load(self):
        vertices=[(0.,0.,0.),(1.,0.,0.),(0.,1.,0.)]
        cloth=XPBDCloth(vertices,[(0,1,2)],damping=1.0)
        for _ in range(24):
            cloth.step(1/120,gravity=(0,0,0))
        self.assertEqual(cloth.positions,[list(vertex) for vertex in vertices])
        self.assertEqual(cloth.previous,cloth.positions)

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
