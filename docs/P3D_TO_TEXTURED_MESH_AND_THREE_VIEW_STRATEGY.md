# P3D Textured Mesh and Orthographic Three-View Strategy

**Strategy date:** 2026-08-28
**Proposed commands:** `p3d_to_3dtexmesh`, `p3d_to_3view`

## 1. Purpose

This strategy defines two native C++ conversion tools:

1. `p3d_to_3dtexmesh` compiles a `.p3d` grammar into a deterministic textured
   mesh package and writes OBJ, STL, or PLY output in every encoding that the
   selected format actually supports.
2. `p3d_to_3view` compiles the same `.p3d` grammar and writes deterministic
   front, side, and top PNG images using orthographic projection and automatic
   zoom-to-extents framing.

Both commands must use one frozen generated-scene snapshot. Geometry exported
to a mesh and geometry rendered into the three views must therefore come from
the same primitive instances, transforms, material slots, and evaluated grammar
state.

## 2. Existing Repository Baseline

The implementation should extend the following current owners rather than
create a second grammar or preview runtime:

- `SceneGenerationContext` owns evaluated `ScenePrimitiveInstance` objects and
  the ordered `SceneMaterialCatalog`.
- `ScenePrimitiveVertexAssemblyService` already produces the required render
  vertex tuple: world position, world normal, and texture coordinate.
- `SceneExportMeshAssemblyService` currently collapses that tuple into a
  position-only `Mesh`; it is suitable for legacy export but is not the source
  model for a textured export.
- `PreviewObjectCameraViewService` already isolates direct object geometry or
  an object containment hierarchy.
- `PreviewObjectCameraFramingService` already targets an object's bounds center,
  sets horizontal views to zero elevation, and selects orthographic projection.
- `ScenePreviewCameraFrameFactory` already creates a `glm::ortho` projection.
- `PreviewFramebufferCaptureService` already reads deterministic framebuffer
  pixels and hashes them, but currently writes PPM rather than PNG.
- `ThreeViewProjectionService` establishes the compatible view orientation:
  front `(x, y)`, side `(-z, y)`, and top `(-z, x)`.
- `third_party/debs/stb/usr/include/stb/stb_image_write.h` is available for a
  native C++ PNG writer.

The root export defect is loss of semantic data after render-vertex assembly.
The root texture defect is that preview materials are represented primarily by
OpenGL texture handles after creation. Export needs canonical CPU texture
assets before any GPU upload occurs.

## 3. Command Contracts

### 3.1 `p3d_to_3dtexmesh`

Example invocations:

```bash
p3d_to_3dtexmesh \
  --input examples/SimpleModernBuilding/SMBv3.grammar \
  --format obj \
  --encoding ascii \
  --texture-mapping bake-atlas \
  --texture-resolution 2048 \
  --output build/SMBv3/SMBv3.obj

p3d_to_3dtexmesh \
  --input examples/SimpleModernBuilding/SMBv3.grammar \
  --format stl \
  --encoding binary-little-endian \
  --texture-policy sidecar \
  --output build/SMBv3/SMBv3.stl

p3d_to_3dtexmesh \
  --input examples/SimpleModernBuilding/SMBv3.grammar \
  --format ply \
  --encoding ascii \
  --texture-mapping bake-atlas \
  --output build/SMBv3/SMBv3.ply

p3d_to_3dtexmesh \
  --input examples/SimpleModernBuilding/SMBv3.grammar \
  --format ply \
  --encoding binary-little-endian \
  --texture-mapping bake-atlas \
  --output build/SMBv3/SMBv3-binary.ply
```

Required options:

- `--input <path>` identifies the `.p3d` grammar.
- `--format obj|stl|ply` selects the output format.
- `--encoding ascii|binary-little-endian` selects a valid format encoding.
- `--output <path>` identifies the primary mesh document.

Important optional controls:

- `--object <spatial-object-id>` exports one object hierarchy instead of the
  whole generated scene.
- `--texture-mapping preserve-uv|bake-atlas` selects portable texture mapping.
- `--texture-policy required|sidecar|none` states whether texture loss is an
  error, is documented in a sidecar package, or is intentionally accepted.
