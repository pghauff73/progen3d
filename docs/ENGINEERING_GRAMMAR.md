# ProGen3D Engineering Grammar Reference

This document describes the executable source grammar and its implementation
boundaries. It is intended for grammar authors, parser/runtime contributors, and
reviewers who need to distinguish accepted syntax from complete runtime
support.

The grammar is implemented in C++, not generated from this document. The
authoritative implementation is `src/Grammar.cpp`; shape descriptors are parsed
and evaluated by `src/grammar/service/ShapeSpecificationParser.cpp` and
`ShapeSpecificationEvaluator.cpp`; spatial and lighting declarations have
dedicated parsers in `src/grammar/service/`. A parser accepting a name or
descriptor does not, by itself, guarantee that every geometry, export, render,
collision, or simulation path supports it.

For explanatory examples and authoring rationale, see the
[grammar writing guide](../examples/Progen3D_Example_Grammar_Corpus/reference/Progen3D_Grammar_Writing_Guide.md).
The example grammar corpus is a partial archive: its historical `current/`
fixtures are not present in this checkout.

## 1. Execution model

Grammar processing has two main phases:

1. **Parse and validate:** source is grouped into logical rules, tokenized,
   parsed into rule/action objects, and checked for structural and semantic
   errors.
2. **Expand and execute:** the selected production, random values, parameters,
   repeats, conditions, and rule calls are resolved into an action stream.
   Transforms, geometry instances, physics state, and supported declarations
   are then applied to a scene-generation context.

Random declarations (`R`, `R*`), conditional branches, and rule calls are
expansion constructs; they are not ordinary scene actions. Expressions are
evaluated before action execution. Blocking diagnostics prevent scene execution
and partial expansion is not published as a valid scene.

## 2. Source and lexical form

### 2.1 Rules and logical lines

A rule consists of a header, a primary production, and optionally one alternate
production:

```text
RuleName(parameters) repeat [reroll-name ...] [; probability] -> production
                                                    [-> alternate-production]
```

Rules may span physical lines. A new logical rule starts on a nonblank,
noncomment line containing `->`, unless the trimmed line begins with `->`; a
leading arrow continues the preceding rule as its alternate. Blank lines and
comment-only lines do not end a logical rule.

The parser moves the first rule whose name is exactly `Start` or `start` to the
front. If neither appears, the first parsed rule is the entry rule. Entry rules
receive zero arguments and therefore cannot declare parameters.

Rule names, parameters, variables, shape names, action names, and declaration
field names are case-sensitive.

### 2.2 Identifiers, numbers, comments

Identifiers use:

```text
[A-Za-z_][A-Za-z0-9_]*
```

`t` is a reserved immutable time input. A rule named `RuleName` also reserves
`RuleName_count` for its generated iteration counter.

Numbers are finite decimal or scientific-notation values. Expressions and
resolved values must be representable as finite, nonzero `float` values (zero
is valid); overflow and nonzero underflow are rejected.

`#` and `//` begin comments through the end of the physical line. Comment
recognition is not string-aware. Backslashes outside comments are rejected.
Source strings are not a general language feature; material names and
declaration identifiers are token-like values.

### 2.3 Token boundaries and arguments

The tokenizer separates `->`, `(`, `)`, `[`, `]`, `{`, `}`, `|`, `;`, `?`, and
`:`. Commas and arithmetic operators are not independent token boundaries.

- Action arguments and rule-call arguments are whitespace-separated.
- Function arguments are comma-separated.
- Compact expressions may contain arithmetic operators without whitespace.

```p3d
S(width height depth)
Panel(width height/2)
lerp(0,1,progress)
```

Do not use commas for action or rule-call arguments, or whitespace in place of
function-argument commas.

## 3. Rule headers and productions

### 3.1 Parameters, repeats, rerolls, and alternates

```p3d
Panel(width height) ->
    S(width height 1)
```

Call parameters are whitespace-separated: `Panel(2 3)`. Each call must supply
exactly the declared arity. Bare variable arguments preserve random-range
metadata where applicable; calculated arguments pass an immutable value.
Parameters shadow caller-visible values. A local random declaration may not
redeclare a parameter.

A repeat count follows the optional parameter list:

```p3d
Row(8) 8 -> Item
Row(count) count+1 -> Item
```

The repeat expression is one compact header token; grouped expressions and
function calls are not supported there. It is converted to an integer by
truncation toward zero. A zero or negative count emits no setup, repeated body,
or tail.

Identifiers after the repeat expression and before `;` or `->` bind reroll
values from caller-visible random variables:

```p3d
Panel 1 Width Height -> ...
```

The first token after the rule name/parameters is the repeat expression; use
`1` for a single call with rerolls and no repetition.

A probability clause is a finite literal in `[0,1]`:

```p3d
Cap ; 0.75 -> Primary
-> Alternate
```

The alternate has probability `1-p`. Selection occurs once per invocation,
including repeated invocations. At most one alternate is accepted. A
probability without an alternate does not branch.

### 3.2 Production sections

A production has one to three nonempty sections separated by `|`:

| Sections | Meaning |
|---|---|
| `body` | Body repeats |
| `setup | body` | Setup executes once, then body repeats |
| `setup | body | tail` | Setup executes once, body repeats, tail executes once |

For positive repeat count `n`:

```text
expand(setup) + expand(body, counter=0..n-1) + expand(tail)
```

The generated `<RuleName>_count` exists in the repeated body and tail, not
setup; the tail sees the final counter value. There may be at most three
sections in either production.

### 3.3 Bracket scopes and conditionals

`[...]` clones the current transform scope and restores the parent after the
closing bracket. It is the supported transform-isolation construct.

```p3d
Start ->
    [ T(-1 0 0) Part ]
    [ T( 1 0 0) Part ]
```

Conditional syntax is:

```p3d
?(condition) true-sequence : false-sequence
```

The false branch consumes the remainder of its current production section or
bracket block. A conditional is therefore a tail construct: subsequent source
belongs to the false branch. There are no general `&&`, `||`, or logical-not
operators. Braces and standalone `*` are discarded by the source parser; do
not use them as syntax.

## 4. Expressions, variables, and time

### 4.1 Arithmetic

Expressions support finite numeric literals, visible variables, parentheses,
unary `+`/`-`, and `+`, `-`, `*`, `/`, `^`. Exponentiation is right-associative;
division by a value with magnitude at most `1e-12` is rejected. Expression
results are checked for finite-float representability.

`&name` requests a fresh sample from the original range of a resampleable
random variable without replacing its stored value. It is not a general
address-of operator.

### 4.2 Random values and visibility

```p3d
R Width(0.5 1.5)
R* Floors(2 6)
```

`R` samples a continuous value between evaluated bounds. `R*` samples an
integer from `ceil(minimum)` through `floor(maximum)`; an empty integer range
is invalid. Bounds are evaluated in order. Variables are visible in their
current rule frame and caller frames. Rule-local declarations disappear when
that invocation returns.

### 4.3 Built-in functions

Function names are lowercase and function arguments are comma-separated.

| Function | Arity | Behavior |
|---|---:|---|
| `sin`, `cos`, `tan` | 1 | Trigonometry; input is radians |
| `radians`, `degrees` | 1 | Angle-unit conversion |
| `abs`, `sqrt`, `floor`, `ceil` | 1 | `sqrt` requires a nonnegative argument |
| `min`, `max` | 2 | Pairwise minimum or maximum |
| `clamp` | 3 | Clamp value between ordered bounds |
| `wrap` | 3 | Wrap into `[minimum, maximum)`; maximum must exceed minimum |
| `lerp` | 3 | Unclamped linear interpolation |
| `ease`, `smoother` | 1 | Clamped cubic smoothstep / quintic smootherstep |
| `accelerate`, `decelerate` | 1 or 2 | Clamped easing; optional positive exponent defaults to 2 |
| `cycle`, `pingpong` | 2 or 3 | Periodic sawtooth / triangle; optional phase defaults to 0 |
| `pulse` | 3 or 4 | Periodic 0/1 pulse; duty must be in `[0,1]`, optional phase defaults to 0 |
| `swing`, `oscillate` | 4 or 5 | Triangle / sinusoidal motion between ordered bounds; optional phase defaults to 0 |
| `spin` | 2 or 3 | `time * rate + offset`; optional offset defaults to 0 |

