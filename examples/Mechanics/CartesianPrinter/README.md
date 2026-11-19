# Cartesian printer under load

Open [CartesianPrinter.p3d](CartesianPrinter.p3d) with the desktop runtime from this branch and advance the preview from 0 to 28.5 seconds. The loaded printer deposits three 56 × 40 mm perimeters, raises the nozzle, parks at 26.5 s and spends two seconds relaxing pressure. To compare with the original kinematic mechanism, change the start production to `Start -> IdealPrinter`.

The frame, two belt drives, two Z screws, six guide shafts and twelve linear bearings are shared geometry. The loaded assembly adds an 18-tooth input pinion and a 36-tooth gear fixed to the filament hob. The 28 interface connections include every bore-to-journal association, each screw and nut, the spur mesh and the hob coupling. Printer X maps to scene X and printer Z to scene Y. Positive printer Y moves the bed in positive scene Z. All deposited segments are built in bed coordinates.

## Flexible filament on the same toolpath

The default grammar threads a 1.75 mm flexible strand through the feeder. The exposed 41 mm span is an XPBD wire with two pinned endpoints. The 156 mm of confined stock between the hob and hotend stores volume under compressive load. Both regions read the same instantaneous filament modulus. Use grammar time for extrusion and motion. At a selected pose, Step Physics advances the free wire under gravity while the prescribed machine parts keep their positions.

| Quantity over 28.5 s | Previous stock and belts | Flexible stock and belts |
| --- | ---: | ---: |
| Commanded filament travel (mm) | 24.6271 | 24.6271 |
| Hob surface travel (mm) | 24.6164 | 24.6164 |
| Stock accepted at the hob (mm) | 23.4476 | 24.6164 |
| Stock delivered to the melt chamber (mm) | 23.4476 | 22.4710 |
| Hob slip (mm) | 1.1688 | 0.0000 |
| Peak gauge pressure (MPa) | 4.5275 | 3.6347 |
| Peak confined-stock shortening (mm) | 0 | 5.3363 |
| Deposited on print moves (mm³) | 55.0989 | 43.8718 |
| Discharged on travel and dwell (mm³) | 1.2991 | 9.8997 |
| Volume still stored at 28.5 s (mm³) | <0.0001 | 5.4379 |
| Peak XY tracking error at 16 Hz (mm) | 0.1033 | 0.1048 |

Both rows of assumptions use the same toolpath, nozzle resistance and grip. Omitting the two optional material records reproduces the previous pressure and tracking model. The flexible run accepts more stock at the hob, then releases some of it after the nozzle leaves the perimeter. At 28.5 s, 5.1603 mm³ remains in compressed stock and 0.2776 mm³ in the melt chamber. A longer dwell releases more of that volume. These values illustrate assumed moduli and effective path length. No measured TPU grade or printer was fitted.

### Confined stock and melt

Let A be stock area, L the guided length and P the melt gauge pressure. The feed force is AP. A standard linear solid in creep form has instantaneous compliance Ci = AL/E₀ and additional recoverable compliance Cd = AL(1/E∞ − 1/E₀), both in mm³/MPa. Its delayed volume W follows τ dW/dt = Cd P − W. Stock shortening is (Ci P + W)/A. An independent melt compliance Cm stores Cm P. The nozzle emits P/R and the hob supplies at most μN/A in pressure. The coupled system uses an implicit volume balance with a unilateral pressure cap at each step. Equal moduli reduce to a purely elastic stock spring. Removing the stock record restores the original analytical chamber update.

At each sample, A × accepted feed equals deposit + travel ooze + melt storage + stock storage. The wire solver supplies the shape upstream of the hob. Its pins represent geometric supports through which material passes. Particle identity does not track the transported stock. The downstream model assumes confined, small-strain axial compression, a uniform cross-section and pressure, and forward feed. It excludes local wall friction, buckling, temperature-dependent material properties and bead solidification.

The belts use a second, retarded extension at the signed instantaneous span load. For instantaneous displacement u and creep ratio b, τ dv/dt = b u − v. Each step integrates that equation at constant sampled load and uses u + v in carriage position. Strand tension is carried by the existing tension-only model.