- `--texture-resolution <pixels>` controls generated or baked texture maps.
- `--units meters|millimeters` applies an explicit scale conversion.
- `--coordinate-system progen3d-y-up|right-handed-y-up|right-handed-z-up`
  applies an explicit basis conversion.
- `--time <seconds>` evaluates temporal grammar content at an explicit time.
- `--seed <integer>` freezes seeded procedural behavior.
- `--manifest <path>` overrides the default JSON manifest path.
- Repeatable `--texture-slot usertextureN=<image-path>` assignments freeze
  texture-library CPU image inputs for standalone conversion. Referencing an
  unassigned `usertextureN` material fails instead of generating a fallback.

The command must reject an unsupported combination before writing any output.
It must write through temporary files, validate the package, and atomically
rename the completed package into place.

### 3.2 `p3d_to_3view`

Example invocation:

```bash
p3d_to_3view \
  --input examples/SimpleModernBuilding/SMBv3.grammar \
  --output-directory build/SMBv3/three-view \
  --width 1024 \
  --height 1024 \
  --padding 0.04 \
  --render-mode shaded \
  --background transparent
```

Required output names are:

- `front.png`
- `side.png`
- `top.png`
- `three-view-manifest.json`

Important optional controls:

- `--object <spatial-object-id>` captures one direct object or object hierarchy.
- `--scope direct|hierarchy|scene` selects the geometry included in extents.
- `--render-mode shaded|albedo|normal|silhouette` selects deterministic output.
- `--background transparent|white|#RRGGBB` selects a fixed background.
- `--width <pixels>` and `--height <pixels>` define each view independently of
  the interactive editor viewport.
- `--padding <ratio>` reserves a fixed image-space border around the extents.
- `--time <seconds>` and `--seed <integer>` freeze temporal and procedural state.
- `--alpha-cutoff <0..1>` defines deterministic cutout rendering for vegetation.
- Repeatable `--texture-slot usertextureN=<image-path>` assignments provide the
  same frozen texture-library CPU assets used by mesh export.

## 4. Format Capability Matrix

| Format | ASCII | Binary | Normals | UVs | Native material/texture binding | Required strategy |
|---|---:|---:|---:|---:|---:|---|
| OBJ | Yes | No standard binary OBJ | Yes | Yes | Yes, through MTL and image files | Write OBJ + MTL + PNG maps. Reject `--encoding binary-little-endian`. |
| STL | Yes | Yes, little-endian de facto | Facet normal only | No | No | Write geometry only. Textures may be retained only in the sidecar package and manifest. Reject `--texture-policy required`. |
| PLY | Yes | Yes, little-endian | Yes | Custom but common | Non-standard | Write UV and `material_index` properties plus documented texture comments and a sidecar manifest. |

The tools must not describe STL as textured and must not invent a binary OBJ
encoding. A request for impossible native texture preservation fails closed.

Binary big-endian PLY is a possible later extension. Version 1 should reuse and
harden the repository's existing binary little-endian direction rather than add
an untested second binary byte order.

## 5. Canonical Object Model

### 5.1 Compilation models

`P3dSceneCompilationRequest`

- input grammar path
- explicit time and seed
- expansion limits
- optional spatial object selection

`CompiledP3dScene`

- frozen `SceneGenerationContext`
- frozen spatial building model
- grammar source hash
- generation settings
- deterministic scene bounds
- diagnostics

`P3dSceneCompilationService`

- parses and validates the grammar
- evaluates it exactly once
- finalizes the spatial model
- rejects unresolved diagnostics
- returns an immutable compiled-scene snapshot

### 5.2 Textured mesh models

`TexturedMeshVertex`

- world position
- world normal
- texture coordinate

`TexturedMeshTriangle`

- three vertex indices
- material slot
- source primitive index
- optional spatial object identifier

`MaterialTextureImage`

- semantic channel: base color, alpha, normal, roughness, metallic, ambient
  occlusion, emissive, or height