Periodic functions require a positive period. `swing` and `oscillate` permit
equal bounds.

### 4.4 Conditions

Conditions accept one arithmetic expression, or one comparison using `<`, `>`,
`<=`, `>=`, `==`, or `!=`. A numeric condition is true when its absolute value
exceeds `1e-6`; equality and inequality use the same absolute epsilon.
Comparisons are numeric, not general boolean expressions.

### 4.5 Time input

`t` is an immutable value in seconds supplied to each grammar rebuild. Use
`cycle`, `pingpong`, `pulse`, `swing`, `oscillate`, or `spin` to derive
time-dependent values. Random choices remain stable across timeline samples
when the editor reuses the same design seed; this is editor behavior, not a
second grammar variable.

## 5. Scene actions

All action expressions are evaluated before execution. The ordinary action
catalog is:

| Action | Arguments | Effect |
|---|---:|---|
| `S(x y z)` | 3 | Scale current primary and secondary transforms |
| `T(x y z)` | 3 | Translate current primary and secondary transforms |
| `A(angle axis)` | 2 | Rotate by degrees around axis `0=X`, `1=Y`, `2=Z` |
| `A(angle axis low high)` | 4 | Clamp angle to ordered bounds before rotation |
| `D(x y z)` | 3 | Change the secondary transform |
| `DSX`, `DSY`, `DSZ` | 3 each | Change deformation scale on the selected minimum face |
| `DTX`, `DTY`, `DTZ` | 3 each | Change deformation translation on the selected minimum face |
| `I(...)`, `!I(...)` | instance form | Add movable or immovable geometry |
| `V(x y z)`, `VR(x y z)` | 3 each | Set pending linear or rotational velocity |
| `M(mass)` | 1 | Set pending mass |
| `P(density)` | 1 | Set pending density-derived mass where supported |
| `G(magnitude x y z)` | 4 | Set persistent gravity magnitude and direction |

Transforms compose in source order. Pending velocity, rotational velocity, mass,
and density apply to the next instance attempt and are cleared after instancing.
`!I` ignores velocity. Density-derived mass requires compatible closed
watertight geometry; supported shape and collision behavior vary.

The airflow action family is described in §8. These actions configure the
next eligible `I(...)` instance when an airflow configuration is active rather
than directly modifying a transform.

## 6. Geometry instances and shape specifications

### 6.1 Instance syntax and appearance

```p3d
I(shape [shape-descriptor] appearance [alpha [texture-scale]])
!I(shape [shape-descriptor] appearance [alpha [texture-scale]])
```

For example:

```p3d
I(Cube material(warmwhitematteplaster) 1 0.5)
!I(Cylinder material(lightgreyroughconcrete) 1 0.5)
```

The instance parser accepts one to three appearance arguments after the shape
descriptor: a required material or index, optional alpha, and optional texture
scale. Runtime defaults for omitted alpha and texture scale are `0.0` and
`0.125`. Explicit material syntax is `material(name)`; explicit legacy index
syntax is `index(expression)`. A bare identifier is ambiguous: if it resolves
to a visible variable it is an index, otherwise it is treated as a material
name.

### 6.2 Recognized shape names

The fixed built-in names are `Cube`, `CubeX`, `CubeY`, `CubeZ`, `Cylinder`, and
`Sphere`. `ShapeSpecificationParser` also recognizes these canonical
shape-specification names:

```text
AxialProfile  Extrude  SweepProfile  VariableSectionSweep  SweepDisk
Revolve  Loft  SurfaceLoft  CurveNetworkSurface  ShellOffset  MirrorShape
CurvedPanel  FormedPanel  HostedOpening  EmbossedBead  EdgeFlange
PanelCut  ShellLoft  FoldedProfile  InstanceArray  CompoundShape
VehicleBodyShell  GeneratedMeshReference  TaperedSweep  BranchJunction
LeafBlade  PetalBlade  Plant  Vine  ScatterRegion
```

The parser also recognizes aliases:

```text
Tube  CylinderSector  DSection  HalfCylinder  Hemisphere  SphereCap
SphereBowl  SphereQuarter  SphereOctant
```

