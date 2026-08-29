# ProGen3D Canonical Material Catalog Proposal

## Document Control

- **Project:** ProGen3D
- **Proposal date:** August 27, 2026
- **Scope:** material naming, material-language ownership, procedural material interpretation, preview rendering, autocomplete, and repository material migration
- **Status:** canonical foundation and advanced preview-material slice implemented; focused deterministic gates pass; language-version ownership, migration, visual approval, and release certification remain pending
- **Compatibility position:** legacy scene-slot spelling and procedural seed identity are preserved; the document-owned `MaterialLanguageVersion` boundary is not yet implemented, so canonical-v2 default promotion remains blocked

## Implementation Update — August 27, 2026

Implemented in the current workspace:

- one typed `MaterialLexiconCatalog` and `MaterialNameCanonicalizationService` shared by material interpretation and `SceneMaterialCatalog`;
- explicit render families, semantic lexeme categories, canonical aliases, unknown-fragment ranges, deterministic warnings, and effect-only emissive inference;
- canonical scene identity that no longer appends `plaster` to recognized rubber, ceramic, polymer, vegetation, emissive, ground, or composite materials;
- typed procedural branches for metal, glass, vegetation, brick, floral textiles, boards, wood, stone, concrete, rubber, general textiles, ground materials, composites, ceramic, polymer, and coating surfaces;
- deterministic CPU-derived normal, roughness, metallic, and ambient-occlusion maps based on the generated height field and interpreted PBR scalars;
- scalar-correct neutral fallback maps for backend textures instead of unrelated random normals, near-white roughness, all-metallic defaults, and synthetic AO;
- an analytic environment BRDF approximation and energy-consistent environment diffuse/specular split in the GLSL 4.60 preview shader;
- deterministic gates for the 100-name canonical catalog, the 61-name executable `.p3d`/`.grammar` corpus, CPU material-map generation, scene material identity, direct GLSL validation, and preview-session/resource architecture.

Explicitly unresolved:

- `bluepipe`, `redpipe`, and `material` remain diagnostic failures because their material meaning cannot be inferred safely;
- existing authored names are not rewritten automatically; canonical recommendations are advisory until migration is reviewed;
- `MaterialLanguageVersion`, frozen legacy visual fixtures, representative image captures, true irradiance/prefiltered-environment resources, a BRDF lookup texture, and scene-color refraction remain future gated work.

## 1. Executive Proposal

ProGen3D should introduce a **Canonical Material Language v2** built around one deterministic material-name owner, explicit lexeme categories, a broader substance catalog, and diagnostics for every unrecognized fragment.

The proposed language retains the current compact concatenated form:

```text
material(warmwhitematteplaster)
material(redmetallicclearcoatpaint)
material(charcoalroughrubber)
material(offwhitewovenmattefabric)
material(softbeigemattesandstone)
```

The proposal does not replace the current procedural material pipeline. It adds a canonical semantic layer in front of it so that:

- every accepted material name has a complete interpretation;
- aliases normalize to one canonical name;
- material family and subtype are not inferred from accidental substrings;
- unsupported words are reported instead of silently ignored;
- broad material subtypes reuse a smaller set of explicit render families;
- existing source and deterministic procedural seeds can be preserved during migration; and
- the Material Library can expose a curated catalog rather than an unstructured token list.

## 2. Investigation Scope

The investigation covered:

- `include/editor/model/MaterialLanguage.h`;
- `include/editor/service/MaterialLanguageService.h`;
- `src/editor/service/MaterialLanguageService.cpp`;
- `include/editor/service/Progen3dPreviewMaterialResourceFactory.h`;
- `src/editor/service/Progen3dPreviewMaterialResourceFactory.cpp`;
- `include/scene/model/SceneMaterialCatalog.h`;
- `src/scene/model/SceneMaterialCatalog.cpp`;
- the material-language and scene-material harnesses;
- current material grammar examples;
- vehicle and building lowering services; and
- static `material(...)` references in repository source, examples, tests, plans, and documentation, excluding build and third-party trees.

The static repository census found **79 distinct literal material names across 1,339 references**.

## 3. Current Material System

### 3.1 Current Ownership

The current responsibilities are divided as follows:

- `SceneMaterialCatalog` converts authored text into a scene slot name.
- `MaterialLanguageService` scans material lexemes and creates a `ProceduralMaterialSpecification`.
- `Progen3dPreviewMaterialResourceFactory` turns the specification into generated texture maps and PBR preview properties.
- `MaterialLibraryPanel` and autocomplete present material choices to the author.

This separation is directionally sound, but canonical-name ownership is duplicated. `SceneMaterialCatalog` and `MaterialLanguageService` do not use the same family catalog or the same definition of a valid material name.

### 3.2 Current Lexicon

The material language currently contains **77 lexemes**:

- **23 colors:** `amber`, `beige`, `black`, `blue`, `brown`, `chrome`, `copper`, `cyan`, `gold`, `gray`, `green`, `grey`, `lime`, `magenta`, `orange`, `pink`, `purple`, `red`, `silver`, `teal`, `violet`, `white`, `yellow`;
- **17 families:** `boards`, `brick`, `ceramic`, `concrete`, `floral`, `glass`, `grass`, `marble`, `metal`, `neon`, `paint`, `plaster`, `plastic`, `rock`, `stone`, `tile`, `wood`;
- **4 opacity terms:** `opaque`, `smoky`, `translucent`, `transparent`; and
- **33 finish terms:** animation, optical, pattern, surface, and wood-species terms currently share one category.

The current `Finish` category mixes unrelated concepts. For example, `oak` is a species, `vertical` is a pattern direction, `emissive` is an optical effect, and `matte` is a surface finish.

### 3.3 Current Corpus Coverage

Only **40 of the 79 distinct names** are fully composed from current lexemes. The remaining **39 names contain ignored fragments**. Those partial names account for **842 of 1,339 references**, or approximately **62.9 percent** of the scanned material references.

High-impact ignored fragments include:

| Ignored fragment | Affected references | Representative authored name | Current consequence |
|---|---:|---|---|
| `charcoal` | 233 | `charcoalroughmetal` | falls back to the default palette |
| `lic` | 181 | `redmetallicpaint` | `metallic` is split into `metal` plus ignored `lic` |
| `warm` | 94 | `amberwarmwood` | temperature intent is discarded |
| `rubber` | 83 | `charcoalroughrubber` | no rubber family is selected |
| `gloss` | 80 | `glossblackpaint` | glossy intent is discarded |
| `sage` | 67 | `sagegreenmattepaint` | the sage contribution is discarded |
| `light` | 58 | `lightgreyroughconcrete` | tone intent is discarded |
| `off` | 39 | `offwhiteceramic` | off-white intent is discarded |
| `soft` | 28 | `softbeigemattesandstone` | tone intent is discarded |
| `darkslate` | 16 | `darkslatesmoothstone` | slate and tone intent are discarded |
| `pastel` | 15 | `pastelbluemattepaint` | tone intent is discarded |
| `fabric` | 8 plus compound cases | `offwhitefabric` | no textile family is selected |

Other unsupported material words include `steel`, `sandstone`, `asphalt`, `terracotta`, `mint`, `bark`, and `pipe`.

### 3.4 Silent-Skip Behavior

`MaterialLanguageService` scans for the longest lexeme at each character position. When no lexeme matches, it advances by one character without recording a diagnostic.

This creates plausible-looking names that are only partially interpreted. Examples include:

- `redmetallicpaint` being recognized as `red + metal + paint`, with `lic` ignored;
- `softbeigemattesandstone` being recognized as `beige + matte + stone`, with `soft` and `sand` ignored;
- `creamtiledceramic` being recognized as `tile + ceramic`, with `cream` and the final `d` ignored; and
- `charcoalroughrubber` being recognized only as `rough`.

Canonical Material Language v2 must not silently discard authored text.

### 3.5 Competing Canonicalization Rules

`SceneMaterialCatalog` lowercases authored text, removes non-alphanumeric characters, and appends `plaster` when its local texture-family list is not found.

That local list includes `fabric`, `carpet`, and `granite`, which are not current material-language families. It omits current families including `ceramic`, `plastic`, `grass`, and `neon`.

The repository census found **17 distinct names across 165 references** that receive an added `plaster` suffix under this rule. Examples include:

- `charcoalroughrubber` becoming `charcoalroughrubberplaster`;
- `offwhiteceramic` becoming `offwhiteceramicplaster`;
- `cyanneonglowing` becoming `cyanneonglowingplaster`;
- `softyellowglossyplastic` becoming `softyellowglossyplasticplaster`; and
- `darkasphalt` becoming `darkasphaltplaster`.

The material language and scene catalog therefore need one canonical-name authority.

### 3.6 Render-Family Coverage

The procedural preview factory has explicit generation branches for:

- metal;
- glass;
- grass;
- brick;
- floral;
- boards;
- neon;
- wood;
- marble;
- stone; and
- concrete.

Other interpreted families use the generic blend branch. The PBR property mapping additionally refers to `cloth`, even though `cloth` is not a current family lexeme. This is evidence that textile support was anticipated but never connected end to end.

### 3.7 Compatibility Risks

Material rendering is deterministic from the authored name, including its seed. Renaming or reordering lexemes can therefore alter both semantic properties and procedural texture details.

The following changes must be treated as visual compatibility changes, not spelling-only cleanup:

- recognizing a previously ignored color or tone;
- changing `redmetallicpaint` from the metal branch to the paint branch;
- separating plaster from paint;
- mapping fabric names to a textile branch;
- removing an automatically appended `plaster`; and
- seeding from a new canonical spelling instead of the legacy authored spelling.

## 4. Canonical Naming Contract

### 4.1 Canonical Form

A canonical material name is a lowercase concatenation of recognized semantic lexemes in this order:

```text
[tone][color1][color2][color3][opacity][pattern...][finish...][effect...][substance]
```

Examples:

```text
warmwhitematteplaster
sagegreenmattepaint
redmetallicclearcoatpaint
bluetransparentfrostedglass
whiteveinedpolishedmarble
offwhitewovenmattefabric
blackwovenmattecarbonfiber
```

### 4.2 Canonical Rules

1. A canonical name contains exactly one terminal substance.
2. A specific substance such as `stainlesssteel` implies its render family and does not require a redundant `metal` suffix.
3. Up to three color lexemes may be combined in authored order.
4. Tone lexemes modify the assembled palette rather than acting as colors.
5. Pattern, finish, and effect lexemes retain distinct meanings.
6. Separators and capitalization are accepted as authoring conveniences but are removed from the canonical result.
7. Aliases are accepted but rewritten to canonical lexemes.
8. Unknown fragments produce diagnostics. Canonical-v2 publication fails when unresolved fragments remain.
9. Conflicting lexemes produce diagnostics. Examples include `opaque + transparent`, `matte + mirror`, and `rough + polished` unless a named compatibility definition explicitly permits them.
10. Geometry-role words such as `pipe`, `floor`, and `sport` are not material lexemes.
11. `usertextureN` remains a reserved canonical namespace and bypasses procedural material interpretation.
12. The canonical name, language version, and deterministic seed basis are recorded in the interpreted specification.

### 4.3 Canonical Aliases

Initial aliases should include:

| Accepted alias | Canonical lexeme or sequence |
|---|---|
| `gray` | `grey` |
| `gloss` | `glossy` |
| `shiny` | `glossy` |
| `dull` | `matte` |
| `duotone` | `twotone` |
| `veins` | `veined` |
| `rock` | `stone` |
| `cloth` | `fabric` |
| `aluminum` | `aluminium` |
| `smoked` | `smoky` |
| `clear` | `transparent` |
| `tiled` | `grid` when used as a pattern, or an explicit tile substance during guided migration |

Aliases must be explicit records. Substring coincidence must never act as an alias.

## 5. Proposed Lexeme Categories

Canonical Material Language v2 should replace the four-way `MaterialLexemeKind` model with purpose-specific categories.

### 5.1 Tone Qualifiers

```text
bright cool dark deep light muted natural pastel soft warm
```

### 5.2 Colors

Retain the current colors, canonicalize `gray` to `grey`, and add the colors already expressed by repository material names and common design domains:

```text
charcoal cream ivory leafgreen maroon mint navy offwhite olive rust sage sand tan terracotta turquoise burgundy
```

### 5.3 Opacity Qualifiers

```text
opaque smoky translucent transparent
```

