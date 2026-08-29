# MCP_OMv1 Grammar Examples

This directory contains current-parser examples for the planned Modern Car
Parametric Object Model v1.

- `MCP_OMv1_Executable_Family_Preview.p3d` displays the four MCSMv1 packages as
  lightweight procedural vehicle candidates.
- `MCP_OMv1_MVP26_Reference_Adapter.p3d` demonstrates the coordinate-frame,
  package, axle, source-evidence, and MVP2.6 adapter boundary for the reference
  variant.

These grammars are executable lowerings, not the canonical parametric model.
The canonical source remains the MCSMv1 variant and field definition until the
typed MCP_OMv1 C++ model and generation services are implemented.

Coordinate convention:

```text
+X = vehicle right
+Y = vehicle up
+Z = vehicle forward
origin = ground projection of package centre
```

Every instance supplies an explicit material argument. The examples avoid the
unsupported MCSMv1 target terms `ImplicitSectionField` and `IsoSurface`.
