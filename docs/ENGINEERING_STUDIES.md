# ProGen3D engineering studies

The engineering study tools combine native geometry bindings with a unit-checked scalar grammar, reproducible time traces and reference thermal, hydraulic and circuit solvers. Run the examples from the repository root with Python 3.10 or later. RK4 and catalogue search use the standard library. BDF, RK45 and the sparse thermal and circuit solvers use NumPy and SciPy. The desktop authoring window uses Tk. The optional local FMI adapter was checked with FMPy 0.3.32.

## Run the bound coupon

```bash
make progen3d-scene-manifest
./progen3d-scene-manifest examples/EngineeringStudies/coupon.p3d /tmp/coupon.scene.json
python3 -m tools.engineering check examples/EngineeringStudies/coupon.p3physics --scene /tmp/coupon.scene.json
python3 -m tools.engineering run examples/EngineeringStudies/coupon.p3physics --scene /tmp/coupon.scene.json --output /tmp/coupon.trace.json
python3 -m tools.engineering view /tmp/coupon.trace.json --output /tmp/coupon.html
```

A reproducible native manifest is included as `examples/EngineeringStudies/coupon.scene.json`. This makes the example runnable without a C++ build. The coupon solves a lumped model using authored heat capacity and conductance. Its geometry binding identifies the body whose temperature is reported.

## Bind named parts and cube faces

Use the existing `Object(id(...) class(...) taxonomy(...))` grammar to name a part. In the bonded coupon, `CouponLeft` and `CouponRight` each directly own one `Cube` inside the assembly object. A sidecar can bind the left part and its mating face with `bind substrate = object("CouponLeft") face "+X"`.

The manifest records the named part's primitive type, transform and source span when it owns exactly one emitted primitive. The names `-X`, `+X`, `-Y`, `+Y`, `-Z` and `+Z` identify faces in an affine cube's local coordinates. Rotation and reflection can change where a face is in the world while preserving its name. A compound object needs a named child for an individual face. Deformed cubes and other geometries can still have object IDs. They currently have no stable named cube faces.

Author-defined IDs survive insertion or resequencing of other primitives. After a geometry edit, generate a fresh manifest and check the bound part and face before updating a pinned revision. `primitive:N` IDs and their `role:object-part:boundary` mesh groups remain valid within their exported revision. The optional `revision "…"` and `face "…"` selectors check the snapshot and requested surface. The sidecar fingerprints its source and manifest for result replay.

In the desktop editor, open `bonded-coupon.p3d` and choose **Inspector**. Select `CouponLeft` in the **Object** list and **+X** in the **Face** list. You can also click a visible face in the preview. An exact edge click selects the part without choosing one of its adjoining faces. The Inspector shows `CouponLeft:+X`, and the preview outlines the selected face when visible. **Reveal source** opens the editor and selects the complete emitting `I(...)` instruction, including any continuation lines.

Rebuilds and timeline samples look up named parts again by ID. An unrelated insertion can move a selected part to another primitive index while its named selection and face remain selected. A split part retains the spatial object selection and clears the face. A missing ID clears the selection. When the preview still shows a previous successful build, **Reveal source** becomes available again once the current text has built successfully.

Run the desktop acceptance check using the bundled coupon fixture.

```bash
./progen3d-editor-gui --engineering-selection-smoke-test \
  --open examples/EngineeringStudies/bonded-coupon.p3d
```

The check exercises the rendered text editor. It reveals the instruction for `CouponLeft:+X`, types an invalid dimension and checks that the build retains the last valid scene. It then corrects the transverse dimension from 20 mm to 24 mm, rebuilds and checks that the same face still reveals its source. The test edits only the in-memory document. The successful run prints `PROGEN3D_GUI_ENGINEERING_SELECTION_PASSED` with the face and observed dimension.

## Author, observe and replay

