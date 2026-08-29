# Curved Primitives

ProGen3D models curved primitives through two canonical families: `Cylinder` and `Sphere`. Partial, hollow, clipped, open, and sectioned variants are domain restrictions on those families rather than independent mesh classes.

## Normalized Geometry

- `Cylinder` has outer radius `0.5`, local-Y span `0..1`, and is centered on the local Y axis.
- `Sphere` has outer radius `0.5` and is centered at the local origin.
- `S(...)` owns external dimensions. Nonuniform scaling produces elliptical cylinders and ellipsoids.
- Radial values, wall thickness, clip offsets, and slab offsets are normalized against the canonical outer radius.
- Azimuth and polar values use degrees.

## Canonical Syntax

```p3d
I(
    Cylinder(
        radial(0.7 1)
        axial(0 1)
        azimuth(15 240)
        topology(shell)
        close(all)
        segments(64 1)
        mapping(cylindrical)
    )
    material(plaster)
    alpha(1)
    texscale(0.5)
)
```

Cylinder options are `radial`, `wall`, `axial`, `azimuth`, `chord`, `clip`, `topology`, `close`, `segments`, and `mapping`.

```p3d
I(
    Sphere(
        radial(0.84 1)
        clip(0 1 0 0 positive)
        topology(shell)
        close(none)
        segments(56 28)
        mapping(spherical)
    )
    material(plaster)
)
```

Sphere options are `radial`, `wall`, `polar`, `azimuth`, `clip`, `slab`, `topology`, `close`, `segments`, and `mapping`.

## Topology

- `topology(surface)` emits only the requested mathematical skin. It has no enclosed volume.
- `topology(solid)` requires inner radius zero and represents filled material.
- `topology(shell)` requires a positive inner radius and represents material between outer and inner skins.
- `wall(thickness)` is shorthand for a shell whose inner fraction is `1 - thickness`.

Density through `P(...)` is accepted only when the generated geometry has validated enclosed volume. Open or non-watertight geometry produces a diagnostic instead of falling back to a box volume.

## Closure

`close(...)` describes generated material boundaries:

- `axial`: Cylinder bottom and top.
- `angular`: start and end faces of a non-full azimuth domain.
- `clip`: faces created by half-space clips.
- `rim`: radial connections required by shell, polar, or angular boundaries.
- `all`: all supported closures.
- `none`: curved skins only.

Closed requested shapes must pass topology analysis: every geometric edge has two triangle incidents, face winding is consistent, no triangle is degenerate, and signed volume is finite. Open shapes retain only the boundaries omitted by policy.

## Chords and Plane Clips

`chord(angle offset side)` applies a 2D half-space to a Cylinder cross-section and extrudes the retained circle-segment region. It does not extrude only the chord line.

`clip(nx ny nz offset side)` applies an ordered local 3D half-space. The normal is normalized during validation. `slab(nx ny nz min max)` expands to two ordered parallel clips.

Polar restriction and plane clipping are not interchangeable. `polar(0 60)` creates radial boundaries directed toward the sphere center; `clip(0 1 0 0.5 positive)` creates a flat plane-cut cap.

## Aliases

Aliases compile to canonical descriptors and never own separate mesh generators:

- `Tube(inner(0.75))`
- `CylinderSector(sweep(180))`
- `DSection(angle(0) offset(0.25) side(positive))`
- `HalfCylinder(angle(0) side(positive))`, explicitly a center-chord half cylinder
- `Hemisphere(axis(y) side(positive))`
- `SphereCap(axis(y) offset(0.35) side(positive))`
- `SphereBowl(inner(0.85) axis(y) offset(0) side(positive))`
- `SphereQuarter`
- `SphereOctant`

The grammar editor autocompletes these aliases and their documented options. Hovering a procedural symbol shows its canonical expansion and defaults.

## Rendering, Export, and Physics

Preview, bounds, camera fitting, picking, and PLY export consume the same resolved mesh object. Curved normals use inverse-transpose transformation under nonuniform scaling. Local volume is analytic for unclipped domains and mesh-derived for validated clipped solids, then multiplied by the absolute determinant of the primary instance transform.

Collision policies are explicit:

- full solid Cylinder and Sphere use validated convex-mesh policy;
- simple closed partial solids use convex-mesh policy;
- shells and open surfaces are static-triangle-only;
- dynamic collision is not enabled for hollow, concave, or open geometry.

See `examples/curved_primitives_showcase.p3d` and `examples/curved_primitives_temporal.p3d`.
