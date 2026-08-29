# MCSMv2.1 Modern Car

MCSMv2.1 is the semantic-geometry release of the hybrid modern-car model.

## Run

```bash
python source/build_mcsmv21_release.py --output MCSMv2_1_Generated --resolution medium
```

## Key additions

- registered semantic surface aligned with an independent implicit scaffold
- bidirectional distance and normal residuals
- closed periodic UV panel and aperture domains
- exclusive final-body face ownership
- explicit static glass aperture support surfaces

## Limits

This is concept-level geometry. Production Class-A, physical closures, moving
windows, exact suspension kinematics, manufacturing, CFD, crash and homologation
remain outside the implemented assurance boundary.

## Variant worker

For a single vehicle only:

```bash
python source/modern_car_mcsmv21.py --output MCSMv2_1_Reference --resolution medium --variant reference --no-preview
```

The complete-release builder isolates variants in separate processes to avoid
retained native proximity-index caches on memory-constrained systems.