Use the exact case shown. Aliases lower to canonical families; each family has
its own option schema and validation. Many procedural families require a nested
descriptor, introduced immediately after the shape name:

```p3d
I(AxialProfile(axis(y) ...) material(name) 1 0.5)
I(SweepDisk(path(...) radius(...) ...) material(name) 1 0.5)
```

Descriptor options are not interchangeable across families. The parser tables
and evaluator in `src/grammar/service/ShapeSpecificationParser.cpp` and
`ShapeSpecificationEvaluator.cpp` define the complete supported option names,
arity, and semantic constraints. Consult those implementations and the
family-specific validators before adding or relying on a descriptor. A known
name may have only parsing/storage support, or incomplete downstream behavior;
do not infer complete geometry, rendering, collision, or physics from name
recognition.

STL catalog/UI support does not make `STL.*` an accepted source instance name.

### 6.3 Descriptor syntax and option arities

The following inventory describes the parser schema for nested shape
descriptors. An arity is the number of whitespace-separated expression
arguments inside an option's parentheses. Each option may appear at most once;
the listed ranges are inclusive. These are parser constraints, not guarantees
that every value is semantically valid or that the resulting shape works in
every runtime subsystem.

| Descriptor family | Options and accepted argument counts |
|---|---|
| `Cylinder`, `Tube`, `CylinderSector`, `DSection`, `HalfCylinder` | `radial(2)`, `wall(1)`, `axial(2)`, `azimuth(2)`, `chord(3)`, `clip(5)`, `topology(1)`, `close(1–4)`, `segments(1–2)`, `mapping(1)` |
| `Sphere`, `Hemisphere`, `SphereCap`, `SphereBowl`, `SphereQuarter`, `SphereOctant` | `radial(2)`, `wall(1)`, `polar(2)`, `azimuth(2)`, `clip(5)`, `slab(5)`, `topology(1)`, `close(1–4)`, `segments(1–2)`, `mapping(1)` |
| `TaperedSweep` | `samples(8–8192)`, `up(3)`, `segments(1)`, `cap(1)` |
| `BranchJunction` | `core(2)`, `parent(5)`, `child(5)`, `segments(1)` |
| `SweepProfile` | `profile(3–4097)`, `path(6–24576)`, `up(3)`, `cap(1)`, `detail(1)` |
| `VariableSectionSweep` | `path(6–24576)`, `station(3–4104)`, `stationTransform(6)`, `nominalSection(2–4097)`, `up(3)`, `frame(1)`, `cap(1)`, `detail(1)` |
| `SweepDisk` | `path(6–24576)`, `curve(7–12289)`, `up(3)`, `radius(1)`, `radiusStart(1)`, `radiusEnd(1)`, `longitudinalSegments(1)`, `radialSegments(1)`, `segments(1)`, `cap(1)`, `detail(1)` |
| `Revolve` | `profile(3–4097)`, `angle(2)`, `segments(1)`, `cap(1)`, `detail(1)` |
| `Loft` | `section(4–4098)`, `cap(1)`, `detail(1)` |
| `SurfaceLoft` | `section(4–4098)`, `detail(1)` |
| `CurveNetworkSurface` | `uCurve(7–12289)`, `vCurve(7–12289)`, `samplesU(1)`, `samplesV(1)`, `tolerance(1)`, `method(1)`, `detail(1)` |
| `ShellLoft` | `section(14–8194)`, `cap(1)`, `detail(1)` |
| `FoldedProfile` | `path(4–8192)`, `thickness(1)`, `depth(1)`, `cap(1)`, `detail(1)` |
| `CurvedPanel` | `width(1)`, `height(1)`, `curvature(1–2)`, `thickness(1)`, `segments(1–2)`, `detail(1)` |
| `FormedPanel` | `section(4–4098)`, `thickness(1)`, `side(1)`, `detail(1)` |
| `HostedOpening` | `host(2–4097)`, `opening(2–4097)`, `depth(1)`, `cap(1)`, `detail(1)` |
| `EmbossedBead` | `path(6–24576)`, `up(3)`, `width(1)`, `depth(1)`, `shoulder(1)`, `side(1)`, `end(1)`, `detail(1)` |
| `EdgeFlange` | `path(6–24576)`, `up(3)`, `width(1)`, `thickness(1)`, `angle(1)`, `bend(1)`, `side(1)`, `detail(1)` |
| `PanelCut` | `path(6–24576)`, `gap(1–2)`, `up(3)`, `cap(1)`, `detail(1)` |
| `VehicleBodyShell` | `thickness(1)`, `section(7)`, `cap(1)`, `detail(1)` |
| `GeneratedMeshReference` | `meshKey(1)`, `detail(1)`, `topology(1)` |
| `LeafBlade`, `PetalBlade` | `profile(1)`, `length(1)`, `width(1)`, `curvature(1)`, `camber(1)`, `twist(1)`, `thickness(1)`, `widthPower(1)`, `segments(2)` |
| `Plant` | `species(1)`, `architecture(1)`, `age(1)`, `state(1)`, `seed(1)`, `generation(1)`, `lAxiom(1–128)`, `lRule(2–128)`, `lSystem(6)`, `trunk(4)`, `branching(12)`, `leaf(7)`, `petiole(3)`, `leafArray(8)`, `phyllotaxis(3–7)`, `organArray(10)`, `flower(8)`, `flowerHead(6)`, `inflorescence(7)`, `fruit(6)`, `whorl(4)`, `growth(8)`, `tropism(5)`, `crown(5–9)`, `crownCustom(1)`, `crownSample(3)`, `spaceColonization(8)`, `crownObstacle(8)`, `floweringAge(1)`, `matureAge(1)`, `detail(1)` |
| `Vine` | `species(1)`, `start(3)`, `direction(3)`, `radius(1)`, `mode(1)`, `collision(1)`, `attachment(1)`, `step(1)`, `segments(1)`, `seekDistance(1)`, `attachDistance(1)`, `tolerance(1)`, `radiusDecay(1)`, `minimumRadius(1)`, `preferred(3)`, `gamma(1)`, `target(7)`, `obstacle(8)`, `detail(1)` |
| `ScatterRegion` | `region(1)`, `species(1)`, `seed(1)`, `surface(7)`, `face(1)`, `density(1)`, `separation(1)`, `scale(2)`, `orientation(1)`, `collisionRadius(1)`, `layer(1)`, `mask(1–10)`, `obstacle(8)`, `detail(1)` |

