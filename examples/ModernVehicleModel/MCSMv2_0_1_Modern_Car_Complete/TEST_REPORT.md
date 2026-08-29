# MCSMv2.0.1 Test and Release Report

## Automated suite

```bash
cd source
python -m unittest discover -s tests -v
```

```text
25 tests run
25 passed
0 failures
0 errors
```

The final release run completed in approximately 12.7 seconds of unittest time.
The full console record is retained in `TEST_RESULTS.txt`.

Coverage includes:

- dependency ordering, typed values, units, hidden-input promotion and cycle paths;
- parameter-specific evidence records;
- right-handed reference-frame validation and Progen3D axis conversion;
- dense continuous section ordering for all four variants;
- evaluated curve-network residuals rather than assigned zero values;
- watertight, wound, positive-volume semantic and implicit surfaces;
- deterministic body generation with no post-mesh affine body correction;
- independent final-mesh occupant containment;
- sampled tyre sweeps checked for collision and declared clearance against the final mesh;
- synthetic self-intersection detection;
- MCSMv2.0.0 compatibility tests;
- V2 assurance boundaries and retained V3-V5 non-claims.

## Generated release validation

| Variant | Assurance | Release gate | Max bound error mm | Max curve residual mm | Self intersections | Min tyre clearance mm | Body faces |
|---|---:|---:|---:|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | V2 | True | 0.766 | 7.053 | 0 | 34.340 | 105,312 |
| Track Widebody AWD | V2 | True | 0.784 | 6.393 | 0 | 27.127 | 104,612 |
| Aero Fastback AWD | V2 | True | 0.662 | 6.962 | 0 | 33.566 | 104,600 |
| Urban Crossover EV AWD | V2 | True | 1.235 | 7.526 | 0 | 31.881 | 104,092 |

## Final-mesh tyre clearance evidence

The pass criterion requires all sampled tyre points to remain outside the final
body and the minimum measured distance to meet the declared design clearance
within a 3 mm discretization tolerance.

| Variant | Wheel envelope | Minimum mm | Declared mm | Tolerance mm | Target pass |
|---|---|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | front_left | 34.663 | 28.000 | 3.000 | True |
| Research Median AWD Hot Hatch | front_right | 34.340 | 28.000 | 3.000 | True |
| Research Median AWD Hot Hatch | rear_left | 35.396 | 28.000 | 3.000 | True |
| Research Median AWD Hot Hatch | rear_right | 35.396 | 28.000 | 3.000 | True |
| Track Widebody AWD | front_left | 28.533 | 24.000 | 3.000 | True |
| Track Widebody AWD | front_right | 28.767 | 24.000 | 3.000 | True |
| Track Widebody AWD | rear_left | 27.163 | 24.000 | 3.000 | True |
| Track Widebody AWD | rear_right | 27.127 | 24.000 | 3.000 | True |
| Aero Fastback AWD | front_left | 33.566 | 28.000 | 3.000 | True |
| Aero Fastback AWD | front_right | 33.794 | 28.000 | 3.000 | True |
| Aero Fastback AWD | rear_left | 35.080 | 28.000 | 3.000 | True |
| Aero Fastback AWD | rear_right | 35.091 | 28.000 | 3.000 | True |
| Urban Crossover EV AWD | front_left | 33.930 | 28.000 | 3.000 | True |
| Urban Crossover EV AWD | front_right | 34.067 | 28.000 | 3.000 | True |
| Urban Crossover EV AWD | rear_left | 31.881 | 28.000 | 3.000 | True |
| Urban Crossover EV AWD | rear_right | 31.901 | 28.000 | 3.000 | True |

## Release verification

`RELEASE_VERIFICATION.json` records successful Python compilation, JSON/CSV
parsing, 3D artifact loading, grammar structural validation and all four variant
gates. STL watertightness is checked after ordinary vertex welding, which is
required because binary STL stores independent triangle vertices.

All four variants achieve **V2 concept-geometry assurance**. This is a release
integrity result, not production-car approval.
