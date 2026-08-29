# MVP25 Chassis 2D-to-3D Contour Reference Log

Generated on August 22, 2026.

## Evidence hierarchy

1. `MVP25_FinalVehicle_Dimension_Schedule.json` is authoritative for CH01-CH22 dimensions and wheel-datum coordinates.
2. `MVP25_Chassis_ThreeView_Contour_Source.png` is the imagegen-created side/front/top contour reference and has SHA-256 `89672f15b66d2e143acd5194e5fc7e524442fe26ef51725427fd447377e90a36`.
3. `MVP25_FinalVehicle_SixView_Dimensioned_Assembly.png` constrains assembly context and has SHA-256 `77b282c882fccc39be4f62be0f970b8329dcf56c1351f691de4620edb9dc8639`.
4. `src/vehicle/service/RedAwdHatchbackMvp25Builder.cpp` supplies existing engineering placement priors and has SHA-256 `8f6de9afead1518145f20446d3bf91f5125c23b7ab344d068676f07bebd7f2f9`.

The generated image is not allowed to override the schedule. Its side-view direction glyph is visually inconsistent with the labelled front/rear datum positions, so coordinate signs come from the schedule and native vehicle frame, not from that glyph.

## Fusion theory

The side view supplies lower and upper height functions over longitudinal station `z`. The top view supplies floor, shoulder, and roof half-width functions over the same stations. The front view supplies a stable closed six-point cross-section topology. Combining those three independent 2D constraints produces nine transverse `Polygon` sections. `Loft` realizes an inspection envelope and `SweepDisk` preserves the source contours and semantic CH member centre-lines as selectable geometry.

The resulting model is deliberately dual-layered:

- the translucent loft communicates the fused three-view envelope;
- the named CH01-CH22 contour families communicate construction intent without pretending the envelope is a stamped monocoque.

## Construction-model improvements

1. Replace line-centre members with `SweepProfile` using scheduled rectangular or hat sections and explicit wall thickness.
2. Represent CH01 as a stamped shell with beads, tunnels, seat mounts, drain holes, and locally thickened interfaces rather than a flat panel.
3. Model every pillar and roof rail as a multi-cell section whose section orientation follows a rotation-minimising frame.
4. Create a joint graph for spot welds, laser welds, adhesive beads, rivets, overlap lengths, and load-path continuity.
5. Treat wheel-mount datums as constrained frames with camber, caster, toe, kingpin inclination, and tolerance covariance, not points alone.
6. Add crush initiators and progressive section changes to front and rear rails instead of uniform beams.
7. Build suspension towers as shell lofts around strut-top interfaces with double-skin reinforcements and service clearances.
8. Separate nominal Class-A package envelope, body-in-white outer envelope, intrusion envelope, and manufacturing springback allowance.
9. Fit later contour stations with uncertainty bands so generated-raster traces carry lower authority than dimensions and native datums.
10. Validate torsional load paths, local buckling, aperture diagonal stiffness, roof crush paths, and side-impact load transfer before structural certification.
11. Preserve semantic surface tags such as `CH10.inner`, `CH10.outer`, `CH10.hingeReinforcement`, and `CH10.beltAnchorZone` through triangulation.
12. Use transactional geometry: retain the last validated chassis if any new loft section self-intersects, changes winding, violates symmetry, or misses a datum.

## Generated artifacts

- reconstruction definition: `MVP25_Chassis_3D_Contour_Definition.json`, SHA-256 `846bf4f4e9695eec498e53d8a88b15b7686bd5f8102cfe9e4546b5b68a7f7607`;
- executable grammar: `MVP25_Chassis_3D_Contours.grammar`, SHA-256 `45f5372290f5f35e137f93174b841e419d9ca27f0c13905102467a75afb69cff`;
- native acceptance: `tests/run_mvp25_chassis_contour_grammar_checks.sh`.

## Fail-closed limits

- this is a contour reconstruction and semantic centre-line model, not a validated crash structure;
- the imagegen sheet is generated artwork, not calibrated orthographic metrology;
- hidden section profiles, joints, reinforcements, materials, tolerances, and manufacturing operations are not certified;
- no finite-element, fatigue, crash, NVH, corrosion, joining, or homologation claim is made.
