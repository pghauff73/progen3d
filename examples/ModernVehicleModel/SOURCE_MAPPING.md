# MVPv2.5 Source Mapping

## Authoritative source and view mapping
- composite image: `MVP2.5sportyredAWD.png`
- SHA-256: `f10a70190e967c2b913113453d0468897df80c314a5f64fead0b90445a13f8e6`
- image extent: 1536 x 1024 pixels
- front: `(0, 0, 492, 509)` -> ProGen3D `front`
- back: `(492, 0, 428, 509)` -> ProGen3D `back`
- right: `(920, 0, 616, 509)` -> ProGen3D `right`
- left: `(0, 509, 492, 515)` -> ProGen3D `left`
- top: `(492, 509, 584, 515)` -> ProGen3D `top`

The executable grammar publishes these crop frames as `ReferenceImageEvidence`
interfaces and publishes the five matching ProGen3D camera directions as
`VehicleObservationSet` interfaces. The native MVPv2.5 observation model uses
the same composite-image path and crop identifiers.

## Generated final-assembly sheet
- image: `MVP25_FinalVehicle_SixView_Dimensioned_Assembly.png`
- SHA-256: `77b282c882fccc39be4f62be0f970b8329dcf56c1351f691de4620edb9dc8639`
- views: front, back, left, right, top, and bottom;
- content: exterior, body-in-white, chassis, apertures, closures, glass,
  hinges, struts, latches, seals, suspension towers, and wheel datums;
- exact machine-readable values: `MVP25_FinalVehicle_Dimension_Schedule.json`.

The generated engineering sheet uses the source vehicle only for visible
exterior identity. Hidden body-in-white and closure dimensions are nominal
engineering definitions and remain explicitly classified as inferred.

## Generated bonnet sheet
- image: `MVP25_Bonnet_SixView_Dimensioned_Assembly.png`
- SHA-256: `5513ba1f6fbfceb7f0e6445ebf2cd9384bbb34933f3e191fdcc643db33c21751`
- image extent: 1536 x 1024 pixels
- views: front, back, left, right, top, and bottom;
- assemblies: Class-A outer skin, stamped reinforcement, paired four-bar
  hinges, support strut, latch, safety catch, rolled-edge hem, and perimeter
  seal;
- sections: transverse `A-A`, longitudinal `B-B`, hem detail, and seal detail;
- exact machine-readable values: `MVP25_Bonnet_Dimension_Schedule.json`.
- executable bonnet-only grammar: `MVP25_Bonnet_Only.grammar`;
- grammar SHA-256: `d4e553432b452bea8a0c375bfa438f12f49df08aece2e77e9bbbb57569a8590d`;
- grammar validation record: `MVP25_Bonnet_Validation.json`;
- acceptance command: `./tests/run_mvp25_bonnet_grammar_checks.sh`.

The visible red exterior form is constrained by `MVP2.5sportyredAWD.png`.
Hidden reinforcement, fastening, hinge, strut, latch, safety-catch, hem, and
seal values are nominal inferred engineering definitions rather than claims of
direct photographic measurement.

## Bonnet 2D-to-3D contour reconstruction
- deterministic generator: `generate_bonnet_3d_contours.py`;
- generator SHA-256: `8e27a36b90a77027b026c928991b76055f078189520db39cfafae885d8b7d095`;
- source views: front, left-side, and top crops from
  `MVP25_Bonnet_SixView_Dimensioned_Assembly.png`;
- reconstruction definition: `MVP25_Bonnet_3D_Contour_Definition.json`;
- definition SHA-256: `9866e2a51a811ce6fcec5f701ca4e530d927d76fa782c9ca28004da52775fc04`;
- executable grammar: `MVP25_Bonnet_3D_Contours.grammar`;
- grammar SHA-256: `c9fc423497d9ef2d2677f023ac4d153663bbb438b68d1363095d4c8a4ee52143`;
- detailed theory and provenance: `MVP25_Bonnet_3D_Contour_Reference_Log.md`;
- validation record: `MVP25_Bonnet_3D_Contour_Validation.json`;
- acceptance command: `./tests/run_mvp25_bonnet_contour_grammar_checks.sh`.