### 5.4 Pattern Qualifiers

```text
checkered coarsegrain finegrain floral grid horizontal mottled perforated ribbed solid speckled striped twotone veined vertical woven
```

### 5.5 Surface Finishes

```text
aged anodized brushed frosted galvanized glossy hammered honed matte mirror painted polished rough satin smooth velvet weathered
```

### 5.6 Optical and Dynamic Effects

```text
active animated clearcoat emissive glowing metallic pearlescent reflective rotating
```

### 5.7 Substance Lexemes

Substance lexemes identify what the material is. Specific substances map to a broader render family.

#### Coatings and Architectural Finishes

```text
paint plaster enamel
```

#### Masonry, Mineral, and Ground Materials

```text
asphalt brick ceramic ceramictile concrete granite gravel limestone marble porcelaintile sandstone slate stone terrazzo tile
```

#### Wood and Plant-Derived Materials

```text
bamboo birchwood boards cedarwood cork oakboards oakwood pinewood walnutboards walnutwood wood
```

#### Metals

```text
aluminium brass bronze iron metal stainlesssteel steel titanium zinc
```

`chrome`, `copper`, `gold`, and `silver` remain color or coating descriptors when followed by a metal substance, preserving names such as `copperpolishedmetal`.

#### Glass and Transparent Solids

```text
crystalglass glass
```

#### Polymers and Composites

```text
acrylic carbonfiber composite fiberglass plastic polycarbonate rubber silicone
```

#### Textiles and Leather

```text
canvasfabric carpet cottonfabric fabric feltfabric leather linenfabric
```

#### Organic, Landscape, and Emissive Materials

```text
bark foliage grass neon soil
```

## 6. Proposed Render Families

The wider substance catalog should not create one renderer branch per commercial material name. Each substance maps to one explicit `MaterialRenderFamily`:

| Render family | Representative substances | Initial procedural strategy |
|---|---|---|
| `Coating` | paint, enamel | smooth blend with finish and clearcoat controls |
| `Plaster` | plaster | mineral noise with low reflectance and shallow height |
| `Concrete` | concrete | current concrete branch with subtype overrides |
| `Brick` | brick | current staggered brick branch |
| `Stone` | stone, marble, granite, limestone, sandstone, slate, terrazzo, gravel | shared mineral base with subtype-specific grain, veins, and height |
| `Ceramic` | ceramic, tile, ceramictile, porcelaintile | smooth fired surface with optional grid or glaze |
| `Wood` | wood, species woods, boards, bamboo, cork, bark | current wood and boards branches with subtype palettes and grain scales |
| `Metal` | metal, steel, stainless steel, aluminium, iron, brass, bronze, titanium, zinc | current metal branch with alloy-specific color, roughness, and anisotropy |
| `Glass` | glass, crystal glass | current glass branch with opacity, IOR, thickness, and frosting controls |
| `Polymer` | plastic, acrylic, polycarbonate, silicone | smooth synthetic branch with subtype optical defaults |
| `Rubber` | rubber | high-roughness polymer branch with fine normal variation |
| `Textile` | fabric, canvas, cotton, felt, linen, carpet | new woven/fiber branch with sheen and directional normals |
| `Leather` | leather | fibrous-grain branch with crease and pore variation |
| `Vegetation` | grass, foliage | current grass branch generalized for leaf and blade structures |
| `Ground` | soil, asphalt, gravel | coarse aggregate branch with high roughness |
| `Emissive` | neon | current neon branch |
| `Composite` | carbon fiber, fiberglass, composite | layered weave or strand branch with resin clearcoat |

Subtypes may override physical defaults, but render-family ownership remains singular and explicit.

## 7. Proposed Object Model

The implementation should read as a purpose-driven object model.

### 7.1 Model Classes

#### `MaterialCanonicalName`

Immutable value object containing:

- normalized canonical text;
- ordered canonical lexemes;
- language version;
- terminal substance identity; and
- deterministic seed basis.

#### `MaterialLexemeDefinition`

Abstract conceptual category for a recognized lexical unit. Concrete subclasses represent real is-a relationships:

- `MaterialToneLexemeDefinition`;
- `MaterialColorLexemeDefinition`;
- `MaterialOpacityLexemeDefinition`;
- `MaterialPatternLexemeDefinition`;
- `MaterialFinishLexemeDefinition`;
- `MaterialEffectLexemeDefinition`; and
- `MaterialSubstanceLexemeDefinition`.

#### `MaterialFamilyDefinition`

Defines one `MaterialRenderFamily`, its physical defaults, supported patterns, supported finishes, and preview strategy.

#### `MaterialSubtypeDefinition`

Associates one canonical substance with exactly one `MaterialFamilyDefinition` and contains subtype-specific palette and PBR overrides.

#### `MaterialAliasDefinition`

Defines one accepted non-canonical token or token sequence and its canonical replacement.

#### `MaterialNameInterpretationReport`

Contains:

- authored name;
- normalized authored name;
- canonical name when available;
- recognized lexemes;
- alias substitutions;
- unrecognized fragments;
- conflicting lexemes;
- compatibility warnings; and
- publishability under the selected language version.

#### `MaterialCompatibilityDefinition`

Freezes the interpretation and seed behavior of one legacy material name when exact visual compatibility is required.

### 7.2 Service Classes

#### `MaterialLexiconCatalog`

Owns all lexeme, alias, family, subtype, ordering, and compatibility definitions. It is the single semantic authority used by scene admission, autocomplete, interpretation, auditing, and migration.

#### `MaterialNameParsingService`

Tokenizes a normalized material name and records every covered and uncovered character range.

#### `MaterialNameCanonicalizationService`

Validates ordering and conflicts, applies explicit aliases, and constructs a `MaterialCanonicalName`.

#### `MaterialSpecificationAssemblyService`

Builds `ProceduralMaterialSpecification` from the canonical model and selected compatibility policy.

#### `MaterialNameMigrationService`

Produces reviewable source replacements. It never edits grammar source without an explicit caller-owned transaction.

#### `MaterialCatalogAuditService`

Scans authoritative repository material references and produces coverage, alias, conflict, render-family, and migration reports.

### 7.3 Existing-Class Changes

- `SceneMaterialCatalog` should store `MaterialCanonicalName` values and delegate canonicalization to `MaterialNameCanonicalizationService`.
- `MaterialLanguageService` should become a facade over parsing, canonicalization, specification assembly, and autocomplete services.
- `ProceduralMaterialSpecification` should replace the ambiguous free-form `family` string with `MaterialRenderFamily` and a `MaterialSubtypeIdentity` while retaining a compatibility string only at existing API boundaries.
- `Progen3dPreviewMaterialResourceFactory` should dispatch by `MaterialRenderFamily`, not string comparisons.
- `MaterialLibraryPanel` should present curated canonical materials, category filters, canonical-name previews, warnings, and explicit legacy migration actions.

## 8. Proposed Initial Canonical Catalog

The following 100 names form a practical first catalog. They are canonical examples, not a closed list; authors can compose other valid names from the lexicon.

### 8.1 Coatings and Plaster

1. `warmwhitematteplaster`
2. `softbeigematteplaster`
3. `coolgreysmoothplaster`
4. `whitepolishedplaster`
5. `sagegreenmattepaint`
6. `pastelbluemattepaint`
7. `blackglossyclearcoatpaint`
8. `redmetallicclearcoatpaint`
9. `whitesatinpearlescentpaint`
10. `deepbluemetallicpaint`
11. `creamglossyenamel`
12. `blackmatteenamel`

### 8.2 Masonry, Mineral, and Ground Materials

13. `lightgreyroughconcrete`
14. `charcoalpolishedconcrete`
15. `warmgreyweatheredconcrete`
16. `redweatheredbrick`
17. `brownroughbrick`
18. `creamglossyceramictile`
19. `charcoalmatteporcelaintile`
20. `mintglossyceramictile`
21. `softbeigemattesandstone`
22. `darksmoothslate`
23. `lightgreypolishedgranite`
24. `whiteveinedpolishedmarble`
25. `greenveinedpolishedmarble`
26. `creamhonedlimestone`
27. `twotonespeckledterrazzo`
28. `darkgreyroughasphalt`
29. `lightgreyroughgravel`
30. `beigeweatheredstone`

### 8.3 Metals

