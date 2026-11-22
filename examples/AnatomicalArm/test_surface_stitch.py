import unittest
from whole_body import scene
from surface_stitch import stitch_limb_collar,stitch_report
from mesh_diagnostics import inspect_mesh

class SurfaceStitchTests(unittest.TestCase):
    def test_shared_rim_topology(self):
        model=scene()
        limb=model["left_arm_skin"]
        collar=model["left_shoulder_skin_collar"]
        merged=stitch_limb_collar(limb,collar)
        self.assertEqual(len(merged[0]),len(limb[0])+len(collar[0])-20)
        self.assertEqual(len(merged[1]),len(limb[1])+len(collar[1]))
        report=stitch_report(merged)
        self.assertEqual(report["nonmanifold_edges"],0)
        quality=inspect_mesh(*merged)
        self.assertEqual(quality["winding_errors"],0,quality)
        self.assertFalse(report["watertight"])

    def test_bilateral_joint_patches(self):
        model=scene(shoulder_left=45,elbow_left=95,
                    hip_right=30,knee_right=100)
        for side in ("left","right"):
            for name in ("shoulder_skin_patch","shoulder_clothed_patch",
                         "hip_skin_patch","hip_clothed_patch"):
                with self.subTest(side=side,name=name):
                    report=stitch_report(model[side+"_"+name])
                    self.assertEqual(report["nonmanifold_edges"],0)

    def test_reject_misaligned_ring(self):
        limb=([(0,0,0),(1,0,0),(0,1,0)],[(0,1,2)])
        collar=([(0,0,1),(1,0,1),(0,1,1),
                 (0,0,2),(1,0,2),(0,1,2)],[(0,3,1)])
        with self.assertRaises(ValueError):
            stitch_limb_collar(limb,collar,sides=3)

    def test_reversed_collar_winding_still_matches_limb(self):
        model=scene()
        limb=model["left_arm_skin"]
        vertices,faces=model["left_shoulder_skin_collar"]
        reversed_collar=(vertices,[(a,c,b) for a,b,c in faces])
        merged=stitch_limb_collar(limb,reversed_collar)
        self.assertEqual(inspect_mesh(*merged)["winding_errors"],0)

    def test_reject_mixed_attachment_winding(self):
        model=scene()
        vertices,faces=model["left_shoulder_skin_collar"]
        faces=list(faces)
        a,b,c=faces[0]
        faces[0]=(a,c,b)
        with self.assertRaisesRegex(ValueError,"inconsistent attachment ring winding"):
            stitch_limb_collar(model["left_arm_skin"],(vertices,faces))

if __name__=="__main__":
    unittest.main()
