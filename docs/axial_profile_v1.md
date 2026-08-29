# AxialProfilev1

`AxialProfile` is a core procedural geometry family. It separates evaluated shape specification, deterministic mesh construction, cached resolved geometry, and scene-instance state.

## Language

```p3d
I(
    AxialProfile(
        axis(y)

        profile(Base polygon(
            -0.5 -0.5
             0.5 -0.5
             0.5  0.5
            -0.5  0.5
        ))

        profile(Shaft polygon(
            -0.3 -0.3
             0.3 -0.3
             0.3  0.3
            -0.3  0.3
        ))

        at(0 Base)
        hold(1)
        step(Shaft)
        hold(3)
        linear(4 Shaft center(0.1 -0.05) scale(0.8 1.1) rotate(12))
        cap(all)
    )
    material(plaster)
)
```

Statements must appear in this order:

1. `axis(y)` exactly once.
2. One or more named `profile(Name polygon(x z ...))` declarations.
3. One initial `at(position Profile ...)` state.
4. Zero or more `hold`, `linear`, or `step` transitions.
5. Optional final `cap(none|all|bottom|top)`; the default is `all`.

`at` and `linear` accept an absolute axial position, a profile name, and optional `center(x z)`, `scale(x z)`, and `rotate(degrees)` section transforms. `hold(position)` preserves the previous section exactly. `step(Profile ...)` replaces the section at the current axial position and creates the exposed horizontal transition surface.

All numeric fields are ordinary grammar expressions evaluated during expansion. The mesh generator never samples randomness.

## v1 Invariants

- Y is the only supported construction axis.
- Every profile is one closed, simple outer polygon with at least three vertices.
- Duplicate terminal vertices are removed; zero edges, collinear triples, zero area, and self-intersection are rejected.
- Canonical profile winding is counter-clockwise in local XZ coordinates.
- All v1 profiles use the same semantic vertex count and retain index correspondence.
- Scale is finite, non-zero, and orientation-preserving.
- Non-step axial states are strictly increasing.
- Step profiles must be strictly nested; crossing, touching, coincident, or disjoint contours are rejected.
- Closed geometry is triangulated deterministically and checked for degenerate, boundary, and nonmanifold edges before publication.

## Safety Ceilings

| Resource | Limit |
| --- | ---: |
| Profiles | 128 |
| Vertices per profile | 256 |
| Total declared profile vertices | 32,768 |
| Axial states | 512 |
| Step transitions | 256 |
| Generated vertices | 131,072 |
| Generated triangles | 262,144 |

Pathological specifications are rejected before mesh allocation.

## Rendering and Evidence

AxialProfile uses the same resolved triangle-mesh path as parameterized Cylinder, Sphere, and imported mesh geometry. Preview, export, bounds, picking, mesh-derived volume, and immovable triangle collision all consume the resolved mesh.

Dynamic collision is deliberately not promoted in v1. `AxialProfile` reports `StaticTriangleMesh`; a dynamic instance is therefore excluded from dynamic collision rather than silently receiving box inertia or box collision.

The scene inspector reports profile, level, transition, mesh, closure, volume, collision, and topology-hash evidence. The preview overlay can display section rings and centerlines with blue hold, green linear, and orange step states.

## Validation Tools

```bash
tools/axialprofile/collada_axial_profile_analyzer.py source.dae \
    --geometry-id ID3050 \
    --output analysis.json

tools/axialprofile/collada_axial_profile_fit.py analysis.json \
    --output candidate.p3d

tools/axialprofile/compare_axial_profile_mesh.py analysis.json generated.ply \
    --output comparison.json
```

The fitter emits an explicitly unapproved candidate with source SHA-256 provenance. It does not promote or certify the result.

## Verification

```bash
./tests/run_axial_profile_geometry_checks.sh
./tests/run_axial_profile_grammar_scene_checks.sh
./tests/run_axial_profile_editor_checks.sh
./tests/run_axial_profile_gui_smoke_check.sh
./tests/run_axial_profile_temporal_gui_smoke_check.sh
./tests/run_axial_profile_evidence_checks.sh
```

`tests/fixtures/axialprofile/component24_validation_manifest.json` records the external Component 24 evidence gate. As of August 21, 2026, this checkout contains no approved `Component_24`, `ID3050`, Coruscant, COLLADA, or `.dae` source asset. The implementation is therefore candidate-complete, while core-promotion validation remains pending source provenance and quantitative comparison.
