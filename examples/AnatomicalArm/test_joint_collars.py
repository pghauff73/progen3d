import unittest
from whole_body import scene,validate
from joint_collars import rim_error,collar_from_ring

class JointCollarTests(unittest.TestCase):
    def test_rims_match_limb(self):
        model=scene(shoulder_left=45,shoulder_right=-20,
                    elbow_left=100,hip_left=35,knee_left=90)
        for side in ("left","right"):
            for limb,collar in (
                ("arm_skin","shoulder_skin_collar"),
                ("arm_sleeve","shoulder_garment_collar"),
                ("leg_skin","hip_skin_collar"),
                ("leg_garment","hip_garment_collar")):
                with self.subTest(side=side,collar=collar):
                    self.assertLess(rim_error(model[side+"_"+collar],
                                              model[side+"_"+limb]),1e-12)
        self.assertTrue(validate(model)["passed"])

    def test_invalid_scale(self):
        with self.assertRaises(ValueError):
            collar_from_ring(([(0,0,0)]*20,[]),(0,0,0),radius_scale=0)

if __name__=="__main__":
    unittest.main()
