# Progen3D Grammar

## Parser-Compatible EBNF and Grammar Writing Guide

**Guide status:** current implementation guide for the uploaded 21 August 2026 source snapshot.

**Normative source order:** `Grammar.cpp` defines source parsing, semantic validation, expression evaluation, stochastic expansion, conditionals, time, rule calls, and action emission. `Scope.cpp` defines transform behavior. `Context.cpp` defines instancing, material canonicalisation, pending physics state, gravity, and simulation state. `Mesh.cpp` defines the six currently usable primitive meshes. `imgui_main.cpp` supplies editor syntax help, procedural material lexemes, and deterministic time-sample seeding.

This guide describes what the current parser and runtime actually do. It does not silently promote unfinished UI or STL-catalog features into the language.

---

## 1. The language model

A Progen3D document is a procedural shape program built from named production rules.

```text
source text
  -> logical rules
  -> structural and semantic validation
  -> recursive stochastic expansion
  -> flat scene-action stream
  -> scope transforms, instancing, and physics setup
```

Random declarations, rule selection, rule parameters, repeat counts, rerolls, and conditionals are resolved during expansion. Transform, instancing, and physics actions are then executed in order against a scene-generation context.

This distinction matters:

- `R` and `R*` do not reach the scene action virtual machine.
- A conditional chooses which tokens are expanded.
- A rule call is replaced by its expanded actions.
- By scene execution time, action expressions have already been evaluated to finite floats.

---

## 2. Minimal grammar

```p3d
Start ->
    S(1 2 1)
    I(Cube material(softblueglossyceramic) 1 1)
```

`Start` is the recommended entry rule. `S` scales the current scope. `I` instantiates a primitive using the current scope.

The explicit material form is recommended:

```p3d
material(softblueglossyceramic)
```

The final two instance arguments are the legacy per-instance alpha field and texture scale. Supplying both, commonly `1 1`, avoids depending on defaults or renderer-path differences.

---

## 3. Canonical EBNF

The standalone EBNF file accompanies this guide. Its central document grammar is:

```ebnf
document   = spacing, rule, { spacing, rule }, spacing ;

rule       = rule-header, "->", production,
             [ "->", production ] ;

rule-header =
    identifier,
    [ parameter-list ],
    [ repeat-and-reroll-clause ],
    [ probability-clause ] ;

parameter-list =
    "(", [ identifier, { argument-separator, identifier } ], ")" ;

repeat-and-reroll-clause =
    compact-repeat-expression,
    { argument-separator, identifier } ;

probability-clause =
    ";", probability-literal ;

production =
    section,
    [ "|", section,
      [ "|", section ] ] ;

sequence =
    { item }, [ conditional ] ;

conditional =
    "?", "(", condition-expression, ")",
    sequence, ":", sequence ;
```

The full file also formalises all actions, rule calls, instancing forms, conditions, expressions, numbers, identifiers, comments, and the current primitive set.

### Why the EBNF has semantic notes

Several implementation rules are context-sensitive and cannot be expressed cleanly in ordinary EBNF:

- rule-call arity must match the target rule;
- an entry rule may not have parameters;
- a bare instance material identifier may become a material index when a visible variable has the same name;
- `<RuleName>_count` is created only during repeated-body expansion;
- variable visibility depends on the active rule-call frame;
- a conditional consumes the remainder of its enclosing sequence into its false branch;
- action arguments use whitespace, while function arguments use commas.

---

## 4. Lexical rules

### 4.1 Identifiers

```text
[A-Za-z_][A-Za-z0-9_]*
```

Identifiers are case-sensitive. `Tower`, `tower`, and `TOWER` are different names.

The built-in identifier `t` is reserved. Do not use `t` as a rule name, parameter, random variable, or header reroll binding.

Also reserve every generated counter name:

```text
<RuleName>_count
```

For example, the repeated-body counter for `Floor` is `Floor_count`.

### 4.2 Numbers

The runtime accepts finite floating-point numbers, including decimal and scientific notation:

```p3d
S(0.01 2e1 -3e-1)
```

Values must remain representable as nonzero finite `float` values. Overflow, nonfinite results, and nonzero values that underflow to zero are rejected.

### 4.3 Comments

Both comment forms run to the end of the physical line:

```p3d
# hash comment
// slash comment
```

Comment discovery is not string-aware. Avoid `#` or `//` inside material text.

### 4.4 Whitespace and punctuation

The lexer inserts token boundaries around:

```text
->  ( )  [ ]  { }  |  ;  ?  :
```

It does not separately tokenise commas or arithmetic operators.

The practical rule is:

- separate action arguments with whitespace;
- separate rule-call arguments with whitespace;
- separate function arguments with commas.

Correct:

```p3d
S(width height depth)
Child(width height/2)
lerp(0.5,1.5,progress)
```

Incorrect:

```p3d
S(width,height,depth)
Child(width,height/2)
lerp(0.5 1.5 progress)
```

A backslash outside a comment is rejected.

---

## 5. Logical rules and multiline layout

A logical rule starts at a noncomment line and continues until the next noncomment line that contains `->`, unless that next line begins with `->`.

Recommended multiline style:

```p3d
Start ->
    Ground
    Tower
    Beacon
```

A multiline alternate can place its second arrow at the beginning of a continuation line:

```p3d
Cap ; 0.7 ->
    PrimaryCap
->
    AlternateCap
```

A logical rule may contain:

- exactly one primary production arrow;
- at most one alternate production arrow;
- at most three `|` sections in each production.

---

## 6. Entry rule

The parser moves the first exact `Start` or `start` rule to the front. If neither exists, the first parsed rule becomes the entry.

Use this strict form:

```p3d
Start -> Scene
```

The entry rule receives no call arguments, so it must have zero parameters.

Invalid:

```p3d
Start(width) -> S(width 1 1)
```

---

## 7. Rule headers

The canonical header shape is:

```text
RuleName(parameters) repeat-expression reroll-name... ; probability
```

Every part after the name is optional, but its order is fixed.

### 7.1 Parameters

Parameters are whitespace-separated:

```p3d
Tower(width height floors) ->
    Building(width height floors)
```

Calls are also whitespace-separated:

```p3d
Tower(8 20 5)
```

A bare identifier argument that refers to a random variable preserves the variable's sampled value, original range, integer flag, and resampleability. A calculated argument becomes an immutable value in the callee.

```p3d
Start ->
    R Width(3 6)
    Panel(Width)       # preserves Width's random range metadata
    Panel(Width*0.5)   # passes an immutable calculated value

Panel(w) ->
    S(w 1 1)
```

Parameters shadow caller-visible names. A local `R` declaration may not use the same name as a parameter.

### 7.2 Repeat count

A repeat count follows the optional parameter list:

```p3d
Row 8 -> Item
Row(n) n -> Item
Row(n) n+1 -> Item
```

The header parser consumes the repeat expression as one lexical token. Use a compact expression with no spaces or parentheses:

```p3d
n
n+1
2*n
n/2
t*3
```

Do not use function calls or grouped expressions in a repeat header:

```p3d
# Not supported by the current header parser
Row(n) floor(n/2) -> Item
Row(n) (n+1) -> Item
```

Repeat values are converted to `int`, so positive fractional values truncate toward zero. Write integer-valued expressions deliberately.

If the repeat value is zero or negative, the rule emits nothing. Setup and tail sections are not executed.

### 7.3 Header reroll bindings

Identifiers after the repeat token and before `;` or `->` are reroll bindings:

```p3d
Child 1 Height Width -> ...
```

This means:

1. find caller-visible `Height` and `Width`;
2. sample fresh values from their original ranges;
3. bind the fresh values inside this invocation of `Child`.

The first token is still the repeat count. To reroll a value without repeating, write `1` explicitly:

```p3d
Start ->
    R Width(0.6 1.4)
    Panel
    Panel

Panel 1 Width ->
    S(Width 1 0.2)
    I(Cube material(whitepaintmatte) 1 0.5)
```

Each call to `Panel` receives one fresh `Width`.

### 7.4 Probability and alternate production

```p3d
Cap ; 0.72 ->
    PrimaryCap
->
    AlternateCap
```

The primary probability must be a finite literal in `[0,1]`. The alternate probability is `1-p`.

Selection occurs once per rule invocation, before the rule's repeated body is expanded. It does not select a new alternate on every repeat iteration.

A probability without an alternate is accepted but has no branching effect. Avoid that ornamental semicolon.

---

## 8. Body sections and repetition

A production has one, two, or three sections:

| Source form | Section 0 | Section 1 | Section 2 |
|---|---|---|---|
| `body` | empty | repeated body | empty |
| `setup \| body` | setup once | repeated body | empty |
| `setup \| body \| tail` | setup once | repeated body | tail once |

For repeat count `n > 0`:

\[
E(R)=E(S_0)+\sum_{i=0}^{n-1}E(S_1,i)+E(S_2)
\]

The generated counter is:

```text
<RuleName>_count = 0, 1, ..., n-1
```

Example:

```p3d
Start -> Row(7)

Row(n) n ->
    T(-3 0 0)
    |
    [ T(Row_count 0 0)
      S(0.8 1 0.8)
      I(Cube material(warmwhitematteplaster) 1 0.5) ]
```

`Row_count` is created after setup, so do not use it in section 0. It is available in the repeated body and remains visible to the tail with its final value.

A one-section rule is often the cleanest repeated rule:

```p3d
Levels(height floors) floors ->
    [ T(0 Levels_count*(height/floors) 0)
      S(2 height/floors 2)
      I(Cube material(whitepaintmatte) 1 0.5) ]
```

---

## 9. Variables and randomness

### 9.1 Continuous random variable

```p3d
R Height(4 8)
```

`R` samples between the evaluated bounds using the runtime's continuous uniform distribution. Equal bounds produce a deterministic value; do not depend on the upper endpoint being sampled.

Bounds are expressions and are evaluated in order at expansion time:

```p3d
R A(2 3)
R B(A A+1)
```

Declare a variable before its first executed use.

### 9.2 Integer random variable

```p3d
R* Floors(2 6)
```

`R*` samples an integer uniformly from:

\[
\lceil minimum\rceil,\ldots,\lfloor maximum\rfloor
\]

The value is stored numerically as a float but retains an integer flag and integer range metadata.

Use `R*` for repeat counts, discrete choices, and legacy material indices.

### 9.3 Fresh sample expression

```p3d
&W
```

`&name` samples a fresh value from that random variable's original range without replacing the stored value.

```p3d
Start ->
    R Width(0.7 1.3)
    S(&Width 1 1)
```

`&` works only for a resampleable value. A calculated parameter is immutable:

```p3d
Child(Width)      # resampleable when Width is a bare random variable
Child(Width*0.5)  # immutable calculated value
```

### 9.4 Visibility and shadowing

Each rule invocation creates a new variable frame. Lookup searches the current frame and then its callers.

Consequences:

- a called rule can read caller-visible variables;
- a parameter or local declaration shadows an outer value;
- a local declaration disappears when the rule returns;
- sibling rule calls do not inherit each other's local variables.

Prefer explicit parameters for stable, reviewable dependencies. Caller visibility is useful, but it can turn a small edit into a hidden coupling.

---

## 10. Expressions

### 10.1 Operators and precedence

From highest practical binding to lowest:

1. primary values and function calls;
2. exponentiation `^`, right-associative;
3. unary `+` and `-`;
4. multiplication and division;
5. addition and subtraction.

Examples:

```p3d
S(3^2 1 1)      # 9
S(-2^2 1 1)     # -4
S(2^-2 1 1)     # 0.25
```

Division by a value whose magnitude is at most `1e-12` is rejected.

### 10.2 Core functions