31. `silverbrushedmetal`
32. `silverpolishedstainlesssteel`
33. `greyroughsteel`
34. `blackmattesteel`
35. `silverpolishedaluminium`
36. `blackanodizedaluminium`
37. `greyroughiron`
38. `copperpolishedmetal`
39. `copperweatheredmetal`
40. `goldpolishedbrass`
41. `brownagedbronze`
42. `silverbrushedtitanium`
43. `greyweatheredzinc`
44. `chromemirrorsteel`
45. `redpaintedsteel`
46. `yellowpaintedsteel`

### 8.4 Wood and Plant-Derived Materials

47. `ambermatteoakwood`
48. `naturalroughoakwood`
49. `brownpolishedwalnutwood`
50. `darkbrownmattewalnutwood`
51. `lightbeigemattebirchwood`
52. `ambermattepinewood`
53. `redbrownweatheredcedarwood`
54. `naturalmattebamboo`
55. `lightbeigeroughcork`
56. `amberhorizontaloakboards`
57. `brownverticalwalnutboards`
58. `whitepaintedwood`

### 8.5 Glass

59. `transparentglass`
60. `smokytransparentglass`
61. `bluetransparentfrostedglass`
62. `greentransparentribbedglass`
63. `ambertranslucentmottledglass`
64. `silvermirrorglass`
65. `transparentcrystalglass`
66. `blackopaqueglass`
67. `warmwhiteemissiveglass`
68. `redtransparentglass`

### 8.6 Polymers and Composites

69. `blackmatteplastic`
70. `whiteglossyplastic`
71. `softyellowglossyplastic`
72. `redroughplastic`
73. `transparentacrylic`
74. `smokytransparentpolycarbonate`
75. `charcoalroughrubber`
76. `blackmattesilicone`
77. `blackwovenmattecarbonfiber`
78. `whiteglossyfiberglass`
79. `greyroughcomposite`
80. `ambertransparentplastic`

### 8.7 Textiles and Leather

81. `offwhitewovenmattefabric`
82. `charcoalwovenmattefabric`
83. `sagegreenwovenfabric`
84. `redvelvetfabric`
85. `bluecanvasfabric`
86. `greyfeltfabric`
87. `creamlinenfabric`
88. `whitecottonfabric`
89. `brownagedleather`
90. `blacksmoothleather`

### 8.8 Landscape and Emissive Materials

91. `sagegreenmattegrass`
92. `darkbrownroughsoil`
93. `brownweatheredbark`
94. `greenmattefoliage`
95. `pinkgreenfloralfabric`
96. `cyanglowingneon`
97. `magentaglowingneon`
98. `amberglowingneon`
99. `redemissiveplastic`
100. `whiteemissiveceramic`

## 9. Existing-Name Migration Proposal

The highest-use existing names should receive explicit migration decisions.

| Existing name | Proposed canonical treatment | Migration note |
|---|---|---|
| `redmetallicpaint` | valid canonical-v2 name | changes from accidental metal-family interpretation to metallic paint; requires visual approval |
| `charcoalroughmetal` | valid after adding `charcoal` | palette changes from fallback grey/white to charcoal; preserve legacy rendering until approved |
| `charcoalroughrubber` | valid after adding `charcoal` and `rubber` | changes from generic/plaster fallback to rubber |
| `glossblackpaint` | `blackglossypaint` | alias and ordering correction |
| `sagegreenmattepaint` | valid after adding `sage` | becomes a two-color sage/green palette |
| `amberwarmwood` | `warmamberwood` | ordering correction without inventing a finish |
| `lightgreyroughconcrete` | valid after adding `light` | tone becomes explicit |
| `warmwhitematteplaster` | valid after adding `warm` and separating plaster from paint | default material requires a frozen compatibility fixture |
| `offwhiteceramic` | valid after adding `offwhite` | off-white becomes one longest-match color lexeme |
| `pastelbluemattepaint` | valid after adding `pastel` | tone becomes explicit |
| `softbeigemattesandstone` | valid after adding `soft` and `sandstone` | changes from generic stone to sandstone subtype |
| `darkslatesmoothstone` | `darksmoothslate` | removes redundant `stone` and uses slate as the substance |
| `offwhitefabric` | valid after adding `offwhite` and `fabric` | changes from fallback material to textile |
| `creamtiledceramic` | `creamgridceramic` or `creamceramictile` | author must choose whether `tiled` describes pattern or substance |
| `steelshinyroughmetal` | manual resolution | `glossy` and `rough` conflict; choose `glossysteel` or `roughsteel` |
| `sportredpaint` | manual resolution | `sport` is a design-role word, not a material property |
| `darkasphalt` | valid after adding `dark` and `asphalt` | changes from plaster fallback to ground material |
| `bluepipe`, `redpipe` | manual resolution | `pipe` is geometry; choose a real substance such as `bluepaintedsteel` |
| `floortex` | replace with a real canonical material or `usertextureN` | no material semantics can be inferred safely |

