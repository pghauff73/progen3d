"""Render matched ordinary/hologram views from one native Lyra OBJ.

blender -b --python examples/LyraFacialIntegration/render_study.py -- \\
    --obj build/lyra-facial-integration/samples/neutral.obj --output build/lyra-renders/neutral
This is an external Blender inspection profile, not native-viewport
shader parity. No displacement, remeshing, subdivision, or face editing occurs.
"""
import argparse
from array import array
import copy
import hashlib
import json
import math
from pathlib import Path
import sys
import time

import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from native_presentation import NativeLyraObjReader, NativePlaneSectionService
from geometry_validation import NativeFrameTransform


def native_face_local_landmarks(document):
    """Remove the actual rigid instance transform before comparing expressions."""
    if not document.landmarks:
        return {}
    faces = [record for record in document.primitive_metadata
             if record.get("source", {}).get("rule") == "LyraFacialSkin"]
    if len(faces) != 1:
        raise ValueError("Expression measurement requires one native LyraFacialSkin transform")
    frame = NativeFrameTransform(faces[0]["primaryTransform"])
    if frame.rigidity_error > 1e-6 or frame.determinant <= 0:
        raise ValueError("Expression measurement requires a rigid, non-reflected native face frame")
    return {name: frame.inverse_point(point) for name, point in document.landmarks.items()}