The generator extracts the largest four-connected red-dominant component in
each crop, samples 11 longitudinal stations and nine transverse positions,
then creates a fused contour network. Raw top-view widths and front-view camber
remain in the definition. The executable widths use endpoint-constrained
neighbour relaxation to remove the single-pixel raster-tip collapse. The
executable camber separately uses a declared symmetric parabolic construction
prior so the loft has valid non-collinear profiles. This is an inferred contour
model, not calibrated photogrammetry or certified Class-A surface evidence.

## Front-door 2D-to-3D contour reconstruction
- component: left front door `FD-L` in a generic local frame;
- deterministic generator: `generate_front_door_3d_contours.py`;
- generator SHA-256: `052b4e65fecf51d9c99175c5304b3006b803910297b5453e660da25e11a0818f`;
- generated three-view authority: `MVP25_FrontDoor_ThreeView_Contour_Source.png`;
- contour-sheet SHA-256: `67743c1d91f02ab6ce1e53de2dca30e02bfea38774f536516c5d8bf3a1394552`;
- reconstruction definition: `MVP25_FrontDoor_3D_Contour_Definition.json`;
- definition SHA-256: `e01438bedc18e6e62fb545fdc9a1872086763d641bacc016ca7e87edacfaa12c`;
- executable grammar: `MVP25_FrontDoor_3D_Contours.grammar`;
- grammar SHA-256: `3a75fcfe04baf05ad5a5aae4bbf1bfa81b392f36d34177d0fb5f22981e7a6d77`;
- detailed theory: `MVP25_FrontDoor_3D_Contour_Reference_Log.md`;
- validation record: `MVP25_FrontDoor_3D_Contour_Validation.json`;
- acceptance command: `./tests/run_mvp25_front_door_contour_grammar_checks.sh`.

The side silhouette is manually traced from the annotated `FD-L` boundary in
the final-vehicle left view. The top view supplies longitudinal exterior-depth
variation. The vehicle front view sees the door edge-on, so its independent
front contour is not measurable; the generated front section is explicitly
nominal and pinned to the scheduled 42 mm depth. The 1120 mm length, 1220 mm
height, and 0.75 mm sheet thickness come from the dimension schedule. The
right front door is a mirror-placement candidate, not a claim that hidden left
and right construction is identical.

## Chassis 2D-to-3D contour reconstruction
- component: `ChassisAssembly` with CH01-CH22 and four wheel-mount datums;
- imagegen three-view source: `MVP25_Chassis_ThreeView_Contour_Source.png`;
- contour-source SHA-256: `89672f15b66d2e143acd5194e5fc7e524442fe26ef51725427fd447377e90a36`;
- deterministic generator: `generate_chassis_3d_contours.py`;
- generator SHA-256: `0421389de3f637a6ac5d5b7af483d2cc1e5d2961217bc8da9a8de4745005d714`;
- reconstruction definition: `MVP25_Chassis_3D_Contour_Definition.json`;
- definition SHA-256: `846bf4f4e9695eec498e53d8a88b15b7686bd5f8102cfe9e4546b5b68a7f7607`;
- executable grammar: `MVP25_Chassis_3D_Contours.grammar`;
- grammar SHA-256: `45f5372290f5f35e137f93174b841e419d9ca27f0c13905102467a75afb69cff`;
- detailed theory and limits: `MVP25_Chassis_3D_Contour_Reference_Log.md`;
- validation record: `MVP25_Chassis_3D_Contour_Validation.json`;
- acceptance command: `./tests/run_mvp25_chassis_contour_grammar_checks.sh`.

The side view supplies lower and upper height functions, the top view supplies
floor/shoulder/roof half-width functions, and the front view supplies a common
six-point section topology. Nine shared Z stations produce the fused inspection
`Loft`; 36 `SweepDisk` curves preserve the side/top/front outlines, CH member
centrelines, suspension towers, and wheel-mount datums. The dimension schedule
overrides generated labels and apparent raster scale. The generated source has
an inconsistent side-view direction glyph, so coordinate signs come from the
vehicle reference frame, scheduled datum coordinates, and native BIW source.
The package is contour evidence and construction intent, not crash-certified or
manufacturing-certified body-in-white geometry.
The acceptance command also opens the grammar in `progen3d-editor-gui` under
Xvfb and requires the 13-rule parse, FOV smoke, rendered frame, and full GUI
smoke markers.

