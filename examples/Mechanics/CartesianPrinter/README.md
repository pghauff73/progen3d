# Cartesian printer under load

Open [CartesianPrinter.p3d](CartesianPrinter.p3d) with the desktop runtime from this branch and advance the preview from 0 to 28.5 seconds. The loaded printer deposits three 56 × 40 mm perimeters, raises the nozzle, parks at 26.5 s and spends two seconds relaxing pressure. To compare with the original kinematic mechanism, change the start production to `Start -> IdealPrinter`.

The frame, two belt drives, two Z screws, six guide shafts and twelve linear bearings are shared geometry. The loaded assembly adds an 18-tooth input pinion and a 36-tooth gear fixed to the filament hob. The 28 interface connections include every bore-to-journal association, each screw and nut, the spur mesh and the hob coupling. Printer X maps to scene X and printer Z to scene Y. Positive printer Y moves the bed in positive scene Z. All deposited segments are built in bed coordinates.

## Results for the supplied assumptions

| Quantity | Simulated value |
| --- | ---: |
| Commanded filament travel | 24.6271 mm |
| Hob surface travel after gear clearance and pitch error | 24.6164 mm |
| Filament entering the hotend | 23.4476 mm |
| Hob slip relative to the filament | 1.1688 mm |
| Supported deposit | 55.0989 mm³ |
| Ooze during travel, layer changes and dwell | 1.2991 mm³ |
| Peak gauge melt pressure | 4.5275 MPa |
| Peak XY tracking error sampled at 16 Hz | 0.1033 mm |
| Belt tooth jumps in this run | 0 |

A 1.75 mm filament has a cross-section of 2.4053 mm². At the end of the dwell, its accepted length accounts for 56.3980 mm³ of material. More than 97% of that volume lands during commanded deposition. The model also accounts for the small volume still stored under pressure. These are demonstration parameters. No values were fitted to a particular printer.

## Mechanical and material response

| Object | Constitutive rule | Effect on the print |
| --- | --- | --- |
| Timing belt and pulley | For tangential demand F, installed tension T₀ and single-span stiffness k, the two tensions are T₀ ± F/2 while both spans remain taut. Beyond that point the unloaded span carries zero tension and the other carries the full load. Belt displacement follows the matching piecewise spring law. | Elastic registration error follows load. In the optional overload regime a slack span and a crossed empirical force threshold produce one pitch of persistent phase loss per loading excursion. The model rearms below 80% of the threshold or when both strands carry tension. |
| Belt teeth, shaft and mount | Effective tooth contact and mount shear give signed displacements F/k. For a circular motor shaft, twist at the pulley rim is F r² L/(GJ), where J = πd⁴/32. | All four deflections, including belt stretch, affect actual carriage position. The motor pulley follows its command. The belt and return pulley follow registered strand travel. |
| Screw nut and transmission play | A stateful play operator holds the follower while the driver crosses the gap. The initial state is centred in the gap. | A direction reversal exposes the full lost motion. Z nut clearance is shared by the synchronised screws in this example. |
| Spur tooth tolerances | Pitch-circle clearance is the sum of the two tooth-thinning allowances plus 2 Δa tan(α) for small centre-distance offsets. A deterministic periodic transmission error is sampled at tooth mesh phase. The gear flank applies the same play operator. | The pinion follows the input. The output gear and hob respond to the actual mesh. Tooth thinning changes rendered tooth widths, and centre distance changes the locations of the gear centres. The rendered flanks are trapezoidal approximations. |
| Guide shafts | The carriage load is shared equally by two simply supported circular guides. For load coordinate a and b = L − a, the deflection at the load is P a² b²/(3 E I L), using the force on one shaft. | Deflection varies with X. The X journals use a curved centreline and the head follows the sag. |
| Heated stack | Uniform free thermal expansion is α L ΔT relative to a reference height. | The nozzle extends downward relative to its carriage. The example supplies a constant temperature difference. |
| Hob and hotend | The available drive force is μN and the corresponding pressure ceiling is μN divided by incoming filament area. Below the ceiling, C dP/dt = Qᵢₙ − P/R. The solver integrates a constant inlet rate exactly over each substep and splits the step at a traction-limit crossing. Rejected feed accumulates as hob slip. | The spool follows accepted stock. Outlet flow follows pressure and can continue after feed stops. Outflow is allocated to deposited material or travel ooze according to the active move. |

The demand on each horizontal axis uses commanded acceleration and a smoothed bearing drag. This is a quasi-static, lumped-parameter model. It resolves drive compliance, bending and thermal growth at part level. Contact stress fields, vibration modes, tooth fracture, permanent plastic set, non-Newtonian melt rheology and bead adhesion require additional material and boundary data. The overload law is a demonstrator for a measured skip threshold. A geared feed advance is assumed, with nonnegative commanded extrusion. Retraction and reverse flow would require a signed material-transport law.

