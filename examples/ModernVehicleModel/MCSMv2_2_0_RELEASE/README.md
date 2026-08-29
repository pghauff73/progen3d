# Modern Car MCSMv2.2

MCSMv2.2 is the concept-kinematics release of the hybrid semantic-surface and implicit-scaffold modern-car generator.

## Delivered layers

```text
Package and evidence-aware parameter DAG
Semantic section and character-curve network
Persistent UV semantic surface
Implicit outer scaffold and independent alignment
Exclusive panel/aperture ownership
Concept suspension hardpoints and wheel-pose solver
Actual tyre-mesh pose sweeps
Four doors, bonnet and rear-hatch motion
Parent-relative helical side-glass motion
Independent sampled collision and clearance validation
V0-V3 assurance record
```

## Regenerate

```bash
python source/generate_release.py --output generated --resolution medium --no-preview
```

See [QUICKSTART.md](QUICKSTART.md) for single-variant generation, tests and output descriptions.

## Assurance

The release achieves **V3 sampled concept-kinematic assurance** for all four variants. It does not claim measured production hardpoints, exact mechanism design, manufacturing approval, structural FEA, CFD, crash validation, homologation or physical testing.

## Key records

- `validation.json`: complete release evidence;
- `VALIDATION_SUMMARY.md`: concise assurance table;
- `kinematics/`: suspension, tyre, closure and glass records;
- `partitions/`: panel and aperture ownership;
- `models/`: body, scene, partition, suspension and closure artifacts;
- `TEST_REPORT.md`: automated verification;
- `KNOWN_LIMITATIONS.md`: explicit model boundary.
