"""Render the four matched native shell-study equilibria with Blender CPU Cycles.

blender -b --python examples/FacialSkin/render_shell_study.py -- \
    --study build/facial-shell-study --output build/facial-shell-study/Facial_Shell_Study.png
"""
import argparse
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from face_study import NativeSceneObjReader
from render_study import FacialStudyRenderScene


class FacialShellStudyRenderScene(FacialStudyRenderScene):
    """A matched comparison of native geometry, with measured force evidence."""

    def add_equilibrium_panel(self, study, identifier, reference_id, title, x, y):
        path = study / "samples" / (identifier + ".obj")
        reference_path = study / "samples" / (reference_id + ".obj")
        prefix = "# facial_elastic "
        record = next(json.loads(line[len(prefix):]) for line in path.read_text().splitlines()
                      if line.startswith(prefix))
        if not record["converged"] or record["relativeFreeForce"] > 1.0e-4:
            raise ValueError("Cannot present a rejected sample as a shell equilibrium")
        native = NativeSceneObjReader().read(path)
        reference = NativeSceneObjReader().read(reference_path)
        current_points = native["objects"][0]["positions"]
        rest_points = reference["objects"][0]["positions"]
        if len(current_points) != len(rest_points):
            raise ValueError("A shell panel requires its own material-mesh reference")
        maximum_displacement_mm = 1000 * max(
            math.dist(current_points[index:index + 3], rest_points[index:index + 3])
            for index in range(0, len(current_points), 3))
        existing = set(bpy.data.objects)
        self.add_native_sample(path, x)
        for obj in set(bpy.data.objects) - existing:
            obj.location.y += y
        self.add_text(identifier + "_title", title, (x, y - 0.130, 0.075),
                      0.013, (0.15, 0.12, 0.23), "CENTER")
        caption = (f"Max motion {maximum_displacement_mm:.3f} mm  |  "
                   f"Force residual {record['relativeFreeForce']:.2e}")
        self.add_text(identifier + "_caption", caption, (x, y - 0.147, 0.075),
                      0.0075, (0.16, 0.14, 0.22), "CENTER")
        energy_caption = (f"Bending energy {record['bendingEnergyJ'] * 1.0e6:.3f} microjoule  |  "
                          f"{record['iterations']} iterations")
        self.add_text(identifier + "_energy", energy_caption, (x, y - 0.160, 0.075),
                      0.0069, (0.23, 0.20, 0.28), "CENTER")

    def render(self, study, output, width=2000, samples=72):
        for mesh_mode, y in (("legacy", 0.150), ("quality", -0.135)):
            for model, x in (("membrane", -0.205), ("shell", 0.205)):
                identifier = f"{mesh_mode}_{model}_left"
                title = f"{mesh_mode.capitalize()} mesh / {model}"
                self.add_equilibrium_panel(study, identifier,
                    f"{mesh_mode}_{model}_neutral", title, x, y)
        self.add_text("heading", "ProGen3D / Facial skin mechanics", (-0.397, 0.318, 0.08),
                      0.025, (0.15, 0.12, 0.23))
        self.add_text("subtitle", "Milestone C  |  Left cable activation 0.50  |  Matched material and support  |  True scale",
                      (-0.395, 0.293, 0.08), 0.0082, (0.20, 0.17, 0.25))
        self.add_text("footnote", "Native static equilibria. Synthetic tissue model; the weak-support quality case remains rejected.",
                      (-0.395, -0.335, 0.08), 0.0077, (0.22, 0.19, 0.27))
        self.add_text("precision", "Reported force residuals use the authoritative double-precision state. The viewer records mesh precision separately.",
                      (-0.395, -0.349, 0.08), 0.0066, (0.25, 0.22, 0.30))
        bpy.ops.mesh.primitive_plane_add(size=2, location=(0, 0, -0.09))
        bpy.context.object.data.materials.append(self.material("paper", (0.92, 0.90, 0.87), 0.75))
        self.add_area_light("key", (-0.30, 0.36, 0.52), 2.8, 0.45, (1.0, 0.91, 0.83))
        self.add_area_light("fill", (0.32, 0.15, 0.40), 1.0, 0.50, (0.81, 0.87, 1.0))
        camera = bpy.data.cameras.new("shell_study_camera")
        camera.type = "ORTHO"
        camera.ortho_scale = 0.90
        camera_object = bpy.data.objects.new("shell_study_camera", camera)
        bpy.context.collection.objects.link(camera_object)
        camera_object.location = (0, 0, 1)
        camera_object.rotation_euler = (Vector((0, 0, 0)) - camera_object.location).to_track_quat("-Z", "Y").to_euler()
        scene = bpy.context.scene
        scene.camera = camera_object
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = samples
        scene.cycles.use_denoising = False
        scene.view_settings.exposure = 1.0
        scene.world.color = (0.15, 0.15, 0.15)
        scene.render.resolution_x = width
        scene.render.resolution_y = round(width * 0.82)
        scene.render.resolution_percentage = 100
        scene.render.threads_mode = "FIXED"
        scene.render.threads = 5
        scene.render.image_settings.file_format = "PNG"
        scene.render.filepath = str(output.resolve())
        output.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(output.with_suffix(".blend").resolve()))
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--study", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--width", type=int, default=2000)
    parser.add_argument("--samples", type=int, default=72)
    arguments = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    FacialShellStudyRenderScene().render(arguments.study, arguments.output,
                                        arguments.width, arguments.samples)
