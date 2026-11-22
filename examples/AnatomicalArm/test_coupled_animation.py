import tempfile
import unittest
from pathlib import Path
from coupled_animation import CoupledArm, angle_at, simulate

class CoupledArmTests(unittest.TestCase):
    def test_angle_schedule(self):
        self.assertAlmostEqual(angle_at(0),0)
        self.assertAlmostEqual(angle_at(1),145)
        self.assertAlmostEqual(angle_at(2),0)

    def test_free_sleeve_keeps_axial_velocity_in_stationary_pose(self):
        # At zero flexion, translation along Y preserves every edge/bend
        # length and the radial contact gap. No constraint should accelerate it.
        arm=CoupledArm()
        arm.cloth.inverse_mass=[100.0]*len(arm.cloth.positions)
        arm.cloth.damping=1.0
        origin=[position[:] for position in arm.cloth.positions]
        dt=1/120
        velocity=0.03
        for previous in arm.cloth.previous:
            previous[1]-=velocity*dt
        for step in range(1,7):
            result=arm.step(0,dt=dt,iterations=2)
            self.assertTrue(result["passed"],result)
            for initial,position in zip(origin,arm.cloth.positions):
                self.assertAlmostEqual(position[1],initial[1]+velocity*dt*step,places=10)

    def test_free_sleeve_damps_axial_motion_without_energy_growth(self):
        arm=CoupledArm()
        arm.cloth.inverse_mass=[100.0]*len(arm.cloth.positions)
        arm.cloth.damping=0.8
        dt=1/120
        initial_velocity=0.03
        initial_y=arm.cloth.positions[-1][1]
        for previous in arm.cloth.previous:
            previous[1]-=initial_velocity*dt
        previous_speed=initial_velocity
        for step in range(1,9):
            previous_position=arm.cloth.positions[-1][1]
            arm.step(0,dt=dt,iterations=2)
            speed=(arm.cloth.positions[-1][1]-previous_position)/dt
            self.assertAlmostEqual(speed,initial_velocity*0.8**step,places=10)
            self.assertLessEqual(abs(speed),previous_speed+1e-10)
            previous_speed=abs(speed)
        expected_distance=initial_velocity*dt*0.8*(1-0.8**8)/(1-0.8)
        self.assertAlmostEqual(arm.cloth.positions[-1][1]-initial_y,expected_distance,places=10)

    def test_stationary_unloaded_sleeve_has_no_drift(self):
        arm=CoupledArm()
        origin=[position[:] for position in arm.cloth.positions]
        for _ in range(6):
            arm.step(0,iterations=2)
        for initial,position in zip(origin,arm.cloth.positions):
            for expected,actual in zip(initial,position):
                self.assertAlmostEqual(actual,expected,places=12)

    def test_moving_contact(self):
        arm=CoupledArm()
        for angle in (0,30,60,90,120,145,120,60,0):
            with self.subTest(angle=angle):
                result=arm.step(angle,dt=1/60,iterations=8)
                self.assertTrue(result["passed"],result)
                self.assertGreaterEqual(result["maximum_edge_strain"],0)
                self.assertGreaterEqual(result["mean_edge_strain"],0)
                self.assertIsInstance(result["worst_clearance_ring"],int)
                self.assertGreaterEqual(result["worst_clearance_ring"],1)
                self.assertIsNotNone(result["minimum_elbow_band_gap_m"])
                self.assertGreaterEqual(result["minimum_elbow_band_gap_m"],result["minimum_radial_gap_m"]-1e-9)

    def test_export(self):
        with tempfile.TemporaryDirectory() as directory:
            result=simulate(directory,frames=3,fps=30)
            self.assertTrue(result["passed"],result["records"])
            self.assertTrue((Path(directory)/"simulation.json").exists())
            self.assertTrue((Path(directory)/"frame_0002.obj").exists())

    def test_triangle_contact_mode(self):
        arm=CoupledArm(contact_mode='triangle',friction=0.15)
        result=arm.step(15,dt=1/60,iterations=2)
        self.assertEqual(result['contact_mode'],'triangle')
        self.assertTrue(result['passed'],result)

    def test_invalid_contact_parameters(self):
        with self.assertRaises(ValueError):
            CoupledArm(contact_mode='unsupported')
        with self.assertRaises(ValueError):
            CoupledArm(friction=1.1)

    def test_substep_animation_export(self):
        with tempfile.TemporaryDirectory() as directory:
            result=simulate(directory,frames=3,fps=30,substeps=4)
            self.assertEqual(result["substeps"],4)
            self.assertEqual(result["solver_steps"],9)
            self.assertEqual(len(result["step_records"]),9)
            self.assertGreaterEqual(result["worst_clearance_step"],0)
            self.assertLess(result["worst_clearance_step"],9)
            self.assertLessEqual(result["minimum_clearance_m"],min(r["minimum_radial_gap_m"] for r in result["records"])+1e-9)
            self.assertEqual(len(result["records"]),3)
            self.assertAlmostEqual(result["records"][-1]["angle"],angle_at(2/30))
            self.assertTrue(result["passed"],result["records"])

    def test_invalid_substeps(self):
        with tempfile.TemporaryDirectory() as directory:
            for value in (0,-1,33,1.5):
                with self.subTest(value=value):
                    with self.assertRaises(ValueError):
                        simulate(directory,frames=2,substeps=value)

    def test_invalid_angle(self):
        with self.assertRaises(ValueError):
            CoupledArm().step(150)

if __name__=="__main__":
    unittest.main()
