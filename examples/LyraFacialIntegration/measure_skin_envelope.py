#!/usr/bin/env python3
"""Record silhouette changes from two actual native head-local captures.

The source capture must use Start -> LyraHead from the pinned original. The
integrated capture must use Start -> LyraHead(0 0). Hair, eyes and accessories
are excluded; this comparison describes the core skin and procedural collars.
It is evidence of the fitted generic shape, not a claim of identity preservation.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re


def file_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def skin_instances(report: dict, original: bool) -> list[dict]:
    sample = report['samples'][0]
    if not report['passed'] or not sample['passed']:
        raise ValueError('Silhouette evidence requires a successful native capture.')
    result = []
    for instance in sample['scene']['instances']:
        source = instance['source']
        rule = source['rule']
        selected = ((rule == 'LyraHead' and source['primitiveOrdinal'] in (0, 1, 2)) if original
                    else rule == 'LyraFacialSkin' or rule.startswith('LyraFaceCollar') or
                    rule.startswith('LyraRetainedHead'))
        if selected:
            if len(instance['geometry']['positions']) % 9:
                raise ValueError('Native skin capture is not expanded triangles.')
            result.append(instance)
    if len(result) != (3 if original else 31):
        raise ValueError('Native skin capture does not contain the declared head components.')
    return result


def section(instances: list[dict], height: float) -> dict:
    points = []
    for instance in instances:
        values = instance['geometry']['positions']
        for start in range(0, len(values), 9):
            triangle = [values[start + offset:start + offset + 3] for offset in (0, 3, 6)]
            if not min(point[1] for point in triangle) <= height <= max(point[1] for point in triangle):
                continue
            for index in range(3):
                first, second = triangle[index], triangle[(index + 1) % 3]
                if first[1] == height:
                    points.append(first)
                if (first[1] < height < second[1]) or (second[1] < height < first[1]):
                    fraction = (height - first[1]) / (second[1] - first[1])
                    points.append([first[axis] + fraction * (second[axis] - first[axis]) for axis in range(3)])
    if not points:
        raise ValueError(f'The declared silhouette plane at Y={height} has no native skin intersections.')
    return {'minimumXM': min(point[0] for point in points),
            'maximumXM': max(point[0] for point in points),
            'frontmostZM': max(point[2] for point in points),
            'rearmostZM': min(point[2] for point in points)}


def measure(source_report: Path, integrated_report: Path, heights: list[float]) -> dict:
    source = skin_instances(json.loads(source_report.read_text()), True)
    integrated = skin_instances(json.loads(integrated_report.read_text()), False)
    sections = []
    for height in heights:
        original, fitted = section(source, height), section(integrated, height)
        sections.append({'headLocalYM': height, 'original': original, 'integrated': fitted,
                         'widthChangeM': fitted['maximumXM'] - fitted['minimumXM'] -
                                         original['maximumXM'] + original['minimumXM'],
                         'frontChangeM': fitted['frontmostZM'] - original['frontmostZM'],
                         'rearChangeM': fitted['rearmostZM'] - original['rearmostZM']})
    return {'source': 'Actual native triangle-plane intersections of core skin; hair, eyes and accessories excluded.',
            'interpretation': 'The generic fitted face changes the jaw/chin silhouette; anchor agreement does not establish likeness preservation.',
            'sourceReportSha256': file_sha256(source_report),
            'integratedReportSha256': file_sha256(integrated_report),
            'sourceNativeBuffers': [instance['nativeBufferSha256'] for instance in source],
            'integratedNativeBuffers': [instance['nativeBufferSha256'] for instance in integrated],
            'sections': sections}


def replace_start(source: str, expression: str) -> str:
    result, count = re.subn(r'^Start\s*->[^\n]*', 'Start -> ' + expression,
                           source, flags=re.MULTILINE)
    if count != 1:
        raise ValueError('A head capture requires exactly one original Start rule.')
    return result


def verified_native_capture(validation: dict, fixture_name: str, expected_source: str,
                            expected_sample: str) -> tuple[Path, dict]:
    matches = [receipt for receipt in validation['nativeEvaluationReceipts']
               if Path(receipt.get('source', '')).name == fixture_name]
    if len(matches) != 1:
        raise ValueError(f'Validation must identify exactly one native capture of {fixture_name}.')
    recorded = matches[0]
    receipt_path = Path(recorded['receipt'])
    source_path = Path(recorded['source'])
    report_path = receipt_path.with_name('report.json')
    requests_path = receipt_path.with_name('requests.json')
    receipt = json.loads(receipt_path.read_text())
    # The validation report embeds the complete original receipt, adding only
    # its location and cache-use status. Comparing every original field binds
    # the receipt itself even though validation v1 stores no receipt-file hash.
    original_fields = {key: value for key, value in recorded.items()
                       if key not in ('receipt', 'cacheReused')}
    if receipt != original_fields or receipt.get('returncode') != 0:
        raise ValueError(f'The native receipt for {fixture_name} differs from passed validation.')
    if receipt.get('samplerSha256') != validation['samplerSha256']:
        raise ValueError(f'The native capture of {fixture_name} used a different sampler.')
    for path, key in ((source_path, 'sourceSha256'), (report_path, 'reportSha256'),
                      (requests_path, 'requestsSha256')):
        if file_sha256(path) != receipt.get(key):
            raise ValueError(f'The recorded {key} for {fixture_name} does not match its current file.')
    if source_path.read_text() != expected_source:
        raise ValueError(f'The {fixture_name} capture is not the exact declared head-local grammar.')
    report = json.loads(report_path.read_text())
    samples = report.get('samples', [])
    if (report.get('passed') is not True or report.get('source') != receipt['source'] or
            len(samples) != 1 or samples[0].get('passed') is not True or
            samples[0].get('id') != expected_sample or samples[0].get('seconds') != 0):
        raise ValueError(f'The recorded {fixture_name} report is not a successful neutral head capture.')
    return report_path, {'fixture': str(source_path), 'sourceSha256': receipt['sourceSha256'],
                         'receipt': str(receipt_path), 'receiptSha256': file_sha256(receipt_path),
                         'report': str(report_path), 'reportSha256': receipt['reportSha256'],
                         'requestsSha256': receipt['requestsSha256'],
                         'samplerSha256': receipt['samplerSha256']}


def validated_integration_captures(directory: Path) -> tuple[Path, Path, Path, dict]:
    directory = directory.resolve()
    validation_path = directory / 'validation.json'
    manifest_path = directory / 'fit_manifest.json'
    ordinary_path = directory / 'Lyra_Procedural_Facial_Elastic.p3d'
    source_path = Path(__file__).parent / 'reference/Lyra_Procedural_NoMesh.p3d'
    validation = json.loads(validation_path.read_text())
    manifest = json.loads(manifest_path.read_text())
    if validation.get('passed') is not True:
        raise ValueError('Silhouette evidence requires passed integration validation.')
    for path, key in ((manifest_path, 'manifestSha256'), (ordinary_path, 'ordinaryGrammarSha256'),
                      (source_path, 'sourceSha256')):
        if file_sha256(path) != validation.get(key):
            raise ValueError(f'The current {path.name} differs from the passed validation receipt.')
    if (file_sha256(ordinary_path) != manifest['generated']['ordinaryGrammarSha256'] or
            file_sha256(source_path) != manifest['source']['sha256']):
        raise ValueError('The current grammar/source does not match the fit manifest.')
    source_report, source_receipt = verified_native_capture(
        validation, 'source_head_reference.p3d', replace_start(source_path.read_text(), 'LyraHead'),
        'source_head_neutral')
    integrated_report, integrated_receipt = verified_native_capture(
        validation, 'head_neutral.p3d', replace_start(ordinary_path.read_text(), 'LyraHead(0 0)'),
        'head_neutral')
    provenance = {'validation': str(validation_path), 'validationSha256': file_sha256(validation_path),
                  'coverageMode': validation.get('coverageMode'),
                  'manifestSha256': validation['manifestSha256'],
                  'ordinaryGrammarSha256': validation['ordinaryGrammarSha256'],
                  'sourceSha256': validation['sourceSha256'],
                  'sourceHeadCapture': source_receipt, 'integratedHeadCapture': integrated_receipt}
    return manifest_path, source_report, integrated_report, provenance


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--integration-dir', type=Path,
                        help='Resolve exact successful head captures from passed validation.json and verify their receipt hashes.')
    parser.add_argument('--source-head-report', type=Path)
    parser.add_argument('--integrated-head-report', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--output', type=Path,
                        help='Separate measurement receipt; defaults to integration-dir/skin_silhouette_sections.json.')
    parser.add_argument('--attach-to-manifest', action='store_true',
                        help='Explicit capture mode only: attach measurements before running validation. Default leaves the manifest unchanged.')
    args = parser.parse_args()
    provenance = None
    if args.integration_dir:
        if args.source_head_report or args.integrated_head_report or args.manifest or args.attach_to_manifest:
            parser.error('--integration-dir cannot be combined with explicit capture inputs or --attach-to-manifest.')
        manifest_path, source_report, integrated_report, provenance = validated_integration_captures(args.integration_dir)
        output = args.output or args.integration_dir / 'skin_silhouette_sections.json'
    else:
        if not all((args.source_head_report, args.integrated_head_report, args.manifest, args.output)):
            parser.error('Provide --integration-dir or all four explicit capture/manifest/output paths.')
        manifest_path, source_report, integrated_report, output = (
            args.manifest, args.source_head_report, args.integrated_head_report, args.output)
    protected_paths = {path.resolve() for path in (manifest_path, source_report, integrated_report)}
    if provenance is not None:
        protected_paths.update((Path(provenance['validation']).resolve(),
                                (args.integration_dir / 'Lyra_Procedural_Facial_Elastic.p3d').resolve(),
                                (Path(__file__).parent / 'reference/Lyra_Procedural_NoMesh.p3d').resolve()))
        for key in ('sourceHeadCapture', 'integratedHeadCapture'):
            capture = provenance[key]
            protected_paths.update(Path(capture[field]).resolve() for field in ('fixture', 'receipt'))
            protected_paths.add(Path(capture['receipt']).with_name('requests.json').resolve())
    if output.resolve() in protected_paths:
        raise ValueError('The separate measurement receipt must not overwrite an input file.')
    manifest = json.loads(manifest_path.read_text())
    result = measure(source_report, integrated_report,
                     manifest['silhouetteSectionHeightsM'])
    result.update(schema='progen3d.lyra-native-skin-envelope.v1',
                  measurementCodeSha256=file_sha256(Path(__file__)),
                  manifestSha256=file_sha256(manifest_path), validationBinding=provenance)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n')
    if args.attach_to_manifest:
        manifest['generated']['skinEnvelopeEvidence'] = result
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'output': str(output), 'manifestModified': args.attach_to_manifest,
                      'validationBound': provenance is not None, 'sections': result['sections']}, indent=2))


if __name__ == '__main__':
    main()