## Mechanical and material response

| Object | Constitutive rule | Effect on the print |
| --- | --- | --- |
| Timing belt and pulley | For tangential demand F, installed tension T₀ and single-span stiffness k, the two tensions are T₀ ± F/2 while both spans remain taut. Beyond that point the unloaded span carries zero tension and the other carries the full load. Belt displacement follows the matching piecewise spring law. | Elastic registration error follows load. In the optional overload regime a slack span and a crossed empirical force threshold produce one pitch of persistent phase loss per loading excursion. The model rearms below 80% of the threshold or when both strands carry tension. |
| Belt teeth, shaft and mount | Effective tooth contact and mount shear give signed displacements F/k. For a circular motor shaft, twist at the pulley rim is F r² L/(GJ), where J = πd⁴/32. | All four deflections, including belt stretch, affect actual carriage position. The motor pulley follows its command. The belt and return pulley follow registered strand travel. |
| Screw nut and transmission play | A stateful play operator holds the follower while the driver crosses the gap. The initial state is centred in the gap. | A direction reversal exposes the full lost motion. Z nut clearance is shared by the synchronised screws in this example. |
| Spur tooth tolerances | Pitch-circle clearance is the sum of the two tooth-thinning allowances plus 2 Δa tan(α) for small centre-distance offsets. A deterministic periodic transmission error is sampled at tooth mesh phase. The gear flank applies the same play operator. | The pinion follows the input. The output gear and hob respond to the actual mesh. Tooth thinning changes rendered tooth widths, and centre distance changes the locations of the gear centres. The rendered flanks are trapezoidal approximations. |
| Guide shafts | The carriage load is shared equally by two simply supported circular guides. For load coordinate a and b = L − a, the deflection at the load is P a² b²/(3 E I L), using the force on one shaft. | Deflection varies with X. The X journals use a curved centreline and the head follows the sag. |
| Heated stack | Uniform free thermal expansion is α L ΔT relative to a reference height. | The nozzle extends downward relative to its carriage. The example supplies a constant temperature difference. |
| Hob and hotend | For the previous stock model, the available drive force is μN and the corresponding pressure ceiling is μN divided by incoming filament area. Below the ceiling, C dP/dt = Qᵢₙ − P/R. The solver integrates a constant inlet rate exactly over each substep and splits the step at a traction-limit crossing. Rejected feed accumulates as hob slip. | The spool follows accepted stock. Outlet flow follows pressure and can continue after feed stops. Outflow is allocated to deposited material or travel ooze according to the active move. |

The demand on each horizontal axis uses commanded acceleration and a smoothed bearing drag. This is a quasi-static, lumped-parameter model. It resolves drive compliance, bending and thermal growth at part level. Contact stress fields, vibration modes, tooth fracture, permanent plastic set, non-Newtonian melt rheology and bead adhesion require additional material and boundary data. The overload law is a demonstrator for a measured skip threshold. A geared feed advance is assumed, with nonnegative commanded extrusion. Retraction and reverse flow would require a signed material-transport law.

The deposited mesh is reconstructed in 0.125 s slices from the outlet volume and the loaded nozzle path. During print moves it uses a faceted, rounded cross-section with width derived from volume per unit length. Travel produces thin traces or drops. After the last commanded motion, discharge accumulates in one growing drop at the parked nozzle. Its shape is an equivalent-volume sphere. Small residual volumes are kept in the numerical balance even below the mesh rendering threshold. Coincident drops and corner joins can overlap. Read `fffDeposit`, `fffOoze` and `fffStored` for material accounting.

## Author a printer program

The native parser recognises one printer program per document. Each `@fff` record occupies one line. The original component records and a home position are required. The stock material and belt creep records are optional. Add one or more moves. Blank lines and trailing `#` comments are supported. A rebuild starts from the declared home and an unpressurised chamber. Seeking backwards reproduces the same state. Samples outside the program duration hold the endpoint state. Add a dwell move to simulate further discharge.