- width and height
- pixel format
- color space
- CPU pixel bytes
- content hash

`SceneMaterialDefinition`

- canonical material name
- scalar PBR values
- opacity and alpha mode
- mapping mode and mapping scale
- projection axes
- owned `MaterialTextureImage` collection

`SceneTexturedMesh`

- ordered vertices
- ordered triangles
- ordered material definitions
- coordinate system and unit metadata
- world bounds
- source-scene provenance

`TexturedMeshExportPackage`

- primary mesh document
- material document when required
- texture image files
- JSON manifest
- validation evidence

### 5.3 Mesh export services

`SceneTexturedMeshAssemblyService`

- reads each retained `ScenePrimitiveInstance`
- delegates geometry generation to `ScenePrimitiveVertexAssemblyService`
- reads every 8-component `SceneRenderVertex`
- preserves position, normal, texture coordinate, material slot, primitive
  provenance, and object association
- preserves triangle order and winding

`SceneMaterialAssetService`

- creates canonical CPU `SceneMaterialDefinition` objects
- resolves procedural and texture-library materials
- owns color-space and alpha interpretation
- provides the same CPU material definitions to preview upload and export

`TextureAtlasBakingService`

- unwraps or uses the existing face-aligned UVs
- evaluates UV or triplanar material mapping into portable 2D atlases
- bakes all enabled PBR channels with matching texel footprints
- writes deterministic padding around atlas islands

`MeshExportCapabilityValidationService`

- validates format, encoding, texture mapping, and texture policy
- reports exactly which properties would be lost
- fails before files are opened when required information cannot be represented

`MeshDocumentWriter`

- abstract category for a service that writes one supported mesh document
- receives `SceneTexturedMesh` and a validated request
- returns document hashes and diagnostics

Concrete writers are real specializations of `MeshDocumentWriter`:

- `ObjMeshDocumentWriter`
- `StlMeshDocumentWriter`
- `PlyMeshDocumentWriter`

`PngTextureImageWriter`

- writes CPU texture images using `stb_image_write.h`
- handles RGB/RGBA channel count explicitly
- never reads texture pixels back from OpenGL

`TexturedMeshManifestWriter`

- records source hash, output hashes, units, coordinate basis, bounds, material
  slots, texture paths, color spaces, alpha policy, mapping policy, and known
  format losses

`TexturedMeshExportWorkflow`

- composes compilation, assembly, capability validation, texture baking,
  document writing, package validation, and atomic publication

### 5.4 Orthographic image models

`OrthographicThreeViewRequest`

- output dimensions
- padding ratio
- render mode and background
- scene, object, or object-hierarchy scope
- explicit time and seed

`OrthographicViewDefinition`

- view name
- world right axis
- world up axis
- world forward axis
- projected center
- orthographic vertical span
- near and far clip distances

`ProjectedSceneExtents`

- minimum and maximum right-axis coordinates
- minimum and maximum up-axis coordinates
- minimum and maximum depth coordinates
- validity and degeneracy evidence

`ThreeViewImageSet`

- front RGBA image
- side RGBA image
- top RGBA image
- shared source-scene hash

`ThreeViewCaptureEvidence`

- view definitions
- source bounds
- projected bounds
- image dimensions
- pixel hashes
- output hashes
- renderer settings

### 5.5 Orthographic image services

`OrthographicViewDefinitionFactory`

- creates front, side, and top view axis definitions
- delegates projected-bound calculation
- calculates aspect-correct zoom-to-extents spans

`ProjectedSceneExtentsService`

- projects all retained world-space geometry or all eight corners of exact
  retained-object bounds into a view basis
- does not use collision bounds when render geometry bounds are available
- rejects an empty or non-finite scene

`OffscreenThreeViewRenderingService`

- owns a fixed-size offscreen RGBA framebuffer and depth buffer
- renders each `OrthographicViewDefinition` through the normal preview renderer
- freezes simulation, material animation, exposure, and random jitter
- reads rows into top-left image order

`PngImageWriter`

- writes front, side, and top images with `stb_image_write.h`
- emits an error when dimensions, channels, or row stride are invalid