- Start with `coupon.p3physics`. Each declaration occupies a line. Units follow a number, such as `4 [W/K]`. Expressions support scalar literals, earlier parameters, states, derived quantities, arithmetic and the solver time `t`. Declare derived quantities in dependency order. The left side of each differential equation is `der(state)` or a constant parameter expression multiplied by that derivative. Each state needs one equation.
- Use `observe T every 0.5 [s] display [degC]` to display temperature on a half-second grid. Output sampling follows accepted solver steps through dense interpolation. Select RK4 for fixed steps, RK45 for an adaptive explicit method, or BDF for stiff scalar systems. A supplied `step` caps the adaptive step or sets the RK4 step. The example tightens the tolerance to check its threshold time to one microsecond.
- Use `event name at 1 [s] { set x = 2 [m] }` for a scheduled reset or `event name when falling(T - 40 [degC]) { emit("threshold") }` to record a root. Analogous guards support rising or either direction. Simultaneous events at an identical time apply in name order. Multiple root crossings inside one accepted step need a suitable step cap. The result retains pre- and post-event states.
- Use `run … --save-checkpoint 32.25 /tmp/checkpoint.json` and `run … --checkpoint /tmp/checkpoint.json` for deterministic replay from the initial conditions and state verification at the checkpoint. The checkpoint stores an observation and input fingerprint. Replaying verifies the observation in a newly constructed solver.
- Use `python3 -m tools.engineering quantities torque` for source identifiers. Compound units support products, quotients and integer or rational powers such as `m^(1/2)`. Temperature points and intervals have separate arithmetic. The unit vocabulary contains the seven SI base units and 22 specially named derived units, selected prefixes and affine Celsius and Fahrenheit. Catalogue names describe quantities. Executable units and backend capabilities have separate interfaces.
- Run `python3 -m tools.engineering ui` to choose and edit a sidecar, search quantity definitions, execute a study, save its trace and open an offline comparison plot. The desktop and CLI call the same compiler and solver.
- The coating example defines a project quantity through `--kinds examples/EngineeringStudies/project-kinds.json`. It evolves a scalar thickness with an authored growth rate. This is an example of registering and plotting an additional kind without adding a keyword.

## Use the reference domain models

```bash
python3 -m tools.engineering capabilities
python3 -m tools.engineering analyse examples/EngineeringStudies/circuit.json
python3 -m tools.engineering analyse examples/EngineeringStudies/coolant.json
python3 -m tools.engineering analyse examples/EngineeringStudies/split-coolant.json
```

Domain JSON inputs use explicit SI suffixes. The implemented providers cover a lumped heat network with conductances and reservoir ports, a cell-centred 1-D conduction model, linear axial expansion and parallel bonded members, fully developed laminar flow in a circular pipe, and linear resistor networks with one capacitive dynamic node. The circuit solves its algebraic variables using sparse elimination and checks any declared initial voltages. The area-overlap transfer maps flux densities between strips over a shared surface and reports source and target power. Split wall/coolant exchange subcycles the fluid, iterates the interface heat rate, retries nonconvergent macrosteps and maintains a heat ledger. The example starts with a deliberately oversized macrostep to exercise rejection.

The `cohesive-opening.json` example supplies a mode-I traction–separation law with an irreversible maximum-opening state and a required parameter-source label. Its values are synthetic. It computes damage from a prescribed opening history, with tensile unloading and complete failure. A coupled fracture prediction requires measured material data and a structural model that determines the opening.

## Run a local FMI model

Install FMPy. Select FMU binaries you trust as you would any executable module. The adapter supports scalar observations from FMI 3 Model Exchange and Co-Simulation and checks declared state capability on request.

```bash
python3 -m tools.engineering fmi Dahlquist.fmu --mode ModelExchange --stop 1 --interval .1 --observe x
python3 -m tools.engineering fmi Dahlquist.fmu --mode CoSimulation --stop 1 --interval .1 --observe x --require-rollback
python3 -m tools.engineering fmi-schedule Clocks.fmu examples/EngineeringStudies/fmi-clocks.json --observe inClock1Ticks --observe inClock2Ticks
```