| Function | Arity | Meaning and constraints |
|---|---:|---|
| `sin(x)`, `cos(x)`, `tan(x)` | 1 | Trigonometry in radians |
| `radians(d)` | 1 | Degrees to radians |
| `degrees(r)` | 1 | Radians to degrees |
| `abs(x)` | 1 | Absolute value |
| `sqrt(x)` | 1 | Requires `x >= 0` |
| `floor(x)`, `ceil(x)` | 1 | Integer boundary as float |
| `min(a,b)`, `max(a,b)` | 2 | Pairwise minimum or maximum |
| `clamp(x,a,b)` | 3 | Requires `b >= a` |
| `wrap(x,a,b)` | 3 | Wraps into `[a,b)`, requires `b > a` |
| `lerp(a,b,u)` | 3 | `a + (b-a)u`; `u` is not clamped |
| `ease(u)` | 1 | Clamped cubic smoothstep |
| `smoother(u)` | 1 | Clamped quintic smootherstep |
| `accelerate(u[,p])` | 1 or 2 | Clamped ease-in, default `p=2`, requires `p>0` |
| `decelerate(u[,p])` | 1 or 2 | Clamped ease-out, default `p=2`, requires `p>0` |

### 10.3 Time functions

| Function | Result |
|---|---|
| `cycle(time,period[,phase])` | Repeating sawtooth in `[0,1)` |
| `pingpong(time,period[,phase])` | Out-and-back triangle in `[0,1]` |
| `pulse(time,period,duty[,phase])` | `1` during duty fraction, otherwise `0` |
| `swing(time,min,max,period[,phase])` | Triangle motion between bounds |
| `oscillate(time,min,max,period[,phase])` | Sinusoidal motion between bounds |
| `spin(time,rate[,offset])` | `offset + time*rate` |

Periods must be greater than zero. `pulse` duty must lie in `[0,1]`. `swing` and `oscillate` require `maximum >= minimum`.

---

## 11. Conditions

Syntax:

```p3d
?(condition) true-sequence : false-sequence
```

Supported comparisons:

```text
<  >  <=  >=  ==  !=
```

Without a comparison, an arithmetic value is true when its absolute value exceeds `1e-6`.

Equality and inequality use an absolute epsilon of `1e-6`.

Example:

```p3d
Beacon ->
    ?(pulse(t,2,0.25)==1)
        BeaconOn
    :
        BeaconOff
```

### Tail-branch rule

The parser gives the false branch the remainder of the current section or square-bracket block. Therefore a conditional is effectively a tail construct.

This:

```p3d
Rule ->
    ?(x>0) A : B
    Common
```

means:

```text
true  -> A
false -> B Common
```

It does not mean `(A or B) followed by Common`.

Use one of these patterns:

```p3d
Rule ->
    ?(x>0) AWithCommon : BWithCommon
```

or:

```p3d
AWithCommon -> A Common
BWithCommon -> B Common
```

There are no `&&`, `||`, or logical-not operators in the current condition parser.

---

## 12. Scope and transform actions

Square brackets clone the current scope, execute the branch, then restore the parent scope:

```p3d
Start ->
    [ T(-1 0 0) Block ]
    [ T( 1 0 0) Block ]
```

This is the main transform-isolation mechanism.

### 12.1 Transform table

| Action | Arguments | Current effect |
|---|---|---|
| `S(x y z)` | 3 expressions | Scales primary and secondary transforms |
| `T(x y z)` | 3 expressions | Translates primary and secondary transforms |
| `A(angle axis)` | 2 expressions | Rotates in degrees around axis `0=X`, `1=Y`, `2=Z` |
| `A(angle axis low high)` | 4 expressions | Evaluates then clamps angle into `[low,high]` |
| `D(x y z)` | 3 expressions | Scales only the secondary transform |
| `DSX(x y z)` | 3 expressions | Multiplies deformation scale on the minimum X face |
| `DSY(x y z)` | 3 expressions | Multiplies deformation scale on the minimum Y face |
| `DSZ(x y z)` | 3 expressions | Multiplies deformation scale on the minimum Z face |
| `DTX(x y z)` | 3 expressions | Adds deformation translation on the minimum X face |
| `DTY(x y z)` | 3 expressions | Adds deformation translation on the minimum Y face |
| `DTZ(x y z)` | 3 expressions | Adds deformation translation on the minimum Z face |