`ThreeViewManifestWriter`

- writes axis, bounds, span, clip planes, pixel hash, output hash, source hash,
  material mode, lighting mode, alpha cutoff, and background

`OrthographicThreeViewWorkflow`

- composes compilation, optional object isolation, extents, view construction,
  rendering, PNG writing, validation, and atomic publication

## 6. UML-Readable Relationships

```text
P3dSceneCompilationService creates CompiledP3dScene

TexturedMeshExportWorkflow composes
  P3dSceneCompilationService
  SceneTexturedMeshAssemblyService
  SceneMaterialAssetService
  TextureAtlasBakingService
  MeshExportCapabilityValidationService
  MeshDocumentWriter
  PngTextureImageWriter
  TexturedMeshManifestWriter

SceneTexturedMesh aggregates
  TexturedMeshVertex
  TexturedMeshTriangle
  SceneMaterialDefinition

SceneMaterialDefinition composes MaterialTextureImage

OrthographicThreeViewWorkflow composes
  P3dSceneCompilationService
  PreviewObjectCameraViewService
  ProjectedSceneExtentsService
  OrthographicViewDefinitionFactory
  OffscreenThreeViewRenderingService
  PngImageWriter
  ThreeViewManifestWriter

ThreeViewImageSet composes three PreviewRgbaImage values
```

The two workflows associate through the immutable `CompiledP3dScene`; neither
workflow owns a second grammar evaluator.

## 7. Textured Mesh Assembly Algorithm

1. Compile the grammar with explicit time, seed, and expansion limits.
2. Finalize spatial object associations.
3. Select the whole scene or the requested object hierarchy.
4. For each retained primitive in source order, call
   `ScenePrimitiveVertexAssemblyService::appendInstanceVertices`.
5. Read each vertex as position `[0..2]`, normal `[3..5]`, and UV `[6..7]`.
6. Add one `TexturedMeshTriangle` for each ordered group of three vertices.
7. Copy `ScenePrimitiveInstance::material_index` and source primitive index.
8. Resolve the optional spatial object identifier through the existing primitive
   association index.
9. Validate finite positions, finite normalized normals, finite UVs, valid
   material slots, complete triangles, and consistent winding.
10. Apply the requested unit and coordinate conversion in one explicit basis
    transformation. Transform normals with the inverse-transpose. Reverse
    winding when the basis determinant is negative.
11. Keep vertices split across UV seams, hard-normal seams, material boundaries,
    and object boundaries. Optional deduplication may combine only vertices with
    identical complete semantic tuples.
12. Compute exported bounds from final exported vertices, not from stale authored
    metadata.

The legacy `SceneExportMeshAssemblyService` remains available for compatibility.
It should not be expanded into a multi-responsibility textured exporter.

## 8. Canonical CPU Material Pipeline

### 8.1 Required refactor

Material generation should return a CPU `SceneMaterialDefinition` first.
OpenGL preview upload becomes a consumer of that model rather than its owner.
This prevents mesh export from depending on an active OpenGL context and avoids
non-portable `glGetTexImage` readback.

The procedural material path already creates CPU pixel arrays before upload.
Those arrays should be retained in `MaterialTextureImage` objects. Texture
library images should be decoded once into the same CPU model. GPU handles in
`PreviewSurfaceMaterial` remain runtime resources and must never appear in an
export manifest.

### 8.2 Channel policy

| Channel | PNG channels | Color interpretation |
|---|---:|---|
| Base color | RGB or RGBA | sRGB |
| Alpha | Gray or RGBA alpha | Linear coverage |
| Normal | RGB | Linear tangent-space vector |
| Roughness | Gray | Linear scalar |
| Metallic | Gray | Linear scalar |
| Ambient occlusion | Gray | Linear scalar |
| Emissive | RGB | sRGB color with linear strength metadata |
| Height | Gray | Linear scalar with scale metadata |

`stb_image_write` does not establish a complete ICC workflow. Version 1 must
record color-space semantics in the manifest and MTL comments. A later libpng
writer may add explicit PNG color-profile chunks without changing the material
object model.