No migration should infer missing physical intent when multiple canonical outcomes are plausible.

## 10. Compatibility Strategy

### 10.1 Freeze the Existing Baseline

Before changing interpretation, generate a compatibility fixture for every observed legacy material name containing:

- authored name;
- scene-catalog slot name;
- recognized lexemes;
- ignored fragments;
- interpreted family;
- complete scalar PBR specification;
- palette values;
- procedural seed;
- generated texture hashes; and
- representative preview image hash where deterministic capture is available.

### 10.2 Version the Language

Introduce:

```cpp
enum class MaterialLanguageVersion
{
    LegacyV1,
    CanonicalV2
};
```

Existing documents without a recorded material-language version remain `LegacyV1`. New documents may select `CanonicalV2`. The persistent owner of this version must be document metadata or an explicit grammar declaration; it must not be inferred from file age or material spelling.

### 10.3 Preserve Legacy Seeds

`LegacyV1` continues seeding from the exact legacy name. `CanonicalV2` seeds from the canonical name. A migration preview must show when a seed change alters generated texture detail.

### 10.4 Make Migration Reviewable

The migration service should produce:

- original and proposed material names;
- semantic differences;
- PBR property differences;
- texture-hash differences;
- unresolved choices; and
- exact source replacements.

Migration remains proposal-only until the owning editor transaction applies the reviewed replacements.

## 11. Implementation Phases

### Phase 0: Evidence Freeze

- Inventory every literal material name from authoritative source and generated-source owners.
- Distinguish canonical source from derived copies before counting migration work.
- Freeze current interpretation and texture evidence for all observed names.
- Record the current source hash and build configuration used to generate evidence.

### Phase 1: Canonical Object Model

- Add the model classes in Section 7.
- Add `MaterialLanguageVersion` and `MaterialRenderFamily`.
- Extract lexeme definitions from the anonymous implementation table into `MaterialLexiconCatalog`.
- Preserve current behavior under `LegacyV1`.

### Phase 2: Complete Parsing and Diagnostics

- Record covered and uncovered character ranges.
- Add alias expansion and canonical ordering.
- Diagnose unknown fragments, repeated substances, incompatible lexemes, and redundant family suffixes.
- Keep diagnostics advisory for `LegacyV1` and fail closed for canonical-v2 publication.

### Phase 3: High-Value Vocabulary Expansion

- Add the lexemes required to interpret current repository names completely.
- Prioritize `charcoal`, `metallic`, `warm`, `rubber`, `gloss`, `sage`, `light`, `offwhite`, `soft`, `slate`, `pastel`, `sandstone`, `fabric`, `cream`, `steel`, `terracotta`, `mint`, `asphalt`, and `bark`.
- Add complete canonical-name tests for every affected legacy name.

### Phase 4: Render-Family Expansion

- Separate plaster from paint.
- Add rubber, textile, leather, ground, polymer, and composite strategies.
- Add subtype overrides for stone, wood, metal, glass, ceramic, and polymer substances.
- Replace string family dispatch with `MaterialRenderFamily` dispatch.

### Phase 5: Material Library and Autocomplete

- Present lexemes in their semantic categories.
- Suggest only order-valid and family-compatible continuations.
- Show canonical output before insertion.
- Add curated filters for architecture, automotive, furniture, landscape, industrial, glass, textiles, and emissive materials.
- Display legacy, alias, conflict, and unknown-fragment warnings.

### Phase 6: Repository Migration