Transforms are composed in source order and are not reset after instancing. Use `[...]` whenever a part should not leak its transforms into following parts.

### 12.2 Rotation

`A` consumes degrees. Trigonometric functions consume radians.

```p3d
A(45 2)
A(degrees(0.5) 1)
A(oscillate(t,-35,50,4) 2 -35 50)
```

The axis must evaluate exactly to `0`, `1`, or `2`.

### 12.3 Split and face-deformed cubes

`Cube`, `CubeX`, `CubeY`, and `CubeZ` have primary and secondary transform paths. `D` changes the secondary path. The axis variants establish split-cube orientations suited to X, Y, or Z extension.

`DS*` and `DT*` apply local deformation to vertices on the corresponding minimum face. They are useful for tapering, contouring, chaining, and asymmetric section control.

Example:

```p3d
WingSegment ->
    [ S(4 0.35 1.2)
      D(1 1 0.45)
      DSX(1 0.75 0.75)
      DTX(0 0.08 0)
      I(CubeX material(agedwhitepaintmatte) 1 0.5) ]
```

### 12.4 Braces

The scene runtime contains handlers for `{` and `}`, but the current source parser discards both tokens. Balanced braces therefore provide no source-level scope behavior.

Do not write:

```p3d
{ T(1 0 0) Block }
```

Use:

```p3d
[ T(1 0 0) Block ]
```

A standalone `*` is also discarded and should not be used.

---

## 13. Instancing

### 13.1 Current primitive set

The current grammar parser accepts exactly:

```text
Cube
CubeX
CubeY
CubeZ
Cylinder
Sphere
```

Use exact case.

Although the uploaded project includes STL catalog and UI infrastructure, `Grammar.cpp` accepts only the six built-ins and `Mesh::isStlInstanceType` currently returns false. `STL.*` is not part of the executable source grammar in this build.

### 13.2 Instance syntax

```p3d
I(type material-spec [alpha [texture-scale]])
!I(type material-spec [alpha [texture-scale]])
```

Examples:

```p3d
I(Cube material(warmwhitematteplaster) 1 0.5)
!I(Cube material(lightgreyroughconcrete) 1 0.5)
I(Sphere index(MaterialChoice) 1 1)
```

`!I` creates an immovable primitive. Pending linear and rotational velocity are ignored for it.

The parser allows one to three arguments after the primitive type:

1. material or material index;
2. optional alpha field;
3. optional texture scale.

The implementation defaults are:

```text
alpha = 0.0
texture scale = 0.125
```

For cross-path clarity, supply both explicitly.

### 13.3 Explicit material versus explicit index

Use:

```p3d
material(name)
index(expression)
```

These forms are deterministic and self-documenting.

A bare identifier is ambiguous:

```p3d
I(Cube FloorTex 1 1)
```

- if `FloorTex` is visible as a grammar value, it becomes a legacy material index;
- otherwise it becomes the literal material name `FloorTex`.

A later variable declaration or call-path change can therefore change the meaning of old source. Prefer:

```p3d
I(Cube index(FloorTex) 1 1)
I(Cube material(FloorTex) 1 1)
```

### 13.4 Legacy material indices