### 8.3 Mapping policy

`preserve-uv` is valid only when the material appearance is defined by exported
UVs. If the preview uses triplanar mapping, `preserve-uv` with
`--texture-policy required` must fail.

`bake-atlas` is the portable default for exact appearance. It evaluates the
current UV or triplanar mapping over exported triangles and bakes all material
channels into UV atlases. Atlas generation must use a deterministic triangle
order, deterministic island packing, fixed padding, and an explicit resolution.

Normal maps must be baked into the tangent basis implied by the exported UVs.
The manifest records MikkTSpace as the required tangent reconstruction rule for
consumers that need tangent-space normal mapping.

## 9. Writer Requirements

### 9.1 OBJ and MTL

The OBJ writer must:

- write `v`, `vt`, and `vn` records
- write triangle `f` records with aligned position/UV/normal indices
- write deterministic `o` or `g` names from spatial object identifiers
- write `usemtl` only when the material slot changes
- reference one deterministic `.mtl` file
- use relative, normalized texture paths

The MTL writer must use portable baseline fields and documented extensions:

- `Kd` and `map_Kd` for base color
- `d` and `map_d` for opacity
- `Ke` and `map_Ke` for emissive color
- `Ns` as a compatibility approximation derived from roughness
- `map_Bump` for tangent-space normal maps
- `map_Pr` and `map_Pm` as documented roughness/metallic extensions

The manifest remains authoritative where importers disagree about MTL PBR
extensions.

### 9.2 STL

The STL writer must support:

- deterministic ASCII STL
- binary little-endian STL with an 80-byte deterministic header
- geometric facet normals derived from final triangle positions
- finite-coordinate and non-degenerate-triangle validation

STL cannot carry UVs, material slots, object hierarchy, or textures. The writer
must not use vendor-specific packed facet colors in version 1. When
`--texture-policy sidecar` is selected, texture images and the manifest may be
written for provenance, but they are not bound to the STL document.

### 9.3 PLY

The PLY writer must support the same schema in ASCII and binary little-endian:

```text
element vertex <count>
property float x
property float y
property float z
property float nx
property float ny
property float nz
property float s
property float t
element face <count>
property list uchar uint vertex_indices
property int material_index
```

The header may include deterministic `comment TextureFile <relative-path>` and
material comments. Because these conventions are not universal, the JSON
manifest remains authoritative. Binary serialization must write explicit
little-endian bytes rather than dumping host memory.

## 10. Orthographic View Definitions

The view convention intentionally matches `ThreeViewProjectionService`:

| View | Camera position side | Forward axis | Image right | Image up | Projection |
|---|---|---|---|---|---|
| Front | positive Z | negative Z | positive X | positive Y | `(x, y)` |
| Side | positive X | negative X | negative Z | positive Y | `(-z, y)` |
| Top | positive Y | negative Y | negative Z | positive X | `(-z, x)` |

These definitions produce true flattened elevations. No perspective matrix,
field-of-view approximation, dolly zoom, or perspective post-processing is
permitted.

## 11. Zoom-to-Extents Mathematics

For each view, let `right`, `up`, and `forward` be an orthonormal basis. Project
each retained world-space vertex `position`:

```text
u = dot(position, right)
v = dot(position, up)
d = dot(position, forward)
```

Accumulate `minimum_u`, `maximum_u`, `minimum_v`, `maximum_v`, `minimum_d`, and
`maximum_d`.

For output width `W`, height `H`, aspect `A = W / H`, padding ratio `P`, and a
small nonzero minimum span:

```text
content_width  = max(maximum_u - minimum_u, minimum_span)
content_height = max(maximum_v - minimum_v, minimum_span)
padded_width   = content_width  * (1 + 2P)
padded_height  = content_height * (1 + 2P)
vertical_span  = max(padded_height, padded_width / A)
horizontal_span = vertical_span * A
```

The projected center is:

```text
center_u = (minimum_u + maximum_u) / 2
center_v = (minimum_v + maximum_v) / 2
center_d = (minimum_d + maximum_d) / 2
target = right * center_u + up * center_v + forward * center_d
```

