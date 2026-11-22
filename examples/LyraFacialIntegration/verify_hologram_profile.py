"""Blender graph-consumption regression for each authored hologram parameter.

Run with blender -b --python examples/LyraFacialIntegration/verify_hologram_profile.py
-- --grammar GENERATED_HOLOGRAM.p3d --output binding_validation.json
This exercises the production material, compositor and projector builders.
It does not claim image similarity or native viewport shader parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hologram_profile import HologramPresentationProfile
from render_study import LyraInspectionMaterials, NativeLyraRenderScene


class HologramGraphConsumptionAudit:
    BINDINGS = {
        "brightness": "brightness",
        "grain_scale": "grainScalePerM",
        "grain_speed": "noiseW",
        "grain_strength": "grainStrength",
        "rim_strength": "rimStrength",
        "scan_spacing": "scanFrequencyRadiansPerM",
        "scan_speed": "scanPhaseRadians",
        "scan_strength": "scanStrength",
        "pulse_speed": "pulseFactor",
        "bloom_threshold": "bloomThreshold",
        "bloom_mix": "bloomMix",
        "projector_strength": "projectorEnergy",
    }

    @staticmethod
    def actual_bindings(profile):
        materials = LyraInspectionMaterials(profile.effective)
        material = materials.get("silvercyanpolishedmetal", "hologram")
        result = materials.actual_hologram_bindings(material)
        scene = bpy.context.scene
        scene.use_nodes = True
        glare = scene.node_tree.nodes.new("CompositorNodeGlare")
        NativeLyraRenderScene.apply_hologram_glare(profile.effective, glare)
        projector = NativeLyraRenderScene.create_hologram_projector(profile.effective)
        result.update({"bloomThreshold": glare.threshold, "bloomMix": glare.mix,
                       "projectorEnergy": projector.data.energy})
        # Verify the parameter-bearing material nodes are connected to output,
        # not merely unused nodes whose inputs happen to change.
        nodes = material.node_tree.nodes
        output = next(node for node in nodes if node.bl_idname == "ShaderNodeOutputMaterial")
        upstream, stack = set(), [output]
        while stack:
            node = stack.pop()
            if node.name in upstream:
                continue
            upstream.add(node.name)
            stack.extend(link.from_node for link in material.node_tree.links if link.to_node == node)
        required = {"HologramScanFrequency", "HologramScanPhase", "HologramScanStrength",
                    "HologramAnimatedGrain", "HologramGrainStrength", "HologramRimStrength",
                    "HologramBrightness", "HologramPulse"}
        if not required <= upstream:
            raise ValueError("A parameter-bearing hologram node does not reach the rendered output")
        scene.node_tree.nodes.remove(glare)
        light = projector.data
        bpy.data.objects.remove(projector, do_unlink=True)
        bpy.data.lights.remove(light)
        bpy.data.materials.remove(material)
        return result

    def run(self, grammar_path):
        # Nonzero time exposes all three authored speeds in a frozen state.
        time_seconds, seed = 0.731, 17
        original = HologramPresentationProfile.read(grammar_path, time_seconds, seed)
        declared = original.receipt()["declared"]
        baseline = self.actual_bindings(original)
        results = []
        with tempfile.TemporaryDirectory(prefix="lyra_hologram_binding_") as directory:
            for parameter, binding in self.BINDINGS.items():
                changed = dict(declared)
                changed[parameter] = (changed[parameter]+0.05 if parameter == "bloom_mix"
                                      else changed[parameter]*1.17+0.013)
                if parameter == "bloom_mix" and changed[parameter] > 1:
                    changed[parameter] -= 0.1
                path = Path(directory)/"authored_profile.p3d"
                path.write_text("# @hologram "+json.dumps(changed)+"\n")
                profile = HologramPresentationProfile.read(path, time_seconds, seed)
                actual = self.actual_bindings(profile)
                changed_value = actual[binding] != baseline[binding]
                results.append({"parameter": parameter, "actualBinding": binding,
                                "declaredBefore": declared[parameter], "declaredAfter": changed[parameter],
                                "actualBefore": baseline[binding], "actualAfter": actual[binding],
                                "changedActualConnectedGraphValue": changed_value})
        return {"schema": "progen3d.lyra-hologram-bindings.v1",
                "passed": all(item["changedActualConnectedGraphValue"] for item in results),
                "declaredParameterCount": len(self.BINDINGS), "blenderVersion": bpy.app.version_string,
                "presentationTimeSeconds": time_seconds, "seed": seed,
                "sourceProfile": original.receipt(), "actualBaselineBindings": baseline,
                "parameterPerturbations": results,
                "rendererSourceSha256": hashlib.sha256(Path(__file__).with_name("render_study.py").read_bytes()).hexdigest(),
                "scope": "Each authored parameter is perturbed independently through the actual declaration parser. "
                         "Production Blender material nodes, glare inputs and projector energy are read back. "
                         "All material parameter nodes reach the surface output. This is graph-consumption evidence, "
                         "not a native viewport or pixel-equivalence claim."}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--grammar", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:])
    result = HologramGraphConsumptionAudit().run(args.grammar)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+"\n")
    print(json.dumps({"passed": result["passed"], "parameters": result["declaredParameterCount"],
                      "output": str(args.output)}))
    if not result["passed"]:
        raise SystemExit(1)