| Index | Material |
|---:|---|
| 0 | `warmwhitematteplaster` |
| 1 | `pastelbluemattepaint` |
| 2 | `lightgreyroughconcrete` |
| 3 | `softbeigemattesandstone` |
| 4 | `darkslatesmoothstone` |
| 5 | `bluewhiteglossysmoothglass` |
| 6 | `amberwarmwood` |
| 7 | `steelshinyroughmetal` |
| 8 | `creamtiledceramic` |
| 9 | `terracottaroughceramic` |
| 10 | `sagegreenmattepaint` |
| 11 | `lightoakwood` |
| 12 | `offwhiteceramic` |
| 13 | `silverbluepolishedmetal` |
| 14 | `charcoalroughmetal` |
| 15 | `softpinkmatteplaster` |
| 16 | `mintglossysmoothtile` |
| 17 | `smokytransparentglass` |
| 18 | `bluewhiteshinysmoothmetal` |

Other integer indices map to `materialN`.

---

## 14. Material descriptor language

Procedural material names are compact descriptor strings. The editor scans recognised lexemes and recommends this order:

```text
color [color2] [color3] [opacity] family [finish...]
```

Example:

```p3d
material(silvermetalpolishedreflective)
material(cyantransparentglassglossy)
material(amberwoodagedrough)
material(cyanneonglowingactive)
```

The current editor lexemes are:

| Group | Recognised words |
|---|---|
| Colors | `orange purple silver yellow magenta lime violet white green black chrome copper beige brown blue grey gray gold amber teal cyan pink red` |
| Opacity | `transparent translucent opaque smoky` |
| Families | `concrete ceramic plastic plaster grass brick neon floral boards glass marble stone metal paint wood rock tile` |
| Finishes and species | `emissive glowing active animated rotating reflective weathered polished mirror twotone duotone horizontal vertical veins veined oak walnut pine cedar birch smooth glossy shiny rough aged matte satin dull` |

Up to three distinct colors are collected.

Material names are canonicalised to lowercase alphanumeric text. `usertextureN` is preserved as a special texture slot. Empty material text becomes `softwhitematteplaster`. If no recognised texture suffix is found by the context, `plaster` is appended.

Use compact no-space names. Quoting is supported only as a light wrapper around a single token, not as a general free-form string system.

---

## 15. Physics actions

Physics setters are pending state for the next `I` or `!I`. They are cleared after every instancing attempt.

| Action | Effect |
|---|---|
| `V(vx vy vz)` | Initial linear velocity for the next movable primitive |
| `VR(rx ry rz)` | Initial rotational velocity for the next movable primitive |
| `M(mass)` | Mass for the next movable primitive |
| `P(density)` | Density-derived mass for the next cube-like primitive |
| `G(magnitude dx dy dz)` | Global gravity, persistent until another `G` |

Example:

```p3d
Start ->
    G(9.81 0 -1 0)

    [ T(0 -0.5 0)
      S(12 0.5 12)
      !I(Cube material(lightgreyroughconcrete) 1 0.5) ]

    [ T(0 4 0)
      V(1 0 0)
      VR(0 45 0)
      M(2)
      S(1 1 1)
      I(Cube material(redmetalglossy) 1 0.5) ]
```

Notes:

- gravity direction is normalised before applying magnitude;
- zero direction or zero magnitude disables gravity;
- later `G` calls replace earlier gravity;
- mass is clamped to at least `0.001`;
- density is clamped to nonnegative;
- density overrides pending `M` only for `Cube`, `CubeX`, `CubeY`, and `CubeZ`;
- rotational velocity magnitude is capped at `720` degrees per second;
- immovable instances receive zero velocity and default mass.

---

## 16. Time-variable grammars

`t` is an immutable evaluation input measured in seconds. It is supplied externally for each grammar rebuild.

```p3d
Rotor ->
    [ A(wrap(spin(t,90),-180,180) 1 -180 180)
      S(0.25 3 0.25)
      I(Cube material(silvermetalpolished) 1 0.5) ]
```

The editor reseeds the grammar generator from the source text and a design nonce. Time-sampled rebuilds reuse that nonce, so `R`, `R*`, and probabilistic production choices remain stable while only `t` changes.

This produces a useful model:

\[
Scene = F(source,\ designNonce,\ t)
\]

A new ordinary Run changes the design nonce. Timeline samples change only `t`.

### Bounded motion pattern