The camera is placed opposite the forward axis far enough to enclose the depth
span. Orthographic image scale is controlled only by `vertical_span`; camera
distance exists solely for valid near/far clipping.

```text
depth_span = max(maximum_d - minimum_d, minimum_span)
clip_margin = max(depth_span * 0.25, 1.0)
camera_position = target - forward * (depth_span / 2 + clip_margin)
near_plane = 0.01
far_plane = depth_span + 2 * clip_margin
```

Each image must be validated by reprojecting all retained vertices and proving
that every projected point falls inside the padded orthographic rectangle.

## 12. Deterministic Rendering Contract

`p3d_to_3view` must not inherit mutable editor state. It creates a dedicated
offscreen render session with:

- fixed viewport dimensions
- RGBA8 or SRGB8_ALPHA8 color attachment
- depth attachment
- orthographic projection only
- fixed camera axes from this document
- fixed studio lighting preset
- fixed exposure and tone mapping
- fixed background
- disabled camera easing and user input
- disabled temporal antialiasing jitter
- frozen physics and grammar time
- frozen material UV animation and pulse phase
- fixed alpha cutoff
- deterministic vegetation LOD selected from projected coverage, or a forced
  export LOD for golden evidence

The renderer should support these stable modes:

- `shaded`: deterministic PBR preview appearance
- `albedo`: unlit base color and alpha
- `normal`: encoded world normal
- `silhouette`: binary foreground coverage compatible with the existing CPU
  three-view service

Transparent capture requires RGBA framebuffer readback. OpenGL rows must be
flipped once into top-left image origin before hashing and PNG writing.

## 13. Output Manifests

### 13.1 Mesh manifest minimum fields

```json
{
  "schema": "progen3d.textured-mesh-export.v1",
  "source": {"path": "...", "sha256": "...", "time": 0.0, "seed": 1},
  "format": "obj",
  "encoding": "ascii",
  "units": "meters",
  "coordinate_system": "right-handed-y-up",
  "bounds": {"minimum": [0, 0, 0], "maximum": [1, 1, 1]},
  "geometry": {"vertices": 0, "triangles": 0, "materials": 0},
  "texture_mapping": "bake-atlas",
  "files": [{"path": "...", "sha256": "..."}],
  "losses": []
}
```

### 13.2 Three-view manifest minimum fields

```json
{
  "schema": "progen3d.orthographic-three-view.v1",
  "source": {"path": "...", "sha256": "...", "time": 0.0, "seed": 1},
  "projection": "orthographic",
  "padding_ratio": 0.04,
  "views": [
    {
      "name": "front",
      "right": [1, 0, 0],
      "up": [0, 1, 0],
      "forward": [0, 0, -1],
      "vertical_span": 1.0,
      "width": 1024,
      "height": 1024,
      "pixel_hash": "...",
      "file_sha256": "..."
    }
  ]
}
```

## 14. Proposed Source Layout

