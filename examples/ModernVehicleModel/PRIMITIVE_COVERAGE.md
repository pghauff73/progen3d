# MVPv2.5 Primitive Coverage

Evidence/Fitting:
ViewObservation, EvidenceWeight, VehicleLandmark, AutomotiveWireframe,
VehicleShapePrior, AutomotiveCharacterCurve, FitObjective.

Executable reference evidence:
ReferenceImageEvidence, VehicleObservationSet, composite image extent,
front/back/right/left/top crop interfaces, ProGen3D view interfaces.

Class-A:
ClassASurfacePatch, PatchConnection, HighlightFlowValidator.

Closures:
BodySideAperture, DoorEgressSurface, AutomotivePanelGap, RolledEdge,
AutomotiveClosure, ClosureHingeStudy, FourBarClosureJoint,
RevoluteClosureJoint, VariableSealSweep, TelescopingLink, StampedSheetPanel.

Glass:
SideGlassSurface, BarrelSurface, GlassChannel, HelicalGlassDrop,
BeltSeal, MirrorGlassSystem.

BIW:
AutomotiveStructuralMember, VariableSectionSweep, SheetPanel, BIWJoint.

Suspension:
SuspensionHardpoint, KinematicLink, SuspensionCorner, WheelPoseSolver,
WheelSweptEnvelope, WheelHouse.

Geometry:
CurvedPanel, ShellLoft, SurfaceLoft, SweepProfile, SweepDisk, Revolve,
RadialArray, SurfacePattern, HostedOpening, CompoundShape.

Bonnet-only executable grammar:
`MVP25_Bonnet_Only.grammar` uses SurfaceLoft plus ShellOffset for BN01 and
BN02, SweepProfile for reinforcement ribs and BN07, SweepDisk for BN03, BN04,
BN05, and BN06, and Extrude for brackets, latch bodies, and BN08. Object,
Interface, Connect, and Joint preserve component ownership and attachment
semantics. The focused runtime gate resolves 29 meshes owned by 16 objects,
with six connections and three joints.

Bonnet contour reconstruction grammar:
`MVP25_Bonnet_3D_Contours.grammar` uses one polygon-section Loft as the fused
envelope, two top-derived SweepDisk edges, one side-derived SweepDisk crown,
nine longitudinal SweepDisk contours, and eleven transverse SweepDisk
contours. Object, Mate interfaces, and Connect(AlignedWith) preserve the
source-to-reconstruction relationship graph. The focused runtime gate resolves
24 parameterized triangle meshes owned by eight objects with four validated
connections, and repeated generator execution is byte-for-byte deterministic.

Front-door contour reconstruction grammar:
`MVP25_FrontDoor_3D_Contours.grammar` uses one polygon-section Loft for the
closed contour envelope, four side-derived SweepDisk boundaries, two
top-derived SweepDisk contours, one front-derived SweepDisk camber, seven
longitudinal SweepDisk contours, and nine transverse SweepDisk contours.
Object, Mate interfaces, and five Connect(AlignedWith) relationships preserve
side/front/top evidence ownership. The focused gate resolves 24 parameterized
triangle meshes owned by nine objects, and the source sheet, JSON, grammar, and
reference log regenerate byte-for-byte.

Chassis contour reconstruction grammar:
`MVP25_Chassis_3D_Contours.grammar` uses one nine-section polygon Loft for the
fused side/front/top inspection envelope and 36 SweepDisk curves for the view
contours, CH01-CH22 member centrelines, suspension towers, and wheel-mount
datums. Twelve semantic objects own all 37 parameterized triangle meshes, and
eight Connect(AlignedWith) relationships retain the evidence-to-envelope graph.
The native gate checks the scheduled 3.100 m floor length, 1.420 m floor width,
2.650 m wheelbase, 1.570 m front track, 1.560 m rear track, and byte-for-byte
regeneration of the definition, grammar, and reference log.
The same acceptance script requires the editor to parse 13 rules and pass its
FOV, rendered-frame, and complete GL preview smoke markers.

CH01-CH22 assembled chassis grammar:
`MVP25_Chassis_CH01_22_Assembled.grammar` gives every scheduled chassis part
one purpose-fitted `ShapeSpecification`: CH01 uses `ShellOffset` with a
`SurfaceLoft` source, CH02-CH18 use 17 `SweepProfile` members, and CH19-CH22
use four annular `ShellLoft` towers. The semantic model contains the root
assembly, the 22-sheet evidence collection, 22 purpose-named part objects, and
the relationship graph. Twenty-two `Connect(AlignedWith)` relationships bind
each part to its individual side/front/top image, while 31 `Connect(FixedTo)`
relationships retain the chassis assembly load path. The native gate validates
22 triangle meshes, 25 semantic objects, 53 connections, 22 one-to-one geometry
bindings, representative scheduled dimensions, authored suspension-tower
placements, and repeatable generation. The GL46 gate parses 26 rules, selects
the isometric camera, fits extents, and produces a deterministic preview.

Collision positioning:
Object, Interface, Position(Drop), road-surface Bearing interface,
transactional collision resolution records and deterministic evidence hashes.

State/Validation:
VehicleState, Interface, Joint, closure sweep tests, glass travel tests,
Class-A continuity tests, wheel envelope tests, reprojection residuals,
five-view GL46 captures, reference silhouette comparison, fit-to-extents checks.
