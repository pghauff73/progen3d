# MCSMv2.2 Generation Notes

The full release was generated with one isolated process per vehicle variant and merged afterward. This limits retained SciPy, trimesh and optional graphics state between high-density final-mesh validation runs.

The supplied `source/generate_release.py` performs the same isolated-worker workflow. The main source also supports `--variant`, `--worker`, and `--merge-workers` for explicit orchestration.

The release uses medium resolution for all four final bodies and semantic/kinematic evidence artifacts. Preview images are reference-variant evidence views and are not photoreal production renders.