```text
include/conversion/model/P3dSceneCompilationRequest.h
include/conversion/model/CompiledP3dScene.h
include/conversion/model/TexturedMeshVertex.h
include/conversion/model/TexturedMeshTriangle.h
include/conversion/model/MaterialTextureImage.h
include/conversion/model/SceneMaterialDefinition.h
include/conversion/model/SceneTexturedMesh.h
include/conversion/model/TexturedMeshExportRequest.h
include/conversion/model/TexturedMeshExportPackage.h
include/conversion/model/OrthographicThreeViewRequest.h
include/conversion/model/OrthographicViewDefinition.h
include/conversion/model/ProjectedSceneExtents.h
include/conversion/model/ThreeViewImageSet.h
include/conversion/model/ThreeViewCaptureEvidence.h

include/conversion/service/P3dSceneCompilationService.h
include/conversion/service/SceneTexturedMeshAssemblyService.h
include/conversion/service/SceneMaterialAssetService.h
include/conversion/service/TextureAtlasBakingService.h
include/conversion/service/MeshExportCapabilityValidationService.h
include/conversion/service/MeshDocumentWriter.h
include/conversion/service/ObjMeshDocumentWriter.h
include/conversion/service/StlMeshDocumentWriter.h
include/conversion/service/PlyMeshDocumentWriter.h
include/conversion/service/PngTextureImageWriter.h
include/conversion/service/TexturedMeshManifestWriter.h
include/conversion/service/TexturedMeshExportWorkflow.h
include/conversion/service/ProjectedSceneExtentsService.h
include/conversion/service/OrthographicViewDefinitionFactory.h
include/conversion/service/OffscreenThreeViewRenderingService.h
include/conversion/service/PngImageWriter.h
include/conversion/service/ThreeViewManifestWriter.h
include/conversion/service/OrthographicThreeViewWorkflow.h

src/conversion/model/*.cpp
src/conversion/service/*.cpp
src/conversion/application/p3d_to_3dtexmesh_main.cpp
src/conversion/application/p3d_to_3view_main.cpp
src/image/third_party/StbImageWriteImplementation.cpp
```

`MeshDocumentWriter` inheritance is justified because each concrete writer is a
kind of mesh-document writer. Format capability, texture baking, compilation,
and publication remain composed services rather than hidden writer behavior.

## 15. Implementation Phases

### Phase 1: Freeze conversion inputs

- Introduce `P3dSceneCompilationRequest` and `CompiledP3dScene`.
- Reuse the editor grammar runtime without editor UI ownership.
- Require explicit seed, time, and expansion limits in evidence.
- Add scene and object-hierarchy selection tests.

### Phase 2: Establish canonical CPU material assets

- Introduce `MaterialTextureImage` and `SceneMaterialDefinition`.
- Refactor procedural and texture-library material creation to produce CPU assets
  before OpenGL upload.
- Make preview upload and export consume the same material definition.
- Prove byte-identical CPU material hashes across repeated runs.

### Phase 3: Preserve textured geometry

- Add `SceneTexturedMeshAssemblyService` beside the legacy position-only service.
- Preserve render vertices, material slots, primitive provenance, and spatial
  object association.
- Add coordinate conversion, unit conversion, winding, UV seam, and normal tests.

### Phase 4: Implement format writers

- Implement OBJ/MTL/PNG package writing.
- Implement ASCII and binary little-endian STL.
- Replace host-memory PLY binary writes with explicit little-endian serialization.
- Extend PLY ASCII and binary schemas with normals, UVs, and material index.
- Add capability validation and atomic package publication.

### Phase 5: Implement portable texture baking

- Support exact UV preservation where valid.
- Add deterministic atlas packing and triplanar-to-UV baking.
- Bake base color, alpha, normal, roughness, metallic, AO, emissive, and height.
- Add atlas bleed and tangent-space validation.

### Phase 6: Implement offscreen orthographic PNG capture

- Add exact projected-extents calculation.
- Add front, side, and top view definitions.
- Add dedicated RGBA offscreen framebuffer rendering.
- Add native PNG writing and manifests.
- Reuse object isolation for direct and hierarchy captures.

### Phase 7: Add command-line applications

- Add both executables to the repository build.
- Return nonzero for grammar, capability, render, validation, or publication
  failures.
- Print concise output paths and evidence hashes on success.

### Phase 8: Certification and compatibility

- Keep existing PLY APIs and PPM preview evidence operational.
- Run focused conversion tests, full test discovery, headless rendering smoke,
  and packaging/build gates.
- Publish golden artifacts only from the exact tested source hash.

## 16. Independent Validation Matrix

### 16.1 Geometry and format fixtures

1. **Asymmetric axis sentinel** proves front/side/top orientation.
2. **Single textured triangle** proves position, normal, UV, winding, and one
   material in OBJ and PLY.
3. **UV seam cube** proves vertices are not merged across UV seams.
4. **Hard-normal cube** proves vertices are not merged across normal seams.
5. **Two-material wall and window** proves deterministic material partitions.
6. **Transparent leaf cards** prove alpha map and double-sided policy.
7. **Triplanar concrete block** proves `preserve-uv` fails when required and
   `bake-atlas` succeeds.