Use a bounded function and an `A` safety envelope together:

```p3d
A(oscillate(t,-30,45,4) 2 -30 45)
```

For continuous spin, wrap before applying:

```p3d
A(wrap(spin(t,90),-180,180) 1 -180 180)
```

A time conditional may change topology:

```p3d
Beacon ->
    ?(pulse(t,2,0.25)==1) BeaconOn : BeaconOff
```

Topology-changing time conditions are valid, but they rebuild different action streams at threshold crossings.

---

## 17. Recursive grammar design

Recursive calls are legal, but termination is your responsibility.

Safe pattern:

```p3d
Start -> Branch(7 2.0)

Branch(depth length) ->
    ?(depth>0)
        [ S(length 0.15 0.15)
          I(CubeX material(amberwoodaged) 1 0.5)
          T(length 0 0)
          [ A( 25 2) Branch(depth-1 length*0.72) ]
          [ A(-25 2) Branch(depth-1 length*0.72) ] ]
    :
        Tip

Tip ->
    S(0.2 0.2 0.2)
    I(Sphere material(greenmattepaint) 1 0.5)
```

The decreasing `depth` parameter is the proof of termination.

### 17.1 Static growth model

For a selected production with setup `S0`, repeated body `S1`, tail `S2`, and repeat `n`:

\[
C(R)=C(S_0)+nC(S_1)+C(S_2)
\]

Use a cost vector:

\[
C=(actions,\ primitives,\ ruleInvocations,\ work)
\]

For an alternate rule with primary probability `p`:

\[
E[C]=pC_{primary}+(1-p)C_{alternate}
\]

For safety, use the componentwise worst case:

\[
C_{max}=\max(C_{primary},C_{alternate})
\]

For a binary recursive grammar with one primitive per node:

\[
P(0)=1,\qquad P(d)=1+2P(d-1)=2^{d+1}-1
\]

At depth `14`, this is `32,767` primitives. At depth `15`, it is `65,535`, which exceeds the current `50,000` primitive ceiling.

Estimate worst-case growth before increasing repeat or recursion dimensions. The geometry may look innocent while the expansion tree is quietly inflating a balloon behind the curtain. 🎈

---

## 18. Runtime safety ceilings

The current grammar expansion limits are:

| Limit | Value |
|---|---:|
| Recursion depth | 256 |
| Rule invocations | 250,000 |
| Expansion work units | 1,000,000 |
| Emitted actions | 250,000 |
| Generated primitives | 50,000 |
| Absolute repeat count | 100,000 |

Exceeding a limit aborts the expansion. Partial action streams and runtime variable snapshots are not published.

These are hard emergency ceilings, not design targets.

---

## 19. Recommended strict writing profile

Use these rules for source that remains readable and resistant to parser ambiguity:

1. Define an explicit zero-parameter `Start` rule.
2. Use one logical rule header per line.
3. Indent continuation lines.
4. Use exact built-in primitive case.
5. Use `material(name)` or `index(expression)`.
6. Supply both instance trailing arguments, commonly `1 0.5` or `1 1`.
7. Separate action and rule-call arguments with whitespace.
8. Use commas only inside function calls.
9. Keep repeat expressions compact and parenthesis-free.
10. Put each conditional at the end of its section or `[...]` block.
11. Use `[...]` for transform isolation.
12. Do not use `{...}` or standalone `*`.
13. Declare random variables before executed use.
14. Pass important dependencies as parameters.
15. Use `R*` for discrete values.
16. Use `<RuleName>_count` only in repeated body or tail.
17. Bound time-driven angles and recursive dimensions.
18. Estimate worst-case action and primitive growth.

---

## 20. Common failures