- Generate a migration report for all authoritative material references.
- Apply only unambiguous, reviewed replacements.
- Leave ambiguous names unresolved with explicit diagnostics.
- Regenerate derived artifacts from their authoritative generators instead of editing generated copies independently.

### Phase 7: Visual Acceptance and Promotion

- Compare legacy and canonical previews for all changed high-use materials.
- Approve expected semantic corrections separately from compatibility-preserving changes.
- Run the complete material, scene-admission, grammar-corpus, GUI build, and representative visual suites.
- Promote canonical-v2 defaults only after exact evidence review.

## 12. Validation Requirements

### 12.1 Lexicon Validation

- Every canonical lexeme has one semantic category.
- Every alias resolves to a canonical lexeme or canonical sequence.
- Longest-match collisions are enumerated and tested.
- No canonical substance is reachable through accidental substring scanning.
- Every subtype maps to exactly one render family.

### 12.2 Canonicalization Validation

- Capitalization and separators normalize deterministically.
- Alias spellings produce one canonical name.
- Canonical names are idempotent under repeated canonicalization.
- Unknown fragments are returned with exact source ranges.
- Conflicting and redundant lexemes produce deterministic diagnostics.
- `usertextureN` behavior remains unchanged.

### 12.3 Corpus Validation

- Every authoritative `material(...)` reference is classified as canonical, legacy-compatible, alias, ambiguous, or invalid.
- Canonical-v2 examples contain zero unknown fragments.
- The catalog contains no duplicate canonical names.
- All 100 proposed catalog names parse completely and route to the intended render family.
- Generated copies match their authoritative generators after migration.

### 12.4 Renderer Validation

- Every render family creates complete preview resources.
- PBR values remain finite and within declared ranges.
- Same canonical name, language version, and seed basis produce identical specification and texture hashes.
- Different aliases of one canonical-v2 material produce the same canonical specification.
- Legacy compatibility fixtures retain their frozen specification and seed behavior.

### 12.5 Visual Validation

At minimum, capture representative materials for:

- matte and metallic paint;
- plaster and concrete;
- polished and weathered stone;
- brushed steel and anodized aluminium;
- clear and frosted glass;
- oak, walnut, and boards;
- plastic and rubber;
- woven fabric and leather;
- grass, soil, and asphalt;
- carbon fiber; and
- neon and emissive surfaces.

Visual approval must distinguish deliberate semantic improvement from accidental source drift.

## 13. Acceptance Gates

Canonical Material Language v2 is ready for default use only when:

1. one catalog owns all material lexemes, aliases, substances, and render-family relationships;
2. canonical-v2 parsing silently ignores zero characters;
3. all 100 proposed catalog names pass deterministic parse and render-family tests;
4. the current repository material inventory has an explicit classification and migration result;
5. legacy fixtures preserve current accepted behavior until migration approval;
6. scene material slots, material interpretation, autocomplete, and preview rendering use the same canonical name;
7. no generated artifact is edited independently of its authoritative source;
8. focused material tests, full test discovery, the complete GUI build, and representative visual tests pass from one frozen source snapshot; and
9. semantic visual changes receive explicit human approval.

## 14. Recommended First Delivery Slice

The first implementation slice should not attempt all render families. It should establish the canonical foundation and solve the largest current failures:

1. create `MaterialLexiconCatalog`, `MaterialNameParsingService`, `MaterialNameCanonicalizationService`, and `MaterialNameInterpretationReport`;
2. retain current interpretation behind `LegacyV1`;
3. add complete coverage diagnostics;
4. add high-use tone and color lexemes;
5. add `metallic`, `rubber`, `fabric`, `steel`, `sandstone`, `slate`, and `asphalt`;
6. unify `SceneMaterialCatalog` with the canonicalization service;
7. add deterministic fixtures for the 79 observed names; and
8. add canonical parse tests for the first 100 catalog names.

This slice creates a stable semantic contract before new procedural texture branches are added.

## 15. Recommendation

Proceed with the proposal as an additive **Canonical Material Language v2**, not as an in-place expansion of the anonymous lexeme table.

The immediate value is not merely more names. The larger benefit is that authored material names become complete, diagnosable, canonical semantic objects shared by scene admission, autocomplete, procedural interpretation, rendering, migration, and evidence.
