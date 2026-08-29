# MVP25 Front Door 2D-to-3D Contour Reference Log

## Artifact identity

- generated: August 22, 2026;
- source vehicle sheet: `MVP25_FinalVehicle_SixView_Dimensioned_Assembly.png`;
- source vehicle SHA-256: `77b282c882fccc39be4f62be0f970b8329dcf56c1351f691de4620edb9dc8639`;
- dimension schedule: `MVP25_FinalVehicle_Dimension_Schedule.json`;
- schedule SHA-256: `03bf853df0cc1c872d9b316b671b21d884a896b12150d1ab385efe485b3733a1`;
- generated three-view contour sheet: `MVP25_FrontDoor_ThreeView_Contour_Source.png`;
- contour sheet SHA-256: `67743c1d91f02ab6ce1e53de2dca30e02bfea38774f536516c5d8bf3a1394552`;
- reconstruction definition SHA-256: `e01438bedc18e6e62fb545fdc9a1872086763d641bacc016ca7e87edacfaa12c`;
- executable grammar SHA-256: `3a75fcfe04baf05ad5a5aae4bbf1bfa81b392f36d34177d0fb5f22981e7a6d77`;
- generator: `generate_front_door_3d_contours.py`.

## Evidence classification

The left front door `FD-L` is the contour authority. Its side silhouette is manually traced from the orange/magenta closure boundaries in the final-vehicle left view. The top trace supplies longitudinal depth variation. The full vehicle front view sees the door edge-on, so the front cross-section cannot be measured independently; it is a nominal camber profile pinned to the scheduled 42 mm maximum depth. The 1120 mm length, 1220 mm height, 42 mm depth, and 0.75 mm sheet values come from `FrontDoors.ClassAOuterSkin` in the machine-readable schedule.

## Reconstruction equations

```text
X(v,z) = D(z) F(v)
Y(v,z) = Y_lower(z) + v [Y_upper(z)-Y_lower(z)]
Z(v,z) = z
```

`Y_lower` and `Y_upper` are the side silhouette. `D(z)` is the top-view exterior depth. `F(v)` is the front-view/section camber. The generated `Loft` adds a -6 mm inboard construction plane only to close the preview envelope; that plane is not a stamped-inner-panel claim.

## Side/top stations

| Station | Z | Lower Y | Upper Y | Exterior depth |
|---:|---:|---:|---:|---:|
| 00 | -0.560 | -0.510 | 0.610 | 0.016 |
| 01 | -0.420 | -0.575 | 0.610 | 0.026 |
| 02 | -0.280 | -0.600 | 0.590 | 0.035 |
| 03 | -0.140 | -0.610 | 0.560 | 0.041 |
| 04 | 0.000 | -0.610 | 0.500 | 0.042 |
| 05 | 0.140 | -0.600 | 0.410 | 0.040 |
| 06 | 0.280 | -0.580 | 0.290 | 0.034 |
| 07 | 0.420 | -0.530 | 0.130 | 0.024 |
| 08 | 0.560 | -0.410 | -0.060 | 0.012 |

## Front camber samples

| Sample | Normalized height `v` | Normalized depth `F(v)` |
|---:|---:|---:|
| 00 | 0.000000 | 0.220 |
| 01 | 0.166667 | 0.640 |
| 02 | 0.333333 | 0.910 |
| 03 | 0.500000 | 1.000 |
| 04 | 0.666667 | 0.930 |
| 05 | 0.833333 | 0.680 |
| 06 | 1.000000 | 0.280 |

## Grammar realization

- one translucent polygon-section `Loft` closes the reconstructed envelope;
- four yellow side contours retain upper, lower, front, and rear silhouette boundaries;
- two green top contours retain exterior crown and inboard reference lines;
- one orange front contour retains the nominal edge-on camber;
- seven cyan longitudinal curves and nine silver transverse curves expose the full 3D network;
- `Object`, `Interface`, and `Connect(AlignedWith)` preserve semantic ownership and evidence relationships.

## Construction-model improvements

1. replace the separable surface with a guide-curve `SurfaceLoft` that constrains belt-line highlight flow;
2. split the upper window frame from the lower Class-A skin at the belt interface;
3. derive the stamped inner panel from hinge, latch, intrusion-beam, regulator, speaker, and wiring load paths;
4. reserve a rolled-edge hem corridor and adhesive bead with local flange-angle variation;
5. model glass channels as a helical or barrel travel envelope rather than a planar slot;
6. add a datum graph for upper/lower hinges, latch, striker, check strap, and regulator rails;
7. carry uncertainty bands around manually traced side/top contours and weight later fitting by source confidence;
8. test mirror validity instead of assuming left/right equality around mirror, handle, and harness features;
9. validate seal compression around the J-surface and belt seal over every closure state;
10. calculate the swept opening volume and collisions with front fender, A-pillar, mirror, sill, and adjacent door;
11. tag exterior skin, window frame, hem, hinge zone, latch zone, intrusion-beam zone, and trim interface through triangulation;
12. use reflection-line and curvature-comb validation before calling the result Class-A.

## Fail-closed limits

- this is a local generic front-door contour, not a positioned vehicle assembly;
- the front contour is nominal because independent front-facing door evidence does not exist;
- hidden stamped construction, glass mechanism, hinge reinforcement, sealing, crash, and manufacturability are not certified;
- the reference sheet is generated artwork, not calibrated metrology photography.
