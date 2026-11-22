import math
import unittest
from spatial_body import spatial_body
from shoulder_skin_corrective import shoulder_corrective
from mesh_diagnostics import inspect_mesh

class ShoulderSkinCorrectiveTests(unittest.TestCase):
    def test_neutral_is_unchanged(self):
        base,_=spatial_body()
        mesh=base["left_arm_continuous_skin"]
        corrected=shoulder_corrective(mesh,(0,0,0),(0,-0.3,0),elevation=0)
        self.assertEqual(corrected,mesh)

    def test_elevation_changes_proximal_skin(self):
        neutral,_=spatial_body()
        raised,_=spatial_body({"left_arm_elevation":150})
        a=neutral["left_arm_continuous_skin"]
        b=raised["left_arm_continuous_skin"]
        self.assertEqual(len(a[0]),len(b[0]))
        self.assertEqual(a[1],b[1])
        self.assertNotEqual(a[0],b[0])
        self.assertTrue(inspect_mesh(*b)["passed"])

    def test_invalid_pose(self):
        mesh=([(0,0,0)]*(61*20),[])
        with self.assertRaises(ValueError):
            shoulder_corrective(mesh,(0,0,0),(0,-0.3,0),elevation=181)

if __name__=="__main__":
    unittest.main()
