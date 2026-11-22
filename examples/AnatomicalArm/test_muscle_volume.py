import math
import unittest
from muscle_volume import volume_scale,volume_corrected_spindle,corrected_shoulders
from spatial_skeleton import spatial_skeleton
from mesh_diagnostics import inspect_mesh

class MuscleVolumeTests(unittest.TestCase):
    def test_unclamped_volume_conservation(self):
        for current in (0.7,0.9,1.0,1.2,1.5):
            factor=volume_scale(1.0,current)
            self.assertAlmostEqual(current*factor*factor,1.0,places=10)

    def test_clamped_shortening(self):
        self.assertEqual(volume_scale(1.0,0.01),1.55)

    def test_bilateral_corrected_muscles(self):
        neutral=spatial_skeleton()
        posed=spatial_skeleton({"left_arm_elevation":140,
                                "left_shoulder_xyz":(20,10,35)})
        meshes,metadata=corrected_shoulders(posed,neutral)
        self.assertEqual(len(meshes),6)
        self.assertEqual(set(meshes),set(metadata))
        for name,mesh in meshes.items():
            with self.subTest(name=name):
                self.assertTrue(inspect_mesh(*mesh)["passed"])

    def test_invalid_lengths(self):
        with self.assertRaises(ValueError):
            volume_scale(0,1)
        with self.assertRaises(ValueError):
            volume_corrected_spindle((0,0,0),(0,0,0),0.2)

if __name__=="__main__":
    unittest.main()
