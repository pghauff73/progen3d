"""Measure native body, fitted-face, model-assembly and equilibrium costs.

The generated fixtures preserve the authored expressions. The body fixture
omits only the pinned source's head call; the facial fixture invokes the final
delivered wrapper at bilateral activation 0.5. This is an offline component
measurement, not a displayed-frame-rate benchmark.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cpu_model():
    information = Path('/proc/cpuinfo')
    if information.exists():
        for line in information.read_text().splitlines():
            if line.startswith('model name'):
                return line.split(':', 1)[1].strip()
    return platform.processor() or 'not reported'


def build_fixtures(integration, destination):
    manifest = json.loads((integration / 'fit_manifest.json').read_text())
    source_path = Path(__file__).parent / 'reference/Lyra_Procedural_NoMesh.p3d'
    if digest(source_path) != manifest['source']['sha256']:
        raise ValueError('Timing source does not match the pinned original character.')
    source = source_path.read_text()
    if source.count('LyraHead ]') != 1:
        raise ValueError('Expected exactly one pinned body-to-head call.')
    body_source = source.replace('LyraHead ]', ']')
    ordinary_path = integration / 'Lyra_Procedural_Facial_Elastic.p3d'
    ordinary_hash = digest(ordinary_path)
    if ordinary_hash != manifest['generated']['ordinaryGrammarSha256']:
        raise ValueError('Timing grammar does not match its generated fit manifest; rebuild and revalidate first.')
    ordinary = ordinary_path.read_text()
    rules = list(re.finditer(r'^(\w+)(?:\([^\n]*\))?\s*->', ordinary, re.MULTILINE))
    face_rules = [ordinary[match.start():rules[index+1].start() if index+1 < len(rules) else len(ordinary)]
                  for index, match in enumerate(rules) if match.group(1) == 'LyraFacialSkin']
    if len(face_rules) != 1:
        raise ValueError('Expected exactly one delivered facial wrapper.')
    face_source = 'Start -> LyraFacialSkin(.5 .5)\n' + face_rules[0]
    destination.mkdir(parents=True, exist_ok=True)
    body_path, face_path = destination / 'body_only.p3d', destination / 'face_only.p3d'
    body_path.write_text(body_source)
    face_path.write_text(face_source)
    return body_path, face_path, {'sourceSha256': digest(source_path),
        'ordinaryGrammarSha256': ordinary_hash,
        'manifestSha256': digest(integration / 'fit_manifest.json'),
        'bodyFixtureChange': 'Remove exactly the original LyraHead call inside its existing scope; all other source bytes remain.',
        'facialFixtureChange': 'Use the exact delivered LyraFacialSkin wrapper with bilateral0.5 activation; no other geometry.'}


def compare_body_baseline(report, receipt_path):
    receipt = json.loads(receipt_path.read_text())
    native_path = receipt_path.with_name('report.json')
    if (receipt.get('returncode') != 0 or receipt['sourceSha256'] != report['fixtureProvenance']['sourceSha256']
            or digest(native_path) != receipt['reportSha256']):
        raise ValueError('Body timing baseline receipt or source hash is invalid.')
    native = json.loads(native_path.read_text())
    neutral = next(sample for sample in native['samples'] if sample['seconds'] == 0)
    expected = Counter(instance['nativeBufferSha256'] for instance in neutral['scene']['instances']
                       if instance['source']['rule'] != 'LyraHead')
    counts = [Counter(sample['instanceBufferSha256']) for sample in report['bodyOnly']['samples']]
    matches = all(actual == expected for actual in counts)
    report['originalBodyBufferComparison'] = {'passed': matches, 'comparedObjects': sum(expected.values()),
        'baselineReportSha256': receipt['reportSha256'], 'baselineSamplerSha256': receipt['samplerSha256'],
        'interpretation': 'Exact actual native buffer multiset equality at time zero; both build identities are recorded.'}
    report['passed'] = report['passed'] and matches


def run(arguments):
    integration = arguments.integration_dir.resolve()
    destination = integration / 'timing'
    body, face, provenance = build_fixtures(integration, destination)
    harness = arguments.timing_harness.resolve()
    native_path = destination / 'native_timing.json'
    result = subprocess.run([str(harness), str(body), str(face), str(native_path)],
                            capture_output=True, text=True, timeout=240)
    (destination / 'native_timing.log').write_text(result.stdout + result.stderr)
    if not native_path.exists():
        raise RuntimeError('Native timing harness produced no report: ' + result.stderr[-2000:])
    report = json.loads(native_path.read_text())
    report['fixtureProvenance'] = provenance
    report['timingHarnessSha256'] = digest(harness)
    report['timingStudySourceSha256'] = digest(Path(__file__))
    report['machine'] = {'operatingSystem': platform.platform(), 'architecture': platform.machine(),
                         'cpuModel': cpu_model(), 'logicalCpuCount': os.cpu_count(),
                         'resolution': None, 'resolutionReason': 'No viewport is drawn in headless evaluation.'}
    report['passed'] = report.get('passed') is True and result.returncode == 0
    if report['passed'] and arguments.source_receipt:
        compare_body_baseline(report, arguments.source_receipt.resolve())
    summary = {}
    for component in ('bodyOnly', 'faceOnly'):
        samples = report.get(component, {}).get('samples', [])
        for field in ('parseIncludingFirstExpansionSeconds', 'repeatedExpansionSeconds',
                      'nativeGenerationSeconds', 'bufferConstructionAndHashSeconds'):
            values = [sample[field] for sample in samples if field in sample]
            if values:
                summary[component + '.' + field] = {'minimumSeconds': min(values),
                    'medianSeconds': statistics.median(values), 'maximumSeconds': max(values)}
    for field in ('modelAssemblySeconds', 'equilibriumSolveSeconds'):
        values = [sample['separateEquilibriumRepeat'][field]
                  for sample in report.get('faceOnly', {}).get('samples', []) if 'separateEquilibriumRepeat' in sample]
        if values:
            summary[field] = {'minimumSeconds': min(values), 'medianSeconds': statistics.median(values),
                              'maximumSeconds': max(values)}
    report['componentTimingSummary'] = summary
    output = integration / 'timing_validation.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'passed': report['passed'], 'output': str(output), 'summary': summary}, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--timing-harness', required=True, type=Path)
    parser.add_argument('--integration-dir', required=True, type=Path)
    parser.add_argument('--source-receipt', type=Path,
                        help='Optional existing native source timeline receipt for exact body-buffer comparison.')
    raise SystemExit(run(parser.parse_args()))
