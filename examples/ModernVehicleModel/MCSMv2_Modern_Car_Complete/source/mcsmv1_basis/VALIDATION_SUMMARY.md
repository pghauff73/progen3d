# Validation Summary

## Package checks

- Python generator compiles successfully.
- All JSON data files parse successfully.
- The Progen3D target grammar has balanced delimiters and no `Object(...)` calls.
- All GLB scenes load through `trimesh` with finite vertices.
- Every body-only STL becomes watertight after ordinary indexed-vertex processing.
- All four body meshes were post-normalized to their target package dimensions.

## Main body meshes

| Variant | Vertices | Faces | Watertight | Target dimensions |
|---|---:|---:|---|---|
| Research Median AWD Hot Hatch | 33,582 | 67,160 | yes | exact within stored tolerance |
| Track Widebody AWD | 34,198 | 68,388 | yes | exact within stored tolerance |
| Aero Fastback AWD | 33,164 | 66,320 | yes | exact within stored tolerance |
| Urban Crossover EV AWD | 34,220 | 68,436 | yes | exact within stored tolerance |

## Interpretation

The watertight test applies to the principal body envelope. Full GLB scenes contain
multiple separate components such as glazing, wheels, lights, mirrors, splitter,
wing and underbody parts, so the scene as a whole is intentionally a multi-body
assembly rather than one watertight manufacturing solid.

The projected frontal areas are rasterized geometric proxies. They are not CFD
results and must not be interpreted as drag coefficients.
