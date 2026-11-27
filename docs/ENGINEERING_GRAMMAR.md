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

The airflow action family is described in §9. These actions configure a
following movable primitive rather than directly modifying a transform.

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

### 6.3 Material names

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
`tolerance(value)`, and `clearance(...)`.

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

Airflow declarations use three whitespace-separated expressions:

```p3d
AirProperties(density dynamicViscosity)
Wind(x y z)
```

The recognized three-expression body actions are:

```text
BodyInertia(i1 i2 i3)  LoadPoint(x y z)  LoadAxis(x y z)
BodyForce(x y z)       BodyTorque(x y z) ForceRamp(start end scale)
DragSurface(cd area length)  LiftSurface(cl area length)
PropSample(advance thrust power)  Propeller(diameter rpm spin)
DuctStation(flow pressure area)  PlateFriction(area length regime)
```

The implementation requires finite values and imposes additional invariants
(positive inertia components, a nonzero load axis, increasing propeller/duct
samples, and a valid propeller table, among others). The declaration sequence
applies to the next `I(...)` body; it must meet the preflight rules in
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
| Lighting declarations | `src/grammar/service/LightingDeclarationParser.cpp` |
| Airflow action names and runtime checks | `src/physics/model/BodyAirflow.cpp` |
| Detailed authoring examples | `examples/Progen3D_Example_Grammar_Corpus/reference/Progen3D_Grammar_Writing_Guide.md` |

