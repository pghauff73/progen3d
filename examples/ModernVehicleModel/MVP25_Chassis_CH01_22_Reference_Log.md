# MVP25 CH01-CH22 Assembled Chassis Reference Log

Generated on August 23, 2026.

## Primitive selection

`CH01` is a formed floor assembly, so one `CompoundShape` preserves an evidence-fitted formed envelope, hosted-opening floor sheet, formed centre tunnel, embossed beads, edge flanges, and longitudinal stamped cells as separately tagged construction features. `CH02` through `CH18` use `VariableSectionSweep`: closed boxes for primary rails and pillars, multi-cell boxes for rockers, open channels for roof rails, and hat sections for crossmembers. Explicit 3D centerline samples and cross-section stations preserve changing front, side, and top contours more closely than a constant profile. Projection-driven sweeps use `referenceAligned` frames; `CH04` and `CH11` retain `rotationMinimizing` carriers with station-derived section-centroid curves. `CH11` combines a 129-station centerline carrier with watertight orthogonal evidence cells because its labeled side and front views contain disconnected stamped-web regions. `CH19` through `CH22` remain tapered annular `ShellLoft` towers with section spacing no greater than 20 mm because their outer and inner section loops vary together through height.

The normal swept-member station cadence is no greater than 8 mm. `CH14` and `CH15` use an 8.8 mm maximum cadence to retain the full 256-column roof-rail evidence sampling. `MVP25_Chassis_CH01_22_Centreline_Sections.json` records every path, station count, maximum step, frame policy, atlas hash, and evidence derivation.

`CH15` is the accepted roof-rail geometry authority. `CH14` is reconstructed only as its horizontal left-hand mirror. The corrected CH14 sheet and corrected CH11-CH15 two-view atlas are visual derivatives; they do not override CH15 geometry.

- `CH01` FloorPan: `CompoundShape` using visual sheet `MVP25_CH01_FloorPan_ThreeView.png`; geometry evidence `CH01` via `direct`.
- `CH02` FrontRailLeft: `VariableSectionSweep` using visual sheet `MVP25_CH02_FrontRail_Left_ThreeView.png`; geometry evidence `CH02` via `direct`.
- `CH03` FrontRailRight: `VariableSectionSweep` using visual sheet `MVP25_CH03_FrontRail_Right_ThreeView.png`; geometry evidence `CH03` via `direct`.
- `CH04` RearRailLeft: `VariableSectionSweep` using visual sheet `MVP25_CH04_RearRail_Left_ThreeView.png`; geometry evidence `CH04` via `direct`.
- `CH05` RearRailRight: `VariableSectionSweep` using visual sheet `MVP25_CH05_RearRail_Right_ThreeView.png`; geometry evidence `CH05` via `direct`.
- `CH06` RockerLeft: `VariableSectionSweep` using visual sheet `MVP25_CH06_Rocker_Left_ThreeView.png`; geometry evidence `CH06` via `direct`.
- `CH07` RockerRight: `VariableSectionSweep` using visual sheet `MVP25_CH07_Rocker_Right_ThreeView.png`; geometry evidence `CH07` via `direct`.
- `CH08` APillarLeft: `VariableSectionSweep` using visual sheet `MVP25_CH08_APillar_Left_ThreeView.png`; geometry evidence `CH08` via `direct`.
- `CH09` APillarRight: `VariableSectionSweep` using visual sheet `MVP25_CH09_APillar_Right_ThreeView.png`; geometry evidence `CH09` via `direct`.
- `CH10` BPillarLeft: `VariableSectionSweep` using visual sheet `MVP25_CH10_BPillar_Left_ThreeView.png`; geometry evidence `CH10` via `direct`.
- `CH11` BPillarRight: `CompoundShape` using visual sheet `MVP25_CH11_BPillar_Right_ThreeView.png`; geometry evidence `CH11` via `direct`.
- `CH12` CPillarLeft: `VariableSectionSweep` using visual sheet `MVP25_CH12_CPillar_Left_ThreeView.png`; geometry evidence `CH12` via `direct`.
- `CH13` CPillarRight: `VariableSectionSweep` using visual sheet `MVP25_CH13_CPillar_Right_ThreeView.png`; geometry evidence `CH13` via `direct`.
- `CH14` RoofRailLeft: `VariableSectionSweep` using visual sheet `MVP25_CH14_RoofRail_Left_ThreeView.png`; geometry evidence `CH15` via `horizontal mirror to left-hand rail`.
- `CH15` RoofRailRight: `VariableSectionSweep` using visual sheet `MVP25_CH15_RoofRail_Right_ThreeView.png`; geometry evidence `CH15` via `direct`.
- `CH16` FrontCrossmember: `VariableSectionSweep` using visual sheet `MVP25_CH16_FrontCrossmember_ThreeView.png`; geometry evidence `CH16` via `direct`.
- `CH17` CentreCrossmember: `VariableSectionSweep` using visual sheet `MVP25_CH17_CentreCrossmember_ThreeView.png`; geometry evidence `CH17` via `direct`.
- `CH18` RearCrossmember: `VariableSectionSweep` using visual sheet `MVP25_CH18_RearCrossmember_ThreeView.png`; geometry evidence `CH18` via `direct`.
- `CH19` FrontSuspensionTowerLeft: `ShellLoft` using visual sheet `MVP25_CH19_FrontSuspensionTower_Left_ThreeView.png`; geometry evidence `CH19` via `direct`.
- `CH20` FrontSuspensionTowerRight: `ShellLoft` using visual sheet `MVP25_CH20_FrontSuspensionTower_Right_ThreeView.png`; geometry evidence `CH20` via `direct`.
- `CH21` RearSuspensionTowerLeft: `ShellLoft` using visual sheet `MVP25_CH21_RearSuspensionTower_Left_ThreeView.png`; geometry evidence `CH21` via `direct`.
- `CH22` RearSuspensionTowerRight: `ShellLoft` using visual sheet `MVP25_CH22_RearSuspensionTower_Right_ThreeView.png`; geometry evidence `CH22` via `direct`.

## Assembly model

The grammar publishes one root chassis assembly, one 22-sheet evidence collection, 22 purpose-named part objects, and one relationship graph. Every part owns exactly one geometry primitive. Twenty-two `AlignedWith` connections retain image provenance and 31 `FixedTo` connections define the chassis load-path graph.

## Evidence limits

- the JSON dimension schedule is authoritative;
- every imagegen sheet is visual construction evidence only;
- mirrored pairs remain separate CH objects and do not imply identical hidden stampings;
- holes, ribs, flanges, joints, welds, material grades, crush behaviour, and manufacturing tolerances are not certified;
- no crash, fatigue, torsion, NVH, corrosion, joining, or homologation claim is made.

## Artifacts

- manifest SHA-256: `043019eaa8a9676d98dcfe69b6310e869dac43789ca3e76d2b20501134a65d94`;
- grammar SHA-256: `c2e812862532db5f9576896fbb3c4cd8498f41e3c953431f48ba6bd6a38661ae`;
- frozen reference-mask manifest SHA-256: `9282628028a73db6ca748c2997459a824f891b7812437acb00a9ca6f898143ee`;
- acceptance command: `./tests/run_mvp25_chassis_ch01_22_assembly_checks.sh`.