class LyraInspectionMaterials:
    """An explicitly declared display palette; names retain native material IDs."""

    PALETTE = {
        "bluecyanfabric": (0.055, 0.22, 0.29),
        "blackbluefabric": (0.018, 0.032, 0.056),
        "blackbluemattepaint": (0.025, 0.035, 0.055),
        "silvercyanpolishedmetal": (0.54, 0.66, 0.70),
        "tealbluemetal": (0.025, 0.30, 0.34),
        "violetbluemetal": (0.20, 0.12, 0.34),
        "whitepolishedmetal": (0.80, 0.84, 0.83),
        "blackmattepaint": (0.015, 0.012, 0.024),
        "violetmattepaint": (0.30, 0.075, 0.20),
        "cyanemissiveplaster": (0.06, 0.60, 0.68),
        "amberemissiveplaster": (0.65, 0.27, 0.035),
        "softpinkmatteplaster": (0.75, 0.49, 0.40),
        "warmwhitematteplaster": (0.90, 0.91, 0.88),
        "tealmattemetal": (0.10, 0.36, 0.34),
    }
    def __init__(self, hologram_settings=None):
        self.cache = {}
        self.hologram_settings = hologram_settings

    def get(self, native_name, profile):
        key = (native_name, profile)
        if key in self.cache:
            return self.cache[key]
        color = self.PALETTE.get(native_name, (0.45, 0.50, 0.53))
        material = bpy.data.materials.new(profile + "__" + native_name)
        material.use_nodes = True
        nodes, links = material.node_tree.nodes, material.node_tree.links
        nodes.clear()
        output = nodes.new("ShaderNodeOutputMaterial")
        surface = nodes.new("ShaderNodeBsdfPrincipled")
        surface.inputs["Base Color"].default_value = (*color, 1.0)
        surface.inputs["Roughness"].default_value = 0.48
        surface.inputs["Metallic"].default_value = 0.25 if "metal" in native_name else 0.0
        if profile == "ordinary":
            if "emissive" in native_name:
                surface.inputs["Emission Color"].default_value = (*color, 1.0)
                surface.inputs["Emission Strength"].default_value = 0.35
            links.new(surface.outputs["BSDF"], output.inputs["Surface"])
        else:
            self.connect_hologram(nodes, links, surface, output, color)
        self.cache[key] = material
        return material

    def connect_hologram(self, nodes, links, surface, output, color):
        settings = self.hologram_settings
        if settings is None:
            raise ValueError("A hologram material requires the authored grammar profile")
        cyan = (0.06, 0.56, 0.66)
        surface.inputs["Base Color"].default_value = (
            *(0.4 * a + 0.6 * b for a, b in zip(color, cyan)), 1.0)
        surface.inputs["Emission Color"].default_value = (*cyan, 1.0)
        geometry = nodes.new("ShaderNodeNewGeometry")
        separate = nodes.new("ShaderNodeSeparateXYZ")
        links.new(geometry.outputs["Position"], separate.inputs["Vector"])
        frequency = nodes.new("ShaderNodeMath")
        frequency.name = "HologramScanFrequency"
        frequency.operation = "MULTIPLY"
        frequency.inputs[1].default_value = 2 * math.pi / settings["scanSpacingM"]
        links.new(separate.outputs["Y"], frequency.inputs[0])
        phase = nodes.new("ShaderNodeMath")
        phase.name = "HologramScanPhase"
        phase.operation = "ADD"
        phase.inputs[1].default_value = settings["scanPhaseRadians"]
        links.new(frequency.outputs[0], phase.inputs[0])
        wave = nodes.new("ShaderNodeMath")
        wave.operation = "SINE"
        links.new(phase.outputs[0], wave.inputs[0])
        stripe = nodes.new("ShaderNodeMath")
        stripe.operation = "GREATER_THAN"
        stripe.inputs[1].default_value = 0.87
        links.new(wave.outputs[0], stripe.inputs[0])
        noise = nodes.new("ShaderNodeTexNoise")
        noise.name = "HologramAnimatedGrain"
        noise.noise_dimensions = "4D"
        noise.inputs["Scale"].default_value = settings["grainScalePerM"]
        noise.inputs["W"].default_value = settings["noiseW"]
        links.new(geometry.outputs["Position"], noise.inputs["Vector"])
        fresnel = nodes.new("ShaderNodeFresnel")
        fresnel.inputs["IOR"].default_value = 1.45
        total = nodes.new("ShaderNodeValue")
        total.outputs[0].default_value = settings["baseEmission"]
        for name, source, strength in (("Scan", stripe.outputs[0], settings["scanStrength"]),
                                       ("Grain", noise.outputs["Fac"], settings["grainStrength"]),
                                       ("Rim", fresnel.outputs["Fac"], settings["rimStrength"])):
            product = nodes.new("ShaderNodeMath")
            product.name = "Hologram"+name+"Strength"
            product.operation = "MULTIPLY"
            product.inputs[1].default_value = strength
            links.new(source, product.inputs[0])
            addition = nodes.new("ShaderNodeMath")
            addition.operation = "ADD"
            links.new(total.outputs[0], addition.inputs[0])
            links.new(product.outputs[0], addition.inputs[1])
            total = addition
        for name, strength in (("Brightness", settings["brightness"]), ("Pulse", settings["pulseFactor"])):
            multiplier = nodes.new("ShaderNodeMath")
            multiplier.name = "Hologram"+name
            multiplier.operation = "MULTIPLY"
            multiplier.inputs[1].default_value = strength
            links.new(total.outputs[0], multiplier.inputs[0])
            total = multiplier
        links.new(total.outputs[0], surface.inputs["Emission Strength"])
        transparent = nodes.new("ShaderNodeBsdfTransparent")
        mix = nodes.new("ShaderNodeMixShader")
        mix.inputs[0].default_value = settings["surfaceOpacity"]
        links.new(transparent.outputs[0], mix.inputs[1])
        links.new(surface.outputs[0], mix.inputs[2])
        links.new(mix.outputs[0], output.inputs["Surface"])

    @staticmethod
    def actual_hologram_bindings(material):
        nodes = material.node_tree.nodes
        return {"scanFrequencyRadiansPerM": nodes["HologramScanFrequency"].inputs[1].default_value,
                "scanPhaseRadians": nodes["HologramScanPhase"].inputs[1].default_value,
                "scanStrength": nodes["HologramScanStrength"].inputs[1].default_value,
                "grainScalePerM": nodes["HologramAnimatedGrain"].inputs["Scale"].default_value,
                "noiseW": nodes["HologramAnimatedGrain"].inputs["W"].default_value,
                "grainStrength": nodes["HologramGrainStrength"].inputs[1].default_value,
                "rimStrength": nodes["HologramRimStrength"].inputs[1].default_value,
                "brightness": nodes["HologramBrightness"].inputs[1].default_value,
                "pulseFactor": nodes["HologramPulse"].inputs[1].default_value}