Alias-only options are `Tube.inner(1)`, `CylinderSector.sweep(1)`,
`DSection.angle(1)/offset(1)/side(1)`,
`HalfCylinder.angle(1)/side(1)`,
`Hemisphere.axis(1)/side(1)`,
`SphereCap.axis(1)/offset(1)/side(1)`, and
`SphereBowl.inner(1)/axis(1)/offset(1)/side(1)`. Cylinder aliases inherit
the `Cylinder` option set; sphere aliases inherit the `Sphere` option set.
`MirrorShape` and `ShellOffset` use a nested-source form instead of the generic
option table:

```p3d
MirrorShape(source(Cube) plane(0 1 0) mode(keepBoth) detail(LOD3))
ShellOffset(source(Sphere) thickness(0.1) side(both) detail(LOD3))
```

Four descriptors have dedicated syntax rather than an option map:

- `AxialProfile` requires `axis(x|y|z)`, one or more
  `profile(name polygon(x1 z1 ...))` definitions, and exactly one
  `at(position profileName [center(x z) scale(x z) rotate(degrees)])`.
  Optional `hold(position)`, `linear(position profileName ...)`, and
  `step(profileName ...)` transitions follow `at`; optional `cap(none|all|
  bottom|top)` must be last.
- `Extrude` takes one `Rect`, `RoundedRect`, `Circle`, `Ellipse`,
  `ChamferRect`, or `Polygon` profile constructor, then a depth expression,
  and optionally `cap(all|none|front|back)`.
- `InstanceArray` requires `source(shapeDescriptor)` and exactly one
  `linear(e e e e)`, `grid(e e e e)`, or `radial(e e e e e e)` pattern;
  `detail(e)` is optional.
- `CompoundShape` requires one or more
  `part(purpose source(shapeDescriptor) [transform(16 expressions)])`
  statements; an optional `detail(e)` is allowed. A missing transform is the
  identity matrix.

