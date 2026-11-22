"""Package native render evidence and its existing validation receipt as HTML.

The report contains the rendered PNGs, their source hashes and actual numerical
receipts. It does not solve geometry or interpolate between equilibria.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path


class LyraIntegrationEvidencePackager:
    def collect_cases(self, render_root):
        cases = []
        identifiers = set()
        for path in sorted(render_root.rglob("render_manifest.json")):
            case = json.loads(path.read_text())
            if not case.get("prototype") and case.get("renderComplete") is not True:
                raise ValueError("An integrated render case did not complete: " + str(path))
            if not case.get("images"):
                raise ValueError("A render case has no images: " + str(path))
            case.setdefault("caseId", path.parent.name)
            case.setdefault("caseLabel", case["caseId"])
            if case["caseId"] in identifiers:
                raise ValueError("Each render case must have a unique caseId")
            identifiers.add(case["caseId"])
            for image in case["images"]:
                source = path.parent / image["image"]
                if not source.is_file():
                    raise ValueError("A render manifest references a missing image: " + str(source))
                image_bytes = source.read_bytes()
                if not case.get("prototype") and hashlib.sha256(image_bytes).hexdigest() != image.get("imageSha256"):
                    raise ValueError("A rendered image no longer matches its receipt: " + str(source))
                image["dataUrl"] = "data:image/png;base64," + base64.b64encode(image_bytes).decode("ascii")
            cases.append(case)
        if not cases:
            raise ValueError("No render_manifest.json found below the supplied render directory")
        cases.sort(key=lambda case: (case["caseId"] != "neutral", case["caseId"]))
        return cases

    def build(self, render_root, validation_path, fit_manifest_path, destination,
              editor_validation_path=None, baseline_validation_path=None,
              silhouette_path=None, hologram_validation_path=None, optimization_path=None,
              timing_path=None):
        cases = self.collect_cases(render_root)
        validation = json.loads(validation_path.read_text()) if validation_path else None
        fit = json.loads(fit_manifest_path.read_text()) if fit_manifest_path else None
        editor = json.loads(editor_validation_path.read_text()) if editor_validation_path else None
        baseline = json.loads(baseline_validation_path.read_text()) if baseline_validation_path else None
        silhouette = json.loads(silhouette_path.read_text()) if silhouette_path else None
        hologram = json.loads(hologram_validation_path.read_text()) if hologram_validation_path else None
        optimization = json.loads(optimization_path.read_text()) if optimization_path else None
        timing = json.loads(timing_path.read_text()) if timing_path else None
        hologram_cases = [case for case in cases if any(image["profile"] == "hologram" for image in case["images"])]
        profile_matches = bool(hologram) and all(
            case.get("hologramProfile", {}).get("profileSha256") == hologram.get("sourceProfile", {}).get("profileSha256")
            and case.get("rendererSourceSha256") == hologram.get("rendererSourceSha256") for case in hologram_cases)
        if any(case.get("prototype") for case in cases):
            status = "Presentation prototype"
        elif validation is None:
            status = "Validation receipt not attached"
        elif validation.get("passed") is False:
            status = "Native integration gates failed"
        elif editor is not None and editor.get("passed") is False:
            status = "Editor integration gates failed"
        elif baseline is not None and baseline.get("passed") is False:
            status = "Baseline regression gates failed"
        elif hologram_cases and (not hologram or hologram.get("passed") is not True or not profile_matches):
            status = "Native receipts attached; matching hologram consumption gate not established"
        elif editor is None:
            status = "Native receipt attached; editor gate not attached"
        elif editor.get("passed") is not True:
            status = "Inspect attached editor validation receipt"
        elif validation.get("passed") is True:
            status = ("Full native and editor integration gates passed"
                      if validation.get("fullTimelineValidated") is True
                      else "Bounded native/editor gates passed; full timeline not validated")
        else:
            status = "Inspect attached validation receipt"
        data = {
            "schema": "progen3d.lyra-facial-report.v1",
            "builtUtc": datetime.now(timezone.utc).isoformat(),
            "status": status,
            "cases": cases,
            "validation": validation,
            "editorValidation": editor,
            "baselineValidation": baseline,
            "silhouette": silhouette,
            "hologramValidation": hologram,
            "optimizationValidation": optimization,
            "timingValidation": timing,
            "fit": fit,
        }
        template = Path(__file__).with_name("report_template.html").read_text()
        payload = json.dumps(data, separators=(",", ":"), ensure_ascii=False).replace("<", "\\u003c")
        if template.count("__EVIDENCE_JSON__") != 1:
            raise ValueError("Report template needs exactly one data placeholder")
        document = template.replace("__EVIDENCE_JSON__", payload)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(document)
        return {"output": str(destination), "caseCount": len(cases),
                "imageCount": sum(len(case["images"]) for case in cases),
                "bytes": destination.stat().st_size, "status": status}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--renders", type=Path, required=True)
    parser.add_argument("--validation", type=Path)
    parser.add_argument("--fit-manifest", type=Path)
    parser.add_argument("--editor-validation", type=Path)
    parser.add_argument("--baseline-validation", type=Path)
    parser.add_argument("--silhouette-sections", type=Path)
    parser.add_argument("--hologram-validation", type=Path)
    parser.add_argument("--optimization-validation", type=Path)
    parser.add_argument("--timing-validation", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(LyraIntegrationEvidencePackager().build(
        args.renders, args.validation, args.fit_manifest, args.output,
        args.editor_validation, args.baseline_validation, args.silhouette_sections,
        args.hologram_validation, args.optimization_validation, args.timing_validation), indent=2))
