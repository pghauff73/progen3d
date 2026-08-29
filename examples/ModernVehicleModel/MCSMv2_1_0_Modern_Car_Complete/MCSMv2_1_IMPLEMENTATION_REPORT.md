# MCSMv2.1 Implementation Report

MCSMv2.1 implements the semantic-geometry patch proposed after the MCSMv2.0.1 integrity release.

## Delivered

- independently tessellated implicit scaffold without wheelhouse subtraction
- registered watertight semantic surface with persistent UV coordinates
- bidirectional point-to-triangle and normal-angle correspondence metrics
- closed UV-domain loops for panels and glazing apertures
- exclusive ownership of every final body face
- explicit windshield, side, quarter, rear and roof glass support surfaces
- surface-domain panel extraction independent of final body face normals
- new validation gates and V2 assurance wording

## Variant results

| Variant | Surface P95 max direction | Normal P95 max direction | Panel overlap | Apertures | Gate |
|---|---:|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | 3.444 mm | 14.748° | 0 | 9 | True |
| Track Widebody AWD | 3.291 mm | 14.406° | 0 | 9 | True |
| Aero Fastback AWD | 3.090 mm | 15.713° | 0 | 9 | True |
| Urban Crossover EV AWD | 3.489 mm | 14.408° | 0 | 9 | True |

## Assurance boundary

The release remains V2 concept geometry. Static apertures and surface partitions do not constitute moving-door, moving-glass, manufacturing, structural, CFD or crash validation.
