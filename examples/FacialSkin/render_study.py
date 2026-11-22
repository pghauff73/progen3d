"""Render an annotated native facial study with Blender's CPU Cycles backend.

blender -b --python examples/FacialSkin/render_study.py -- \
    --study build/facial-skin-study --output build/facial-skin-study/Facial_Study.png
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


class FacialStudyRenderScene:
    def __init__(self):
        bpy.ops.object.select_all(action="SELECT")
        bpy.ops.object.delete(use_global=False)
        self.materials = {}

    def material(self, name, color, roughness=0.52):
        if name in self.materials:
            return self.materials[name]
        material = bpy.data.materials.new(name)
        material.use_nodes = True
        shader = material.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Base Color"].default_value = (*color, 1.0)
        shader.inputs["Roughness"].default_value = roughness
        if name.endswith("_ink"):
            nodes = material.node_tree.nodes
            nodes.remove(shader)
            emission = nodes.new("ShaderNodeEmission")
            emission.inputs["Color"].default_value = (*[v*v for v in color], 1.0)
            material.node_tree.links.new(emission.outputs[0], nodes.get("Material Output").inputs["Surface"])
        self.materials[name] = material
        return material

    def add_native_sample(self, path, x_offset):
        native = NativeSceneObjReader().read(path)
        for data in native["objects"]:
            positions = data["positions"]
            points = [positions[i:i+3] for i in range(0, len(positions), 3)]
            normals = [data["normals"][i:i+3] for i in range(0, len(data["normals"]), 3)]
            faces = [(i, i+1, i+2) for i in range(0, len(points), 3)]
            mesh = bpy.data.meshes.new(path.stem+"_"+data["name"])
            mesh.from_pydata(points, [], faces)
            mesh.update()
            for polygon in mesh.polygons:
                polygon.use_smooth = True
            if hasattr(mesh, "use_auto_smooth"):
                mesh.use_auto_smooth = True
            mesh.normals_split_custom_set(normals)
            obj = bpy.data.objects.new(mesh.name, mesh)
            bpy.context.collection.objects.link(obj)
            obj.location.x = x_offset
            obj.data.materials.append(self.material(data["material"], data["color"]))

    def add_text(self, name, content, position, size, color, align="LEFT"):
        curve = bpy.data.curves.new(name, "FONT")
        curve.body = content
        curve.size = size
        curve.align_x = align
        obj = bpy.data.objects.new(name, curve)
        bpy.context.collection.objects.link(obj)
        obj.location = position
        obj.data.materials.append(self.material(name+"_ink", color))

    def add_area_light(self, name, position, energy, size, color):
        light = bpy.data.lights.new(name, "AREA")
        light.energy = energy
        light.shape = "DISK"
        light.size = size
        light.color = color
        obj = bpy.data.objects.new(name, light)
        bpy.context.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (-obj.location).to_track_quat("-Z", "Y").to_euler()

    def render(self, study, output, width=1800, samples=96, elastic=False):
        panels = (
            (-0.23, "neutral", "01  NEUTRAL", "Connected skin and open boundaries"),
            (0.0, "smile", "02  SMILE", "Paired cheek and mouth-corner fields"),
            (0.23, "combined", "03  COMBINED", "Smile + jaw opening + blink"),
        )
        heading = "ProGen3D  /  Facial skin"
        subtitle = "Native grammar milestone A  |  Procedural geometry  |  Independent expression controls"
        footnote = "Actual native samples. Open front-face prototype; elastic tissue and full Lyra integration remain future work."
        if elastic:
            reference = NativeSceneObjReader().read(study/"samples"/"neutral.obj")
            panels = []
            for offset, filename, title, activation in (
                (-0.23, "neutral", "01  NEUTRAL", 0.0),
                (0.0, "smile_left", "02  LEFT CABLE", 0.5),
                (0.23, "smile", "03  BOTH CABLES", 0.5),
            ):
                path = study/"samples"/(filename+".obj")
                native = NativeSceneObjReader().read(path)
                prefix = "# facial_elastic "
                record = next(json.loads(line[len(prefix):]) for line in path.read_text().splitlines()
                              if line.startswith(prefix))
                if not record["converged"]:
                    raise ValueError("Cannot render an unconverged elastic sample")
                displacement = 1000*math.dist(native["landmarks"]["mouth_left_corner"],
                                             reference["landmarks"]["mouth_left_corner"])
                caption = f"Activation {activation:.2f}  |  Left corner {displacement:.2f} mm"
                panels.append((offset, filename, title, caption))
            heading = "ProGen3D  /  Elastic facial skin"
            subtitle = "Native grammar milestone B  |  Supported membrane + tensile cables  |  Neutral material reference"
            footnote = "Native equilibria at true scale. Synthetic material and cable paths; no bending, dynamics or lip-contact solve."
        for x, filename, title, caption in panels:
            self.add_native_sample(study/"samples"/(filename+".obj"), x)
            self.add_text(title, title, (x, -0.141, 0.075), 0.012, (0.15, 0.12, 0.23), "CENTER")
            caption_color = (0.16, 0.14, 0.22) if elastic else (0.30, 0.28, 0.35)
            self.add_text(title+"_caption", caption, (x, -0.158, 0.075), 0.0075, caption_color, "CENTER")
        self.add_text("heading", heading, (-0.355, 0.170, 0.08), 0.025, (0.15, 0.12, 0.23))
        self.add_text("subtitle", subtitle,
                      (-0.354, 0.147, 0.08), 0.0088,
                      (0.20, 0.17, 0.25) if elastic else (0.32, 0.29, 0.37))
        self.add_text("footnote", footnote,
                      (-0.354, -0.188, 0.08), 0.0077,
                      (0.22, 0.19, 0.27) if elastic else (0.35, 0.32, 0.40))
        bpy.ops.mesh.primitive_plane_add(size=2, location=(0, 0, -0.09))
        bpy.context.object.data.materials.append(self.material("paper", (0.92, 0.90, 0.87), 0.75))
        self.add_area_light("key", (-0.26, 0.30, 0.48), 2.2, 0.38, (1.0, 0.91, 0.83))
        self.add_area_light("fill", (0.30, 0.15, 0.36), 0.8, 0.45, (0.81, 0.87, 1.0))
        camera = bpy.data.cameras.new("study_camera")
        camera.type = "ORTHO"
        camera.ortho_scale = 0.79
        obj = bpy.data.objects.new("study_camera", camera)
        bpy.context.collection.objects.link(obj)
        obj.location = (0, 0, 1.0)
        obj.rotation_euler = (Vector((0, 0, 0))-obj.location).to_track_quat("-Z", "Y").to_euler()
        scene = bpy.context.scene
        scene.camera = obj
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = samples
        scene.cycles.use_denoising = elastic
        if elastic:
            scene.view_settings.exposure = 1.0
        scene.render.resolution_x = width
        scene.render.resolution_y = round(width*960/1800)
        scene.render.resolution_percentage = 100
        scene.world.color = (0.15, 0.15, 0.15)
        scene.render.image_settings.file_format = "PNG"
        scene.render.filepath = str(output.resolve())
        scene.render.threads_mode = "FIXED"
        scene.render.threads = 5
        output.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(output.with_suffix(".blend").resolve()))
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--study", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--width", type=int, default=1800)
    parser.add_argument("--samples", type=int, default=96)
    parser.add_argument("--elastic", action="store_true", help="Render the native elastic comparison")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    FacialStudyRenderScene().render(args.study, args.output, args.width, args.samples, args.elastic)
