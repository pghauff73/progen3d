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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-head-report', type=Path, required=True)
    parser.add_argument('--integrated-head-report', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    result = measure(args.source_head_report, args.integrated_head_report,
                     manifest['silhouetteSectionHeightsM'])
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    manifest['generated']['skinEnvelopeEvidence'] = result
    args.manifest.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'output': str(args.output), 'sections': result['sections']}, indent=2))


if __name__ == '__main__':
    main()
