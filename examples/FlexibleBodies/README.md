# Flexible wires, belts and sheets

Open [WireBeltSheet.p3d](WireBeltSheet.p3d) in the native desktop editor and start the physics timeline. The wire hangs between two pins, the belt circulates and settles on two guides, and the membrane sags between four pinned corners. Step Physics advances the same model used by playback. Reset Sim restores the authored positions and initial velocities.

## Author an object

Place the physical declarations before the instance. Rule parameters and arithmetic expressions work in the numeric fields. The material in the instance controls its appearance. The physical properties come from `FlexMaterial`.

```text
Start -> G(9.81 0 0 -1) Cable(1.2)

Cable(span) ->
    FlexDimensions(span 0.008 0.04) FlexGrid(24 0)
    FlexMaterial(1200 5000000 2)
    FlexPin(0 0) FlexPin(24 0)
    I(FlexibleWire material(copper) 1 1)
```

| Instance | FlexDimensions arguments in metres | FlexGrid and pin addressing |
|---|---|---|
| `FlexibleWire` | End-to-end X span, circular diameter, initial midspan sag towards local −Z. Rest length follows the sampled centreline. | `FlexGrid(n 0)` creates n+1 nodes. `FlexPin(i 0)` fixes a node. |
| `FlexibleBelt` | Guide centre distance along X, belt width along Y, belt thickness. `FlexGuideRadius(r)` sets the common pitch radius in the XZ plane. | `FlexGrid(n m)` creates n rows with m+1 nodes per row. The return row connects to row 0. Use n divisible by four, at least eight. |
| `ElasticSheet` | Length along X, width along Y, thickness. The rest mid-surface lies in the XY plane with its width centred at Y=0. | `FlexGrid(n m)` creates (n+1) × (m+1) nodes. `FlexPin(i j)` addresses one node. |

`FlexMaterial(density young damping)` takes kg/m³, Pa and s⁻¹. The default values are 1100, 5,000,000 and 1. These are illustrative effective properties. For an assembly such as an insulated stranded wire or reinforced belt, use measured effective properties when comparing predictions with hardware.

`FlexBeltSpeed(v)` initialises belt circulation in m/s. A positive value moves the upper run along +X. The guides are fixed, frictionless cylinders that constrain the radial positions of belt nodes. Between nodes, the wrap follows chords. A finer longitudinal grid reduces that chord error. Damping slows circulation after launch. The model does not calculate driven-pulley torque, slip or tooth engagement.

The default grids are 24 × 0, 32 × 1 and 12 × 8 for wire, belt and sheet respectively. Without `FlexPin` the wire and sheet are free. A pin holds its initial world location. Use `T` and `A` for placement and positive uniform `S` for scaling. Specify independent dimensions through `FlexDimensions`. `P` overrides density, and `M` overrides total particle mass when a density override is absent. `V` and `VR` supply initial translation and spin to free nodes. `!I` keeps the instance fixed.

`FlexSceneUnits(1000)` displays SI geometry in a millimetre assembly. The default is 1. The conversion affects the local render mesh only. Specify lengths, velocity, acceleration and material properties in SI as before. Ordinary uniform scope scaling still resizes the physical body.

## Mechanical model

Each instance owns its particles, velocity state, pins and elastic constraints. Wire mass is the sum of density × circular area × segment length. Surface mass comes from density × thickness × discrete mid-surface area. End and boundary nodes receive their tributary share. Uniform scaling by a factor s multiplies the physical mass by s³ at fixed density.

Distance constraints use compliance L/(EA). Three-node angle constraints use compliance ℓ/(EI), where ℓ is the average spacing beside the hinge. For a round wire, I = πd⁴/64. The ribbon grid uses width-weighted axial and transverse strips plus symmetric diagonal shear bracing. Its response is an effective lattice approximation. It does not implement a calibrated nonlinear shell, plasticity, wire torsion, self-contact or collisions with other scene primitives.

The XPBD solver uses substeps targeting 1/240 s, four projection sweeps per substep and exponential velocity damping. It limits a single scene update to 240 substeps. Use ordinary interactive time steps and compare grid refinements when inspecting a result. Preview, picking bounds and mesh export use the current deformed surface. Pins and the two ideal belt guides are the supported boundary conditions.

## Reproduce the checks

Run `make flexible-body-check` from the project root. The suite compiles the example through the native grammar, steps it, checks closed and consistently wound meshes, and verifies reset/replay, mass scaling, rotated gravity, pinning, guide clearance, circulation and invalid declarations.

For a straight wire hanging in its axial direction under distributed self-weight, the small-strain tip extension is ρgL²/(2E). With L = 1 m, ρ = 1000 kg/m³ and g = 9.81 m/s², the expected extensions are 49.05 mm at 100 kPa and 24.525 mm at 200 kPa. The test exercises both cases at 60 and 120 scene updates per second. A separate single-hinge bending check compares the equilibrium tip displacement against Wℓ³/(EI) at two wire diameters.

The projection equations follow [Macklin, Müller and Chentanez, XPBD](https://mmacklin.com/xpbd.pdf). The substepping approach is described in [Macklin et al., Small Steps in Physics Simulation](https://diglib.eg.org/bitstream/handle/10.1145/3309486-3340247/02-14-macklin.pdf).