class NativeLyraRenderScene:
    LIGHTS = (
        ("key", (-0.35, 0.6, 1), 2.0, (1.0, 0.91, 0.83)),
        ("fill", (0.55, 0.2, 1), 0.8, (0.75, 0.86, 1.0)),
        ("edge", (-0.25, 0.25, -1), 1.5, (0.65, 0.85, 1.0)),
    )
    VIEWS = {
        "front": {"direction": (0, 0, 1), "target": (0, 1.63, 0.02), "scale": 0.345},
        "profile": {"direction": (1, 0, 0), "target": (0, 1.63, 0.02), "scale": 0.345},
        "profile_left": {"direction": (1, 0, 0), "target": (0, 1.63, 0.02), "scale": 0.345},
        "profile_right": {"direction": (-1, 0, 0), "target": (0, 1.63, 0.02), "scale": 0.345},
        "three_quarter": {"direction": (0.72, 0.08, 1), "target": (0, 1.63, 0.02), "scale": 0.345},
        "mouth": {"direction": (0, 0.05, 1), "target": (0, 1.554, 0.125), "scale": 0.105},
        "cutaway": {"direction": (0.85, 0.06, 1), "target": (0, 1.63, 0.02), "scale": 0.345},
        "body": {"direction": (0.1, 0.02, 1), "target": (0, 0.89, 0), "scale": 1.92},
    }

    def __init__(self, document, width, samples, section_retained_rules=(), camera_reference=None,
                 camera_offset=(0.0, 0.0, 0.0), hologram_profile=None):
        bpy.ops.object.select_all(action="SELECT")
        bpy.ops.object.delete(use_global=False)
        self.document = document
        self.view_specs = copy.deepcopy(self.VIEWS)
        for specification in self.view_specs.values():
            specification["target"] = tuple(a + b for a, b in zip(specification["target"], camera_offset))
        reference = camera_reference or document
        if all(name in reference.landmarks for name in ("mouth_upper_margin", "mouth_lower_margin")):
            mouth = tuple((a + b) * 0.5 for a, b in zip(
                reference.landmarks["mouth_upper_margin"], reference.landmarks["mouth_lower_margin"]))
            self.view_specs["mouth"]["target"] = mouth
            self.view_specs["mouth"]["scale"] = 0.09
        self.hologram_profile = hologram_profile
        self.hologram_settings = hologram_profile.effective if hologram_profile else None
        self.materials = LyraInspectionMaterials(self.hologram_settings)
        self.objects = []
        self.section_objects = []
        self.section_evidence = []
        self.section_retained_rules = set(section_retained_rules)
        self.cable_annotations = []
        self.maximum_position_conversion_m = 0.0
        self.build_native_geometry()
        self.configure_lighting_and_camera()
        self.configure_render(width, samples)
        self.loaded_geometry_digest = self.blender_geometry_digest()

    def add_surface(self, surface, suffix=""):
        if not surface.triangle_count:
            return None
        points = [surface.positions[i:i + 3] for i in range(0, len(surface.positions), 3)]
        normals = [surface.normals[i:i + 3] for i in range(0, len(surface.normals), 3)]
        faces = [(i, i + 1, i + 2) for i in range(0, len(points), 3)]
        mesh = bpy.data.meshes.new(surface.name + suffix)
        mesh.from_pydata(points, [], faces)
        mesh.update()
        for polygon in mesh.polygons:
            polygon.use_smooth = True
        mesh.normals_split_custom_set(normals)
        obj = bpy.data.objects.new(mesh.name, mesh)
        bpy.context.collection.objects.link(obj)
        obj["native_material"] = surface.material_name
        obj["native_object"] = surface.name
        if not suffix:
            for index, vertex in enumerate(mesh.vertices):
                error = math.dist(vertex.co, surface.positions[3 * index:3 * index + 3])
                self.maximum_position_conversion_m = max(self.maximum_position_conversion_m, error)
        return obj

    def build_native_geometry(self):
        for surface in self.document.surfaces:
            self.objects.append(self.add_surface(surface))

    def build_section(self):
        if self.section_objects:
            return
        section = NativePlaneSectionService()
        for surface in self.document.surfaces:
            source_rule = surface.source_metadata.get("source", {}).get("rule")
            if source_rule in self.section_retained_rules:
                obj = self.add_surface(surface, "__retained_section_interior")
                self.section_objects.append(obj)
                self.section_evidence.append({
                    "sourceObject": surface.name, "sourceRule": source_rule,
                    "sourceTriangles": surface.triangle_count,
                    "sectionTriangles": surface.triangle_count,
                    "retainedWholeAsDeclaredInterior": True,
                })
                continue
            clipped, evidence = section.section(surface)
            evidence["sourceRule"] = source_rule
            self.section_evidence.append(evidence)
            obj = self.add_surface(clipped, "__section_x_le_0")
            if obj is not None:
                self.section_objects.append(obj)
        self.add_cable_annotations()

    def add_cable_annotations(self):
        for record_index, mechanics in enumerate(self.document.mechanics):
            for muscle in mechanics.get("muscles", []):
                curve = bpy.data.curves.new("native_cable_axis_" + muscle["name"], "CURVE")
                curve.dimensions = "3D"
                curve.bevel_depth = 0.0003
                curve.bevel_resolution = 2
                spline = curve.splines.new("POLY")
                spline.points.add(1)
                for point, position in zip(spline.points, (muscle["origin"], muscle["insertion"])):
                    point.co = (*position, 1.0)
                obj = bpy.data.objects.new(curve.name, curve)
                bpy.context.collection.objects.link(obj)
                obj["diagnostic_annotation"] = True
                material = bpy.data.materials.new(curve.name + "_display")
                material.use_nodes = True
                shader = material.node_tree.nodes.get("Principled BSDF")
                color = (0.95, 0.30, 0.04, 1.0) if muscle["activation"] > 0 else (0.30, 0.50, 0.60, 1.0)
                shader.inputs["Base Color"].default_value = color
                shader.inputs["Emission Color"].default_value = color
                shader.inputs["Emission Strength"].default_value = 0.4
                obj.data.materials.append(material)
                obj.hide_render = True
                self.cable_annotations.append(obj)

    def configure_lighting_and_camera(self):
        for name, direction, energy, color in self.LIGHTS:
            light = bpy.data.lights.new(name, "SUN")
            light.energy = energy
            light.angle = 0.25
            light.color = color
            obj = bpy.data.objects.new(name, light)
            bpy.context.collection.objects.link(obj)
            obj.rotation_euler = (-Vector(direction)).to_track_quat("-Z", "Y").to_euler()
        self.hologram_projector = None
        if self.hologram_settings:
            self.hologram_projector = self.create_hologram_projector(self.hologram_settings)
        camera = bpy.data.cameras.new("native_lyra_inspection_camera")
        camera.type = "ORTHO"
        camera.clip_start = 0.001
        camera.clip_end = 100.0
        self.camera = bpy.data.objects.new(camera.name, camera)
        bpy.context.collection.objects.link(self.camera)
        bpy.context.scene.camera = self.camera

    @staticmethod
    def create_hologram_projector(settings):
        light = bpy.data.lights.new("authored_hologram_projector", "SUN")
        light.energy = settings["projectorEnergy"]
        light.color = (0.06, 0.56, 0.66)
        light.angle = 0.25
        obj = bpy.data.objects.new(light.name, light)
        bpy.context.collection.objects.link(obj)
        obj.rotation_euler = (-Vector((0.0, -1.0, 0.35))).to_track_quat("-Z", "Y").to_euler()
        obj.hide_render = True
        return obj

    @staticmethod
    def apply_hologram_glare(settings, glare):
        glare.glare_type = "FOG_GLOW"
        glare.quality = "HIGH"
        glare.threshold = settings["bloomThreshold"]
        glare.mix = settings["bloomMix"]
        glare.size = 8

    def configure_render(self, width, samples):
        scene = bpy.context.scene
        scene.render.engine = "CYCLES"
        scene.cycles.device = "CPU"
        scene.cycles.samples = samples
        scene.cycles.seed = 0
        scene.cycles.use_denoising = False
        scene.cycles.max_bounces = 8
        scene.cycles.transparent_max_bounces = 12
        scene.render.resolution_x = width
        scene.render.resolution_y = width
        scene.render.resolution_percentage = 100
        scene.render.threads_mode = "FIXED"
        scene.render.threads = 5
        scene.render.image_settings.file_format = "PNG"
        scene.render.image_settings.color_mode = "RGB"
        scene.world.use_nodes = True
        scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.035, 0.045, 0.065, 1)
        scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.35
        scene.view_settings.view_transform = "AgX"
        scene.view_settings.exposure = 0.0
        scene.use_nodes = True
        nodes = scene.node_tree.nodes
        nodes.clear()
        render = nodes.new("CompositorNodeRLayers")
        self.glare = nodes.new("CompositorNodeGlare")
        self.glare.glare_type = "FOG_GLOW"
        self.glare.quality = "HIGH"
        if self.hologram_settings:
            self.apply_hologram_glare(self.hologram_settings, self.glare)
        self.glare.size = 8
        output = nodes.new("CompositorNodeComposite")
        scene.node_tree.links.new(render.outputs["Image"], self.glare.inputs["Image"])
        scene.node_tree.links.new(self.glare.outputs["Image"], output.inputs["Image"])

    def blender_geometry_digest(self):
        digest = hashlib.sha256()
        for obj in self.objects:
            points = array("f", [0.0]) * (3 * len(obj.data.vertices))
            obj.data.vertices.foreach_get("co", points)
            digest.update(obj.name.encode())
            digest.update(points.tobytes())
        return digest.hexdigest()

    def render(self, view, profile, output):
        specification = self.view_specs[view]
        target = Vector(specification["target"])
        self.camera.location = target + Vector(specification["direction"]).normalized()
        # Native coordinates use +Y up. Blender's generic tracking helper uses
        # its own world-up convention and can roll profile views by 90 degrees.
        forward = (target - self.camera.location).normalized()
        right = forward.cross(Vector((0, 1, 0))).normalized()
        up = right.cross(forward).normalized()
        self.camera.rotation_euler = Matrix((right, up, -forward)).transposed().to_euler()
        self.camera.data.ortho_scale = specification["scale"]
        is_section = view == "cutaway"
        if is_section:
            self.build_section()
        for obj in self.objects:
            obj.hide_render = is_section
        for obj in self.section_objects:
            obj.hide_render = not is_section
        for obj in self.cable_annotations:
            obj.hide_render = not is_section
        active = self.section_objects if is_section else self.objects
        for obj in active:
            obj.data.materials.clear()
            obj.data.materials.append(self.materials.get(obj["native_material"], profile))
        self.glare.mute = profile == "ordinary"
        if self.hologram_projector:
            self.hologram_projector.hide_render = profile != "hologram"
        if self.blender_geometry_digest() != self.loaded_geometry_digest:
            raise ValueError("Native source geometry changed between presentation profiles")
        scene = bpy.context.scene
        scene.render.filepath = str(output.resolve())
        started = time.monotonic()
        bpy.ops.render.render(write_still=True)
        return {
            "image": output.name,
            "imageSha256": hashlib.sha256(output.read_bytes()).hexdigest(),
            "view": view,
            "profile": profile,
            "derivedCutaway": is_section,
            "cameraPositionM": list(self.camera.location),
            "cameraTargetM": list(target),
            "orthographicHeightM": specification["scale"],
            "renderSeconds": time.monotonic() - started,
            "geometryDigestBeforeRender": self.loaded_geometry_digest,
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--obj", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--samples", type=int, default=24)
    parser.add_argument("--views", default="front,profile,three_quarter,cutaway")
    parser.add_argument("--profiles", default="ordinary,hologram")
    parser.add_argument("--case-id", default="neutral")
    parser.add_argument("--case-label", default="Neutral")
    parser.add_argument("--comparison-obj", type=Path,
                        help="Verify the other grammar variant has identical native geometry")
    parser.add_argument("--camera-reference-obj", type=Path,
                        help="Reuse neutral landmarks for the mouth camera across expression cases")
    parser.add_argument("--camera-offset", type=float, nargs=3, default=(0.0, 0.0, 0.0),
                        help="Offset fixed view targets without moving the native geometry")
    parser.add_argument("--section-retain-rules", default="",
                        help="Comma-separated source rules retained whole in the section")
    parser.add_argument("--allow-baseline-without-facial-metadata", action="store_true")
    parser.add_argument("--hologram-grammar", type=Path,
                        help="Consume this grammar's # @hologram JSON declaration in the Blender adapter")
    parser.add_argument("--presentation-time", type=float, default=0.0,
                        help="Frozen seconds for declared hologram grain, scan and pulse speeds")
    parser.add_argument("--hologram-seed", type=int, default=0)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    args.output.mkdir(parents=True, exist_ok=True)
    document = NativeLyraObjReader().read(args.obj)
    if not args.allow_baseline_without_facial_metadata:
        if not document.landmarks or not document.mechanics:
            raise ValueError("An integrated facial render requires native landmarks and mechanics")
        for mechanics in document.mechanics:
            if not mechanics.get("converged"):
                raise ValueError("A rejected native equilibrium cannot be presented as a solved pose")
    comparison = None
    if args.comparison_obj:
        other = NativeLyraObjReader().read(args.comparison_obj)
        if (other.geometry_sha256 != document.geometry_sha256 or
                other.triangle_count != document.triangle_count):
            raise ValueError("Ordinary and hologram grammar variants changed native geometry")
        comparison = other.provenance()
    retained_rules = [rule for rule in args.section_retain_rules.split(",") if rule]
    known_rules = {surface.source_metadata.get("source", {}).get("rule")
                   for surface in document.surfaces}
    if any(rule not in known_rules for rule in retained_rules):
        raise ValueError("A declared section interior rule is absent from native metadata")
    camera_reference = NativeLyraObjReader().read(args.camera_reference_obj) if args.camera_reference_obj else document
    hologram_profile = None
    if args.hologram_grammar:
        from hologram_profile import HologramPresentationProfile
        hologram_profile = HologramPresentationProfile.read(args.hologram_grammar,
                                                          args.presentation_time, args.hologram_seed)
    if "hologram" in args.profiles.split(",") and hologram_profile is None:
        raise ValueError("Hologram rendering requires --hologram-grammar with an authored profile")
    scene = NativeLyraRenderScene(document, args.width, args.samples, retained_rules,
                                 camera_reference, args.camera_offset, hologram_profile)
    local_landmarks = native_face_local_landmarks(document)
    reference_local_landmarks = native_face_local_landmarks(camera_reference)
    landmark_displacements = {name: math.dist(point, reference_local_landmarks[name]) * 1000
                              for name, point in local_landmarks.items()
                              if name in reference_local_landmarks}
    manifest = document.provenance()
    manifest.update({
        "prototype": bool(args.allow_baseline_without_facial_metadata),
        "caseId": args.case_id,
        "caseLabel": args.case_label,
        "mechanics": document.mechanics,
        "landmarks": document.landmarks,
        "ordinaryHologramVariantComparison": comparison,
        "cameraReferenceSourceSha256": camera_reference.source_sha256,
        "fixedViewTargetOffsetM": args.camera_offset,
        "landmarkDisplacementsFromReferenceMm": landmark_displacements,
        "landmarkDisplacementFrame": "native face local, each actual rigid instance transform removed",
        "renderEngine": "Blender Cycles",
        "blenderVersion": bpy.app.version_string,
        "rendererSourceSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "nativeReaderSourceSha256": hashlib.sha256(Path(__file__).with_name("native_presentation.py").read_bytes()).hexdigest(),
        "nativeFrameSourceSha256": hashlib.sha256(Path(__file__).with_name("geometry_validation.py").read_bytes()).hexdigest(),
        "device": "CPU",
        "samples": args.samples,
        "resolution": [args.width, args.width],
        "denoising": False,
        "colorTransform": "AgX",
        "exposure": 0.0,
        "nativeViewportShaderParityClaimed": False,
        "displayPalette": LyraInspectionMaterials.PALETTE,
        "hologramProfile": hologram_profile.receipt() if hologram_profile else None,
        "unchangedCommonLightRigAcrossProfiles": True,
        "totalLightingIdenticalAcrossProfiles": hologram_profile is None,
        "profileSpecificProjector": ({"profiles": ["hologram"], "type": "SUN",
            "directionFromModel": [0.0, -1.0, 0.35], "color": [0.06, 0.56, 0.66],
            "energy": scene.hologram_projector.data.energy, "angularDiameterRadians": 0.25}
            if scene.hologram_projector else None),
        "directionalLights": [{"name": name, "directionFromModel": direction,
                               "energy": energy, "color": color, "angularDiameterRadians": 0.25}
                              for name, direction, energy, color in scene.LIGHTS],
        "worldBackground": {"rgba": [0.035, 0.045, 0.065, 1], "strength": 0.35},
        "cycleSettings": {"maximumBounces": 8, "transparentMaximumBounces": 12,
                          "seed": 0, "threads": 5},
        "blenderNativePositionSha256": scene.loaded_geometry_digest,
        "maximumObjDecimalToBlenderPositionChangeM": scene.maximum_position_conversion_m,
        "cutaway": {"plane": "world x <= 0 m", "capsAdded": False,
                    "retainedWholeSourceRules": retained_rules,
                    "cableAnnotations": "native origin/insertion axes, 0.3 mm display radius; not muscle volume",
                    "method": "triangle-plane clipping of a derived copy; intersections interpolated"},
        "images": [],
        "renderComplete": False,
    })
    (args.output / "render_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    for view in args.views.split(","):
        for profile in args.profiles.split(","):
            if view == "cutaway" and profile != "ordinary":
                continue
            destination = args.output / (view + "_" + profile + ".png")
            manifest["images"].append(scene.render(view, profile, destination))
            manifest["cutaway"]["objects"] = scene.section_evidence
            (args.output / "render_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if not manifest["images"]:
        raise ValueError("The requested views/profiles produced no image")
    manifest["renderComplete"] = True
    if hologram_profile:
        actual_materials = [material for (name, profile), material in scene.materials.cache.items()
                            if profile == "hologram"]
        manifest["actualHologramBindings"] = {
            "materialNodeValues": {material.name: LyraInspectionMaterials.actual_hologram_bindings(material)
                                   for material in actual_materials},
            "bloomThreshold": scene.glare.threshold,
            "bloomMix": scene.glare.mix,
            "projectorEnergy": scene.hologram_projector.data.energy,
        }
    (args.output / "render_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