8. **Non-uniform transform** proves inverse-transpose normal conversion.
9. **Reflected coordinate basis** proves winding reversal.
10. **Degenerate triangle** proves fail-closed validation.

For every valid fixture:

- export twice and require byte-identical files
- independently parse the generated document
- compare counts, bounds, winding, normals, UVs, material indices, and hashes
- prove binary output byte order using known byte sequences

### 16.2 Texture fixtures

1. RGB checkerboard base color
2. RGBA cutout foliage
3. tangent-space normal direction sentinel
4. roughness gradient
5. metallic split
6. ambient-occlusion corner mask
7. emissive stripe
8. height ramp with scale metadata

OBJ must preserve all portable bindings in MTL and the manifest. PLY must
preserve UVs/material indices and sidecar bindings. STL tests must prove that
native texture requirements are rejected and sidecar loss is explicit.

### 16.3 Orthographic three-view fixtures

For each fixture and each front/side/top view:

- prove the projection kind is orthographic
- prove the camera height/target aligns with the bounds center for horizontal
  views
- prove every retained vertex projects inside the padded rectangle
- prove the occupied pixel bounds approach the requested padding without
  clipping
- prove front `(x,y)`, side `(-z,y)`, and top `(-z,x)` orientation
- prove repeated PNG bytes and pixel hashes are identical
- prove rectangular outputs preserve aspect-correct framing
- prove scene, direct-object, and hierarchy scopes exclude unrelated geometry

The CPU `ThreeViewProjectionService` silhouette should be used as an independent
coverage oracle for `--render-mode silhouette`. GPU and CPU silhouettes should
meet an explicit intersection-over-union threshold after matching resolution
and raster edge policy.

### 16.4 Round-trip and visual gates

- Load OBJ through an independent test parser and compare semantic mesh data.
- Load STL ASCII and binary and compare triangle geometry.
- Load PLY ASCII and binary and compare the declared schema and values.
- Re-render imported OBJ and PLY fixtures from the same orthographic views.
- Compare albedo images with per-pixel error and silhouette IoU metrics.
- Require no clipped geometry and no missing texture paths.
- Record accepted thresholds per render mode; do not claim exact PBR identity
  across third-party renderers whose lighting models differ.

## 17. Completion Gates

The conversion slice is complete only when all of the following are true:

- OBJ ASCII writes geometry, normals, UVs, MTL, and PNG textures.
- Binary OBJ requests fail with an exact capability diagnostic.
- STL ASCII and binary little-endian write equivalent geometry.
- STL texture loss is explicit and required native texture requests fail.
- PLY ASCII and binary little-endian use equivalent textured-mesh schemas.
- PLY binary byte order is host-independent.
- UV and triplanar materials have a portable, validated mapping path.
- CPU material assets are the shared authority for preview and export.
- Front, side, and top PNGs use orthographic projection only.
- Three-view framing is centered and zoomed to the selected geometry extents.
- Object and object-hierarchy capture exclude unrelated geometry.
- Every output package contains source and artifact hashes.
- Repeated conversion of deterministic fixtures is byte-identical.
- Focused tests, broad repository tests, headless rendering smoke, and build
  packaging pass from the same source revision.

Passing a position-only mesh export test or a CPU silhouette test alone does not
establish completion of this strategy.

## 18. Recommended First Implementation Slice

The safest first slice is:

1. Add canonical `SceneTexturedMesh` assembly from the existing
   `SceneRenderVertex` stream.
2. Add CPU `SceneMaterialDefinition` output for one procedural material and one
   texture-library material.
3. Implement OBJ/MTL/PNG with `preserve-uv` only.
4. Implement exact projected extents and PNG `silhouette` mode through a fixed
   offscreen framebuffer.
5. Validate the asymmetric axis sentinel, textured triangle, UV seam cube, and
   transparent leaf card.

This slice proves the shared data authority and orthographic camera contract
before binary formats, atlas baking, and full shaded capture add complexity.