| Record | Fields in order |
| --- | --- |
| `beltX`, `beltY` | Moving mass (kg), drag (N), installed tension per span (N), one-span axial stiffness, tooth contact stiffness, mount shear stiffness (all N/mm), shaft radius and length (mm), shaft shear modulus (MPa), reversal gap (mm), ratchet threshold (N), belt pitch (mm), pulley teeth |
| `gear` | Module (mm), pinion teeth, wheel teeth, pressure angle (degrees), pinion and wheel tooth-thinning allowances (mm each), signed centre-distance error (mm), pitch-line transmission-error amplitude (mm), hob working radius (mm) |
| `guide` | Support span (mm), diameter (mm), Young modulus (MPa), shared carriage mass (kg). The example frame and journals use a nominal 240 mm span and 8 mm shafts. |
| `thermal` | Effective hot length (mm), expansion coefficient (per kelvin), signed temperature rise (K) |
| `nut` | Total axial reversal gap (mm) |
| `hotend` | Filament diameter (mm), effective compressive compliance C (mm³/MPa), flow resistance R (MPa·s/mm³), normal nip force (N), traction coefficient |
| `stockMaterial` (optional) | Density (kg/m³), instantaneous E₀ and relaxed E∞ (MPa), creep retardation time (s), confined stock length (mm). Require E₀ ≥ E∞ > 0. |
| `beltCreep` (optional) | X creep ratio, X retardation time (s), Y creep ratio, Y retardation time (s). Zero ratio recovers the original elastic belt. |
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
| Deposit, Ooze, Stored, MeltStored | mm³. Stored combines the confined stock and melt. |
| StockE0, StockDensity | MPa and kg/m³ |
| Compression, DelayedCompression, InletFeed | mm. InletFeed is the stock length delivered to the melt chamber. |
| FeedForce | N |
| XCreep, YCreep | mm of signed retarded extension, included in XStretch and YStretch |

To isolate a low-grip feeder, reduce the nip force in `@fff hotend` while holding the motion program fixed. Slip increases when pressure reaches the lower cap. To exercise belt ratcheting in the regression, the test uses a 1 kg carriage, 4 N of installed tension and a 10 N jump threshold. A 12 N demand unloads one span and loses one tooth. Holding this demand causes one event, and reversing it can cause another. For measured inputs, belt span stiffness, direction-reversal travel, tooth fit, filament advance under load, and pressure/flow response constrain different terms in the model. For an isolated linear chamber, a pressure decay constrains RC. With flexible stock, measure the axial creep response independently to separate it from melt compliance.

## Reproduce the run

```sh
make -j2 build/form-study-sample build/printer-response-csv printer-response-check flexible-body-check
python3 tests/check_cartesian_printer.py
python3 tests/check_loaded_printer.py
build/printer-response-csv examples/Mechanics/CartesianPrinter/CartesianPrinter.p3d > response.csv
```

The C++ checks cover reversible play, gear fit, belt tension and re-engagement, shear, torsion scaling, elastic bending, thermal growth, slip, pressure decay, material conservation and timestep refinement. The native geometry checks cover the original 25 reference poses and the loaded mesh at print, reversal, travel and dwell times. The CSV exports the same solver channels read by the grammar.

The engineering basis for the belt is the distinction between installed span tension, effective force and elastic registration error in the [Gates precision drive manual](https://www.gates.com/content/dam/documents-library/catalogs/light-power-and-precision-manual.pdf) and [Gates belt sizing guide](https://www.gates.com/content/dam/documents-library/installation-guides/BeltSizingGuide.pdf). The gear relation uses the small-change centre-distance approximation and tooth-thickness allowances described in [Elements of Metric Gear Technology](https://qtcgears.com/materials/catalogs/Tech.pdf). The nozzle law uses the linear compression and flow assumptions described in [Klipper’s kinematics notes](https://www.klipper3d.org/Kinematics.html#pressure-advance). Here those assumptions predict the uncompensated plant response.

The flexible-stock law uses the instantaneous and recoverable creep terms described in [Mechanical Behavior of Materials, chapter 17](https://msecore.northwestern.edu/332/332text.pdf). The upstream wire uses the [native flexible-body solver](../../FlexibleBodies/README.md).
