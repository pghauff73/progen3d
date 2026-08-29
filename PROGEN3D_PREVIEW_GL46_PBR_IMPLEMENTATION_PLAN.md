# ProGen3D Preview GL46 and PBR Implementation Plan

Date: 2026-08-22

## Objective

Upgrade the preview canvas with constant-velocity keyboard navigation, position-pivoted view rotation,
optional picking/highlighting, OpenGL 4.6 rendering, higher-resolution physically
based materials, and corrected glass transparency and reflectiveness.

## Implemented Object Model

- `PreviewNavigationInput` represents held preview-navigation actions.
- `PreviewInteractionSettings` owns picking visibility and navigation velocities.
- `PreviewCameraMotionController` advances a `PreviewCameraController` from input
  and elapsed time without frame-rate-dependent movement or rotation.
- `OpenGl46CapabilityValidator` rejects contexts below OpenGL 4.6 with an explicit
  runtime diagnostic.
- `PreviewSurfaceMaterial` owns the PBR factors and composed texture maps used by
  the renderer.

## Implementation Phases

1. Add W/A/S/D/Q/E movement and Z/C left/right rotation around the camera position.
2. Add a preview checkbox that suppresses hit testing and all selection outlines.
3. Require an OpenGL 4.6 Core context and migrate every shader to GLSL 460 Core.
4. Replace the fixed 128-pixel procedural material size with selectable 256, 512,
   1024, and 2048 resolutions.
5. Bind base-color, normal, roughness, metallic, ambient-occlusion, alpha, height,
   emissive, and environment maps to the material shader.
6. Use energy-conserving GGX lighting, clearcoat, sheen, anisotropy, subsurface
   wrapping, environment reflection, and Fresnel transmission.
7. Render transparent surfaces back-to-front with premultiplied-alpha blending and
   depth writes disabled.

## Glass Contract

Glass materials expose opacity, transmission, index of refraction, thickness,
attenuation color, roughness, reflectance, normal strength, and clearcoat. Clear,
smoky, frosted, crystal, polished, and mirror descriptors alter those properties.
Reflection uses the active cubemap when available and a deterministic procedural
environment otherwise.

## GL46 Lighting Completion

The preview lighting path now uses a purpose-driven scene and electrical object
model rather than hard-coded shader lights:

- `SceneLight` represents directional, point, spot, and rectangular-area light
  definitions with photometry, provenance, reference frame, and shadow policy.
- `PreviewLightCollection` owns the editable light set and deterministic studio
  defaults.
- `PreviewLightBuffer` uploads 80-byte `GpuLightRecord` values through an OpenGL
  4.6 shader-storage buffer bound at binding point 2.
- the GLSL 460 lighting shader reads as many as 32 visible lights and applies
  directional, point, spot, and bounded rectangular-area fallback evaluation.
- `LightFixture`, `LightSwitch`, `LightingCircuit`, and the electrical evaluator
  preserve fixture-control relationships independently of rendering.
- the Lights and Electrical Controls panels expose editing, presets, gizmos,
  picking, circuit state, validation, and sidecar persistence.
- grammar declarations lower `Light`, `LightFixture`, `LightSwitch`,
  `LightingCircuit`, `Controls`, and `LightingArray` into the scene model.

Shadow allocation is deterministic and bounded at one directional, four spot,
and two point-light requests. The current renderer produces one primary shadow
depth map; additional accepted spot slots are reserved architecture rather than
independent rendered shadow maps. Forward+ is recommended by evidence when more
than 24 GPU lights are visible, but remains intentionally deferred until a
measured workload justifies the additional render path. Multi-map shadow arrays
and LTC rectangular-area evaluation also remain later work.

The modern-residence grammar now realizes 27 lights, 27 fixtures, 6 switches,
and 9 circuits. Three spot lights request shadows, all three requests receive a
bounded allocation, and 27 GPU records occupy 2160 SSBO bytes. Its deterministic
lighting-state hash is `7986025927890646612`.

## Verification Evidence

- `tests/run_preview_session_checks.sh` compiles and runs deterministic camera,
  projection, timeline, and selection model checks.
- The camera tests compare one second of movement at 30, 60, and 144 frames per
  second and validate constant position-preserving rotation velocity.
- Static renderer gates require GLSL 460, environment-map sampling, physical
  transmission uniforms, premultiplied-alpha blending, and selectable material
  resolution.
- `make -j2` proves the complete editor compiles and links with the new model and
  renderer surfaces.
- GUI smoke and visual capture runs prove OpenGL 4.6 context creation, shader
  compilation, scene generation, and framebuffer capture on a GL46-capable runtime.
- `tests/run_lighting_model_checks.sh`,
  `tests/run_lighting_scene_persistence_checks.sh`, and
  `tests/run_lighting_grammar_scene_checks.sh` prove the scene model, electrical
  graph, sidecar round trip, grammar lowering, and scope-relative placement.
- `tests/evidence/modern_luxury_residence_lighting_evidence_2026-08-22.json`
  records the complete residence lighting and shadow-allocation state.

## Acceptance Gate Result

Passed on August 22, 2026 using a Mesa 26.1.4 OpenGL 4.6 Core context with GLSL
4.60. The modern-residence visual test completed scene generation, FOV checks,
frame rendering, and framebuffer capture without shader or program diagnostics.
The final editor binary SHA-256 is
`b83020f8c844248eea6ccab28bc8365170c11742356cebc5415f1d12a43df288`.
The deterministic front and rear residence overlays have SHA-256 values
`038f7426e3db3b891d54382cfdec43c2d761eaa60a868e250344f985cb54db1c`
and `69b3c66e199fff6d54a774c1947c9b712ddbb227f6a88bf852c1be967723c6fb`.
The lighting, placement, and visual evidence JSON SHA-256 values are respectively
`76c56b91b3a5cf6ec3336536110fff3ba28588dd918b8c5eaa97df9bcb5e92f3`,
`59e429a8ef77b68a45c85dca2d5147b6473e6652562b77aaee1a03cd8233187d`,
and `cdbe6592ce3429c2f7378f8d168ef934ddd47059d0b664448b3d17fe285be851`.
A machine exposing OpenGL 4.5 or earlier fails explicitly rather than silently
downgrading the renderer.