## CH01-CH22 individual references and assembled chassis
- generated reference sheets: 22 individual 1536 x 1024 PNG files named
  `MVP25_CH01_FloorPan_ThreeView.png` through
  `MVP25_CH22_RearSuspensionTower_Right_ThreeView.png`;
- view content per sheet: side, front, and top construction views with the
  scheduled principal dimensions;
- deterministic artifact generator: `generate_chassis_ch01_22_assembly.py`;
- generator SHA-256: `9e824175297f33d4846d9f19cb437226fcf5bb22582780dbaa33e7841f6b5fe8`;
- image-hash and dimension manifest:
  `MVP25_Chassis_CH01_22_Reference_Manifest.json`;
- manifest SHA-256: `ea1615b7987d15fe2cc139dd20572ed7af8eb3a6fa660a2ad97a7a3c69b0a711`;
- executable assembly grammar: `MVP25_Chassis_CH01_22_Assembled.grammar`;
- grammar SHA-256: `7706fc2fbbdb6d0d72153e9aa6ec48f8fddd6aee5b317c05e4a4de066a3acf0b`;
- primitive and evidence rationale: `MVP25_Chassis_CH01_22_Reference_Log.md`;
- GL46 fit-to-extents preview:
  `MVP25_Chassis_CH01_22_Assembled_GL46_Isometric.png`;
- validation record: `MVP25_Chassis_CH01_22_Validation.json`;
- acceptance command:
  `./tests/run_mvp25_chassis_ch01_22_assembly_checks.sh`.

The assembled grammar uses a `ShellOffset` over a `SurfaceLoft` for CH01,
17 `SweepProfile` members for CH02-CH18, and four annular `ShellLoft` towers
for CH19-CH22. Twenty-two `AlignedWith` connections preserve one-to-one image
provenance, while 31 `FixedTo` connections encode the structural load-path
graph. The native gate resolves 22 triangle meshes owned by 22 part objects,
checks 25 total semantic objects, 53 total relationships, one-to-one geometry
bindings, scheduled representative dimensions, and the authored floor-pan and
suspension-tower placements. The imagegen sheets remain visual construction
references only; the JSON dimension schedule is authoritative.

## Collision positioning
- four wheel assemblies begin 0.240 m above their resolved road position;
- every wheel publishes a `roadContact` support interface;
- every wheel uses `Position(... mode(Drop) ...)` against
  `MVPV25_GROUND_REFERENCE.roadSurface`;
- resolved wheel centres are at Y=0.335 m;
- axle stations are Z=+1.325 m and Z=-1.325 m, preserving the 2.650 m wheelbase;
- the deterministic runtime gate records four successful placement operations
  and 24 collision queries.

## Directly supported by the five-view image sheet
- red five-door sporty hatchback
- AWD
- 4.350 m overall length
- 1.800 m overall width
- 1.430 m overall height
- 2.650 m wheelbase
- black roof / dark roof insert
- black multi-spoke wheels
- red calipers
- swept front lamps
- wide dark grille
- front splitter
- side skirts
- wheel arches and rear haunch
- rear spoiler
- rear lamps
- diffuser
- quad circular exhaust
- visible side door seams
- visible side-glass perimeter
- front/rear/side/top silhouette constraints

## Inferred
- exact hinge coordinates
- B-lines and J-surfaces
- rolled edge and flange geometry
- weatherstrip sections
- window barrel surfaces
- helical window-drop parameters
- door inner construction
- bonnet four-bar geometry
- gas strut mounts
- BIW member sections
- suspension hardpoints
- suspension type details
- AWD driveline geometry
- interior geometry

The grammar marks uncertain hidden geometry as inferred where practical. The
reference image constrains visible form; it does not falsely certify hidden
mechanisms that are not visible in the five panels.