The remaining nested-source descriptors require exactly one `source(...)`.
The shape parser and evaluator are the source of truth if these schemas change.

### 6.4 Material names

Materials are compact descriptor names, not quoted free-form strings. Explicit
material names are canonicalized by the runtime. The editor recognizes lexical
groups for colors, opacity, material families, finishes, and wood species; use
compact forms such as:

```p3d
material(silvermetalpolishedreflective)
material(cyantransparentglassglossy)
```

The [writing guide's material section](../examples/Progen3D_Example_Grammar_Corpus/reference/Progen3D_Grammar_Writing_Guide.md#14-material-descriptor-language)
lists the editor vocabulary and legacy material-index table.

## 7. Spatial, lighting, and vehicle declarations

These are grammar declarations parsed as structured fields, not ordinary rule
calls. Field names are case-sensitive, duplicate fields are rejected, and
expressions inside vector/scalar fields use the grammar expression evaluator.
Declarations are finalized into the runtime model; declarations that refer to
missing IDs or unsupported relationships produce diagnostics.

### 7.1 Spatial objects

`Object(...)` requires `id(...)` and `class(...)`; optional fields are
`name(...)`, `container(...)`, `taxonomy(...)`, `layer(...)`, and `mask(...)`.
It must be followed by a bracketed body:

```p3d
Object(id(Wall) class(PhysicalElement) name(ExternalWall))
[
    I(Cube material(whitepaintmatte) 1 0.5)
    Interface(id(exterior) type(EnvelopeSurface) ...)
]
```

The body establishes object ownership and may contain geometry and nested
declarations. A geometry-free semantic object is valid when the model allows
it.

### 7.2 Interfaces and relationships

`Interface(...)` requires `id(...)` and `type(...)`. Supported fields:
`origin(x y z)`, `normal(x y z)`, `tangent(x y z)`,
`region(Point)`, `region(PlaneRectangle width height)`,
`region(AxisSegment length)`, `region(ObjectBoundaryFace name)`,
`tolerance(value)`, and `clearance(nominal)` or
`clearance(nominal minimum maximum)`. The one-value form uses the nominal
expression for all three values.

`Connect(...)` requires `id(...)`, `source(object interface)`,
`target(object interface)`, and `type(...)`. Optional fields are
`clearance(...)` and `insertionDepth(value)`.

`Position(...)` requires `id(...)`, `moving(interface)`,
`target(object interface)`, and `mode(...)`. Optional fields:
`direction(frame x y z)`, `clearance(...)`, `seatingDepth(value)`,
`maxDistance(value)`, `tolerance(value)`, `priority(value)`, and `mask(...)`.
Supported positioning modes and solving behavior are runtime constraints, not
arbitrary names accepted as guaranteed-success requests.

### 7.3 Vehicle joints

`Joint(...)` requires `id(...)`, `type(...)`, `source(object interface)`, and
`target(object interface)`. Optional fields are `axis(x y z)`,
`limits(min max)`, and `state(value)`.

### 7.4 Lighting and electrical controls

- `Light(id ...)`: `type`, `shadow`, `reference`, `twoSided`, `position`,
  `direction`, `color`, `cone`, `area`, `temperature`, `range`, `exposure`,
  and one intensity unit: `lumens`, `candela`, or `lux`.
- `LightFixture(id ...)`: `type`, `emitter`, `mount`, `shadow`, `position`,
  `direction`, `clearance`, `cone`, `temperature`, `lumens`, and `range`.
- `LightSwitch(id ...)`: `type`, `mount`, `state`, `position`, `height`, and
  `dimmer`.
- `LightingArray(id ...)`: `fixture`, `mount`, `shadow`, `circuit`, `origin`,
  `direction`, `clearance`, `grid`, `spacing`, `cone`, `temperature`, `lumens`,
  `range`, and `controls`.
- `LightingCircuit(id ...)`: `controls`, `fixtures`, and `alwaysOn`.
- `Controls(id target ...)`: a control identifier followed by its target
  identifier list.

Fields are specified using `field(value)` syntax. For example:

```p3d
Light(Entry type(Point) position(0 3 2) color(1 0.8 0.6) lumens(800))
```

Exact field types, required values, enumerations, and cross-reference rules are
defined in `src/grammar/service/LightingDeclarationParser.cpp` and the lighting
model validators.

## 8. Airflow grammar

The source actions `AirProperties` and `Wind` set the medium and wind vector.
`AirProperties` takes two whitespace-separated expressions; `Wind` takes
three. Airflow actions are accumulated and validated by the grammar executor,
then attached to the next eligible `I(...)` instance when the airflow state is
active and reset after that attempt. They do not attach merely because an
instance is described as a "body".

```p3d
AirProperties(density dynamicViscosity)
Wind(x y z)
```

The recognized airflow body actions and expression arities are:

| Action | Arguments |
|---|---:|
| `BodyInertia(i1 i2 i3)` | 3 |
| `LoadPoint(x y z)`, `LoadAxis(x y z)` | 3 each |
| `BodyForce(x y z)`, `BodyTorque(x y z)` | 3 each |
| `ForceRamp(start end scale)` | 3 |
| `DragSurface(cd area length)`, `LiftSurface(cl area length)` | 3 each |
| `PropSample(advance thrust power)` | 3 |
| `Propeller(diameter rpm spin)` | 3 |
| `DuctStation(flow pressure area)` | 3 |
| `PlateFriction(area length regime)` | 3 |

The implementation requires finite values and imposes additional invariants
(positive inertia components, a nonzero load axis, increasing propeller/duct
samples, and a valid propeller table, among others). The declaration sequence
must meet the preflight rules in
`src/physics/model/BodyAirflow.cpp`. This is a specialized grammar extension,
not a general-purpose fluid solver.

## 9. Structural limits and unsupported syntax

The current source language does not provide:

- general boolean operators or assignment statements;
- arbitrary strings or comma-separated action/rule arguments;
- named source scopes;
- source-level brace scopes (`{...}` is discarded);
- multiple alternate productions or more than three `|` sections;
- function calls or parenthesized expressions in rule-header repeats;
- STL instance references.

Expansion and execution have safety budgets (including recursion depth, rule
invocations, work units, emitted actions, primitive count, and repeat count).
Treat these as abort limits, not recommended design targets. Exact values are
implementation constants in `src/Grammar.cpp` and may change between versions.

## 10. Authoring and engineering checklist

Before accepting a grammar:

1. Define an explicit zero-parameter `Start` rule.
2. Confirm every rule call's target and arity; avoid hidden dependencies on
   caller-visible variables.
3. Keep repeat expressions compact and estimate worst-case recursive/repeated
   growth.
4. Put conditionals at the end of the intended section or bracketed body.
5. Balance all `[...]` scopes and use brackets for transform isolation.
6. Use explicit `material(...)` or `index(...)` instance appearance.
7. Check the descriptor schema and downstream runtime support for every
   non-built-in shape family.
8. Validate object/interface/connection/constraint references and inspect
   diagnostics for unsupported runtime behavior.
9. Keep an invalid fixture for diagnostics and a minimal valid example for
   every parser extension added.

## 11. Parser and runtime source map

| Concern | Implementation |
|---|---|
| Source grouping, token parsing, expansion, expression evaluation, semantic checks, limits | `src/Grammar.cpp` |
| Shape names, aliases, descriptor syntax | `src/grammar/service/ShapeSpecificationParser.cpp` |
| Shape expression lowering and validation dispatch | `src/grammar/service/ShapeSpecificationEvaluator.cpp` |
| Spatial object, interface, connection, constraint declarations | `src/grammar/service/Spatial*DeclarationParser.cpp` and `SpatialObjectSpecificationParser.cpp` |
| Vehicle joints | `src/grammar/service/VehicleJointDeclarationParser.cpp` |
| Lighting declarations, including `Controls` | `src/grammar/service/LightingDeclarationParser.cpp` |
| Airflow source actions and arity; attachment and execution | `src/Grammar.cpp` |
| Airflow value validation and simulation semantics | `src/physics/model/BodyAirflow.cpp` |
| Detailed authoring examples | `examples/Progen3D_Example_Grammar_Corpus/reference/Progen3D_Grammar_Writing_Guide.md` |