Scheduled Execution serialises the explicit input clock sequence, including source order for simultaneous activations. The reference FMUs and model semantics come from the [Modelica Association Reference FMUs v0.0.41](https://github.com/modelica/Reference-FMUs/releases/tag/v0.0.41) and the [FMI 3.0.2 specification](https://fmi-standard.org/docs/3.0.2/).

## Verification and scope

```bash
python3 -m unittest discover -s tests -p 'test_engineering_studies.py' -v
tests/run_engineering_native_checks.sh
```

The reference tests cover legacy gravity, velocity and angular velocity, explicit and density-derived mass, fixed-seed sampling, rejection of open surfaces, affine units, dimensional mismatches, stale geometry bindings, fixed-step refinement, event resets, checkpoint replay, conservation, conduction convergence, axial expansion, circuit consistency, hydraulic pressure drop and custom quantity registration. The separate optional FMI checks use the published Dahlquist and Clocks binaries.

The implemented grammar solves explicit scalar ODEs. A sparse resistor model supplies a specific index-one circuit case. The broader work is general tensor and field expressions, nonlinear DAE/PDE assembly, calibrated mixed-mode contact and fracture, external multi-FMU rollback coordination, FMI clock discovery, and docking in the existing native editor. The examples describe bounded reference problems and provide fixtures for expanding those capabilities.

## Couple heat, expansion and damage in a bonded coupon

```bash
make progen3d-scene-manifest
./progen3d-scene-manifest examples/EngineeringStudies/bonded-coupon.p3d /tmp/bonded.scene.json
python3 -m tools.engineering analyse examples/EngineeringStudies/bonded-coupon.json \
  --scene /tmp/bonded.scene.json --output /tmp/bonded.json
python3 -m tools.engineering view /tmp/bonded.json --output /tmp/bonded.html
```

The included `bonded-coupon.scene.json` reproduces the native binding. It contains two adjoining axis-aligned cube bodies on the x axis. A geometry-bound study must declare the exact manifest revision in `geometry.revision`. In the example, `layer_objects` names the two parts and `layer_faces` names their contacting local faces in layer order. The solver checks that the specified faces adjoin, including sign changes from reflection. Older studies may omit `layer_faces` to infer the faces from the layer order and axis. The geometry check also compares body lengths, common cross-section and spacing to the study declaration.

The example is a synthetic two-layer end-bonded stack with a common cross-section. Piecewise-linear temperature programs act at the outer faces. Heat flows through each layer and through a specified area-specific bond resistance. For the joint between adjacent cells, use the sum of both half-cell resistances and the bond resistance. The thermal ledger integrates the heat entering each end and checks that against the change in thermal energy stored in both layers.

The model integrates stress-free thermal expansion along the axis. Grip displacement minus thermal expansion gives the displacement available to elastic compliance and bond opening. Tension follows a bilinear cohesive law with irreversible maximum-opening history. Compression closes the interface with a separate stiffness. The solver rejects a path that enters an unstable softening regime where displacement control cannot trace a unique equilibrium. Temperature drives mechanics in this model. The thermal interface resistance is fixed and the heat ledger omits mechanical dissipation. A separate physical model is needed for post-separation heat transfer, bending, shear and mixed-mode fracture.

With a `convergence` block, each level doubles the cells per layer and halves the maximum heat step. A level passes when the event set matches the preceding level and the changes in onset and separation times, peak opening and final damage fall within the specified limits. Read the returned `converged` flag before using a trace. Damage extrema are tracked from accepted heat steps and interpolated loading extrema. They do not depend on the plot sampling interval. The reported damage is the fractional reduction of tensile unloading stiffness in this law.

Fit a local monotonic force–opening envelope:

```bash
python3 -m tools.engineering fit-cohesive examples/EngineeringStudies/cohesive-fit-synthetic.csv \
  --area 0.0004 --source 'Synthetic fit fixture' --output /tmp/cohesive-fit.json
```

Use the fitted stiffness, peak traction, fracture energy and source to populate `bond` in a model for the corresponding specimen. The CSV uses local bond opening in metres and tensile force in newtons. Grip motion also includes adherend and fixture compliance. The two displacements are distinct measurements. Provide enough loading-envelope points on both sides of the peak. A reported failure opening beyond the last sample is an extrapolation of the bilinear model.

Compare a saved trace against an independent thermal-cycle record:

```bash
python3 -m tools.engineering compare-measurements /tmp/bonded.json measured.csv \
  --source 'Coupon and instrument identifiers' --output /tmp/residuals.json
```

Use `coupon-measurements-template.csv` for the column names. Set each `channel` to its corresponding trace quantity and share a time origin. Report each unit explicitly and optionally provide standard uncertainty in that same unit. The comparison converts units, interpolates the recorded trace at measurement times and reports residuals and RMSE per channel. The comparison rejects times outside the trace. Keep measured point locations consistent with the chosen observation. Layer averages and surface measurements describe different quantities.

```bash
python3 -m unittest discover -s tests -p 'test_bonded_coupon.py' -v
```

The example uses synthetic material values and a synthetic loading program. Its analytic and refinement checks verify the implementation. Validation for a physical coupon requires separate material and interface measurements and a thermal-cycle dataset.