The deposited mesh is reconstructed in 0.125 s slices from the outlet volume and the loaded nozzle path. During print moves it uses a faceted, rounded cross-section with width derived from volume per unit length. Travel produces thin traces or drops. Small residual volumes are kept in the numerical balance even below the mesh rendering threshold. Coincident drops and corner joins can overlap. Read `fffDeposit`, `fffOoze` and `fffStored` for material accounting.

## Author a printer program

The native parser recognises one printer program per document. Each `@fff` record occupies one line. All component records and a home position are required. Add one or more moves. Blank lines and trailing `#` comments are supported. A rebuild starts from the declared home and an unpressurised chamber. Seeking backwards reproduces the same state. Samples outside the program duration hold the endpoint state. Add a dwell move to simulate further discharge.

| Record | Fields in order |
| --- | --- |
| `beltX`, `beltY` | Moving mass (kg), drag (N), installed tension per span (N), one-span axial stiffness, tooth contact stiffness, mount shear stiffness (all N/mm), shaft radius and length (mm), shaft shear modulus (MPa), reversal gap (mm), ratchet threshold (N), belt pitch (mm), pulley teeth |
| `gear` | Module (mm), pinion teeth, wheel teeth, pressure angle (degrees), pinion and wheel tooth-thinning allowances (mm each), signed centre-distance error (mm), pitch-line transmission-error amplitude (mm), hob working radius (mm) |
| `guide` | Support span (mm), diameter (mm), Young modulus (MPa), shared carriage mass (kg). The example frame and journals use a nominal 240 mm span and 8 mm shafts. |
| `thermal` | Effective hot length (mm), expansion coefficient (per kelvin), signed temperature rise (K) |
| `nut` | Total axial reversal gap (mm) |
| `hotend` | Filament diameter (mm), effective compressive compliance C (mm³/MPa), flow resistance R (MPa·s/mm³), normal nip force (N), traction coefficient |
| `bead` | Nominal supported width and layer height (mm). Nominal area is h(w − h) + πh²/4. |
| `home` | Initial X, Y, Z in bed coordinates (mm) |
| `move` | Duration (s), target X, Y, Z (mm), nonnegative extrusion multiplier. Zero selects travel or dwell. One requests nominal bead volume along the move. |

The expressions take a time argument in seconds. For example, `fffPressure(t)` reads MPa, `fffX(t)-fffCommandX(t)` gives signed X error, and `fffDeposit(t)-fffDeposit(a)` gives supported outflow since time a. The solver uses at most 5 ms per step and lands exactly on each move boundary.

| Channels, each prefixed with `fff` | Units |
| --- | --- |
| X, Y, Z, CommandX, CommandY, CommandZ, Lift, Sag, Expansion | mm |
| XStrand, XStretch, XTooth, XShear, XTwist and Y equivalents | mm |
| XTight, XSlack, YTight, YSlack | N |
| XJumps, YJumps | Cumulative event count |
| CommandFeed, HobFeed, Feed, Slip | mm of stock or equivalent surface travel |
| Pressure, Flow | MPa, mm³/s |
| Deposit, Ooze, Stored | mm³ |

To isolate a low-grip feeder, reduce the nip force in `@fff hotend` while holding the motion program fixed. Slip increases when pressure reaches the lower cap. To exercise belt ratcheting in the regression, the test uses a 1 kg carriage, 4 N of installed tension and a 10 N jump threshold. A 12 N demand unloads one span and loses one tooth. Holding this demand causes one event, and reversing it can cause another. For measured inputs, belt span stiffness, direction-reversal travel, tooth fit, filament advance under load, and pressure/flow response constrain different terms in the model. A measured pressure-relaxation time alone constrains the product RC.

## Reproduce the run

```sh
make -j2 build/form-study-sample build/printer-response-csv printer-response-check
python3 tests/check_cartesian_printer.py
python3 tests/check_loaded_printer.py
build/printer-response-csv examples/Mechanics/CartesianPrinter/CartesianPrinter.p3d > response.csv
```

The C++ checks cover reversible play, gear fit, belt tension and re-engagement, shear, torsion scaling, elastic bending, thermal growth, slip, pressure decay, material conservation and timestep refinement. The native geometry checks cover the original 25 reference poses and the loaded mesh at print, reversal, travel and dwell times. The CSV exports the same solver channels read by the grammar.

The engineering basis for the belt is the distinction between installed span tension, effective force and elastic registration error in the [Gates precision drive manual](https://www.gates.com/content/dam/documents-library/catalogs/light-power-and-precision-manual.pdf) and [Gates belt sizing guide](https://www.gates.com/content/dam/documents-library/installation-guides/BeltSizingGuide.pdf). The gear relation uses the small-change centre-distance approximation and tooth-thickness allowances described in [Elements of Metric Gear Technology](https://qtcgears.com/materials/catalogs/Tech.pdf). The nozzle law uses the linear compression and flow assumptions described in [Klipper’s kinematics notes](https://www.klipper3d.org/Kinematics.html#pressure-advance). Here those assumptions predict the uncompensated plant response.
