import unittest
from junction_proximity import proximity,junction_report
from whole_body import scene

class JunctionProximityTests(unittest.TestCase):
    def test_triangle_distance(self):
        tri=([(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])
        points=([(0.2,0.2,0.1)],[])
        result=proximity(points,tri,stride=1)
        self.assertAlmostEqual(result["minimum_distance_m"],0.1)
        self.assertEqual(result["samples"],1)
        self.assertEqual(result["worst_sample_index"],0)

    def test_body_junction_distances(self):
        result=junction_report(scene(shoulder_left=45,hip_right=30))
        self.assertEqual(set(result),{"left_shoulder","right_shoulder",
                                      "left_hip","right_hip"})
        for item in result.values():
            self.assertEqual(item["samples"],20)
            self.assertGreaterEqual(item["worst_sample_index"],0)
            self.assertLess(item["worst_sample_index"],20)
            self.assertGreaterEqual(item["minimum_distance_m"],0)
            self.assertGreaterEqual(item["maximum_nearest_distance_m"],
                                    item["minimum_distance_m"])

    def test_invalid_stride(self):
        tri=([(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])
        with self.assertRaises(ValueError):
            proximity(tri,tri,stride=0)

if __name__=="__main__":
    unittest.main()