| Source | Problem | Correction |
|---|---|---|
| `S(1,2,3)` | Action commas are not argument separators | `S(1 2 3)` |
| `Child(1,2)` | Rule-call commas are not separators | `Child(1 2)` |
| `lerp(0 1 t)` | Function arguments require commas | `lerp(0,1,t)` |
| `A(30 3)` | Axis must be `0`, `1`, or `2` | `A(30 2)` |
| `Start(x) -> ...` | Entry rule receives no arguments | Move parameters to a called rule |
| `R t(0 1)` | `t` is reserved | Choose another identifier |
| `Row(n) floor(n/2) -> ...` | Repeat header accepts one compact token | Precompute/pass a value or use `n/2` |
| `{ T(1 0 0) Part }` | Braces are discarded | `[ T(1 0 0) Part ]` |
| `I(STL.part material(x) 1 1)` | STL instancing is disabled in this build | Use one of the six built-ins |
| `I(Cube FloorTex 1 1)` | Bare name may change between material and index | Use `material(FloorTex)` or `index(FloorTex)` |
| `?(x>0) A : B Common` | `Common` belongs only to false branch | Call a common helper from both branches |
| `S(H 1 1) R H(2 2)` | Runtime declaration order is wrong | Move `R H(...)` before the use |

---

## 21. Compact feature examples

### Random parameter

```p3d
Start ->
    R Height(2 5)
    Tower(Height)

Tower(h) ->
    S(1 h 1)
    I(Cube material(whitepaintmatte) 1 0.5)
```

### Integer repeat and counter

```p3d
Start -> Stack(6)

Stack(n) n ->
    [ T(0 Stack_count 0)
      S(1 0.9 1)
      I(Cube material(lightgreyroughconcrete) 1 0.5) ]
```

### Per-call header reroll

```p3d
Start ->
    R Width(0.6 1.2)
    Panel
    T(1.5 0 0)
    Panel

Panel 1 Width ->
    S(Width 1 0.2)
    I(Cube material(bluepaintmatte) 1 0.5)
```

### Per-use reroll

```p3d
Start ->
    R Width(0.6 1.2)
    S(&Width 1 0.2)
    I(Cube material(bluepaintmatte) 1 0.5)
```

### Alternate geometry

```p3d
Cap ; 0.8 ->
    I(Cube material(whiteceramicglossy) 1 0.5)
->
    I(Sphere material(whiteceramicglossy) 1 0.5)
```

### Time animation

```p3d
Start ->
    [ T(0 oscillate(t,0,3,4) 0)
      A(swing(t,-25,25,3) 2 -25 25)
      I(Cube material(cyanneonglowing) 1 0.5) ]
```

---

## 22. Conformance checklist

A grammar is ready for a parser test when all answers are yes:

- Is every delimiter balanced within its logical rule?
- Is every rule name unique?
- Does `Start` have zero parameters?
- Does every call supply exactly the target parameter count?
- Are all action arities exact?
- Are all variables visible and declared before the executed use?
- Are all function names supported and arities correct?
- Are function arguments comma-separated?
- Are action and rule arguments whitespace-separated?
- Is every random range finite and ordered?
- Does every `R*` range contain at least one integer?
- Are rotation axes exact integers in `{0,1,2}`?
- Are rotation bounds ordered?
- Is every material/index choice explicit?
- Does every conditional intentionally own the rest of its sequence?
- Does every recursive cycle have a decreasing termination measure?
- Is worst-case expansion below all safety ceilings?

---

## 23. Current implementation boundaries

The following features are present in surrounding source or UI scaffolding but are not executable source-grammar features in this snapshot:

- STL instancing;
- semantic brace scopes;
- comma-separated action arguments;
- general boolean expressions;
- assignments;
- named source scopes;
- arbitrary string literals;
- more than one alternate;
- more than three body sections;
- function calls in rule-header repeat expressions.

These are good candidates for a future grammar revision, but they should not be inferred into current source.

---

## 24. Files delivered with this guide

- `Progen3D_Grammar.ebnf`: standalone strict EBNF.
- `Progen3D_Grammar_Writing_Guide.md`: this guide.
- `Progen3D_Grammar_Showcase.p3d`: a single current-syntax example combining sections, counters, parameters, random values, alternate productions, materials, physics, deformation, and time.
