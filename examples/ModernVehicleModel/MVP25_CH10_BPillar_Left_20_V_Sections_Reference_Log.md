# MVP25 CH10 B-Pillar Left 20 V-Section Reference Log

Generated on August 23, 2026.

## Purpose

`MVP25_CH10_BPillar_Left_20_V_Sections.png` is an imagegen-created engineering sheet showing twenty CH10 cross-sections as the `VariableSectionSweep` parameter `V` changes. `V01` is the bottom rocker attachment at `V=0`; `V20` is the top roof attachment at `V=1`. The intermediate stations are equally spaced in normalized sweep parameter and therefore 51.5789 mm apart over the 980 mm path.

## Evidence and authority

1. `MVP25_CH10_BPillar_Left_ThreeView.png` constrains the visible B-pillar form and nominal 110 mm by 85 mm section.
2. `MVP25_Chassis_CH01_22_Frozen_Reference_Masks/CH10_front.png` supplies the section-width envelope and centre offset at each station.
3. `MVP25_Chassis_CH01_22_Frozen_Reference_Masks/CH10_side.png` supplies the section-depth envelope and centre offset at each station.
4. `MVP25_CH10_BPillar_Left_20_V_Sections.json` is authoritative for the twenty sampled station values.
5. The PNG is visual construction evidence and must not override the JSON schedule.

The schedule uses the same vertical-envelope sampling rules as the assembled CH10 `VariableSectionSweep` grammar. Its path coordinate increases in `+Y` from 210 mm to 1190 mm in the vehicle reference frame. Section views look along `+Y`, with `+X` right and `+Z` forward.

## Artifacts

- source three-view SHA-256: `a0a7fd054bd804aee5fd8dc6846e866f06780d114babda09bb77dac2664f367a`;
- twenty-station JSON SHA-256: `6e47a69d3364b732e6970507c9834b015ec55bc00dd2fd4544253754efe142cd`;
- imagegen section-sheet SHA-256: `2d7e5a4290aa03671ac9fafe7b00fec195c5c0f48ad263f5f1d644a42d7042c4`;
- reproducible schedule generator: `generate_ch10_v_section_schedule.py`.

## Construction limits

- The twenty values define outer rectangular section envelopes, not certified stamping tool surfaces.
- The repeated thin-wall profile communicates plausible closed-section construction but does not infer hidden flanges, ribs, holes, reinforcements, joints, spot welds, adhesives, or material grades.
- The 1.2 mm sheet thickness is the nominal CH10 schedule value and is not a local-gauge survey.
- No crash, fatigue, torsion, buckling, NVH, corrosion, joining, manufacturability, or homologation claim is made.
