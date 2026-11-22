import math
import unittest
from junction_bridge import bridge_to_surface,make_bridges
from whole_body import scene

class JunctionBridgeTests(unittest.TestCase):
    def test_bilateral_bridges(self):
        model=scene(shoulder_left=50,hip_right=25,knee_right=95)
        for layer in ("skin","clothed"):
            bridges,reports=make_bridges(model,layer)
            self.assertEqual(len(bridges),4)
            for name,(vertices,faces) in bridges.items():
                with self.subTest(name=name):
                    self.assertEqual(len(vertices),40)
                    self.assertEqual(len(faces),40)
                    self.assertTrue(all(math.isfinite(v) for p in vertices for v in p))
                    self.assertFalse(reports[name]["target_topology_welded"])
                    self.assertGreaterEqual(reports[name]["maximum_bridge_length_m"],0)

    def test_invalid_layer(self):
        with self.assertRaises(ValueError):
            make_bridges(scene(),layer="unknown")

    def test_invalid_mesh(self):
        with self.assertRaises(ValueError):
            bridge_to_surface(([(0,0,0)],[]), ([(0,0,0)],[]))

if __name__=="__main__":
    unittest.main()
