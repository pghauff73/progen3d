import json
import tempfile
import unittest
from pathlib import Path
from scene_interchange import SCHEMA, export_scene

class SceneInterchangeTests(unittest.TestCase):
    def test_scene_schema_and_meshes(self):
        with tempfile.TemporaryDirectory() as directory:
            target=Path(directory)/"scene.json"
            scene=export_scene(target,frames=2,fps=30)
            self.assertEqual(scene["schema"],SCHEMA)
            self.assertEqual(len(scene["frames"]),2)
            self.assertEqual(json.loads(target.read_text())["schema"],SCHEMA)
            for frame in scene["frames"]:
                self.assertEqual(len(frame["objects"]),7)
                self.assertIn("elbow",frame["landmarks_m"])
                for obj in frame["objects"]:
                    n=len(obj["vertices_m"])
                    self.assertTrue(all(0<=v<n for tri in obj["triangles"] for v in tri))

    def test_bad_sampling(self):
        with self.assertRaises(ValueError):
            export_scene("/tmp/invalid.json",frames=0)

if __name__=="__main__":
    unittest.main()
