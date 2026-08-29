# Required Progen3D Operators

This advanced grammar intentionally targets the proposed next-generation geometry/reasoning stack.

## Geometry kernel
- Profile2D
- Curve3D / Bezier3 / Polyline3
- ExtrudeProfile
- ExtrudeProfileWithVoids
- SweepProfile
- SweepDisk
- Revolve
- SurfaceLoft
- ShellLoft
- RoundedPanel / RoundedExtrude
- CurvedPanel
- FoldedProfile
- LayerSet
- SurfaceTiling
- HeightField
- PanelBox / PanelArray
- InstanceArray / RadialArray / GridArray
- RepeatExtrusion / RepeatProfileOnSurface

## Architectural reasoning
- Program
- StyleState
- Datum
- Space
- HostedWall / HostedOpening
- PositionConstraint
- Connection
- CollisionRule
- validation gates

## Vegetation
- BranchGraph
- TaperedSweep
- LeafBlade
- OrganArray
- Phyllotaxis
- LSystem
- SpaceColonization
- VinePath
- SurfaceAttachment

## Vehicle
- VehiclePackage
- VehicleDatums
- BodySection
- GuideCurve
- SurfaceLoft / BodyShell
- WheelArch
- PanelCut
- CurvedPanel
- WheelAssembly
- RadialArray
- articulated joints later

## Current-parser compatibility
The present Progen3D parser does not implement these operators. This file is therefore a target/reference grammar for the implementation roadmap, not a current executable scene.
