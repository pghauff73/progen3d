# Cartesian filament printer

Open [CartesianPrinter.p3d](CartesianPrinter.p3d) in the ProGen3D desktop editor, fit the model and advance grammar time from 0 to 26.5 seconds. The machine approaches the bed, prints three stacked rectangular perimeters, lifts the nozzle and parks. Scrubbing backward reconstructs the earlier machine pose and the material deposited up to that instant.

## Mechanical object model

The example uses native `Object`, `Interface` and `Connect` declarations. Each shaft, linear bearing, lead nut, screw, belt, motor, drive pulley, idler and belt clamp has its own object identity. All lengths are millimetres. The printer uses its bed centre as the XY origin. ProGen3D scene Y points upward. The physical bed travels in the direction opposite to the programmed Y move, carrying all previously deposited material with it.

| Assembly | Transmission | Guidance and load path |
| --- | --- | --- |
| X carriage | X motor and 20-tooth drive pulley, 2 mm pitch belt, idler and clamp | Two horizontal 8 mm shafts on the lifting gantry. Two 14 mm long bushings slide on each shaft. The head carries the heater, heat sink and nozzle. |
| Y bed | Independent motor, matching pulley, belt, idler and bed clamp | Two frame-mounted 8 mm shafts, each carrying a front and a rear bushing. The heated bed rides on the four bushings. |
| Z gantry | Two synchronised motors and two four-start screws. Each screw has a 2 mm pitch and an 8 mm lead. A travelling nut on each end of the gantry converts rotation to lift. | Two vertical 8 mm guide shafts. Upper and lower bushings on each side carry the gantry's lateral loads. |
| Filament feed | A 5 mm effective-radius hob and an opposing idler draw 1.75 mm filament from a spool with a 25 mm payout radius. | A two-span Bowden guide maintains a 156 mm centreline between the fixed feeder and moving hotend. The guide has a schematic bend at its midpoint. |

The default model has 26 declared connections, including all 12 bore-to-journal associations and both nut-to-screw associations. Inspect `XLowerLeftBearing`, `YLeftFrontBearing` and `ZLeftNut` to trace examples. The spatial graph stores ownership and connectivity. Time-dependent rules impose the driving and following motions. The screw flank and bearing sleeves are simplified render geometry. The nominal radial running clearance in each linear bearing is 0.04 mm. Internal recirculating ball tracks are implicit.

## Toolpath and production

`Start -> Job(56 40 0.24 0.48 1.75 2 20 2 4)` sets perimeter X and Y, layer height, deposited width, filament diameter, belt pitch, pulley tooth count, screw pitch and screw start count. The frame, travel and initial lead-screw envelope are fixed for this example. Keep proposed edits within their clearances. Each side uses cubic ease-in/out with zero endpoint velocity. The example has three layers and stops at every corner.

| Time (s) | Operation |
| --- | --- |
| 0–2 | Move to the start at clearance height, then lower to the first layer. |
| 2–9, 9.5–16.5, 17–24 | Lay one 56 × 40 mm perimeter on each layer. |
| 9–9.5, 16.5–17 | Raise the gantry 0.24 mm. Keep feed constant. |
| 24–26.5 | Lift by 12 mm and park. The completed part rides with the bed. |

For belt pitch $p$ and tooth count $N$, pulley rotation magnitude is $2\pi |\Delta s|/(pN)$. Screw rotation magnitude is $2\pi |\Delta Z|/\ell$ for screw lead $\ell$. Index marks on the pulley faces and screw couplers expose the direction of rotation. Marks on the belt circulate over the two straights and both wraps. [Klipper rotation distance](https://www.klipper3d.org/Rotation_Distance.html)

For supported extrusion of width $w$ and height $h$, the idealised rounded-rectangle area is $A=h(w-h)+\pi h^2/4$. A deposited centreline length $L$ consumes $E=LA/(\pi d^2/4)$ millimetres of round stock of diameter $d$. The unprinted travel and layer lifts add zero material. [Slic3r flow math](https://github.com/slic3r/Slic3r-Manual/blob/master/src/advanced/flow-math.md)

The default job lays **576 mm** of bead, giving **59.2352 mm³** of nominal deposited volume and **24.6271 mm** of stock consumed. The final nominal wall height is **0.72 mm**. The mesh uses eight facets per semicircle and butt-ended segments. Its individual segment volumes are approximately 1.12% below the smooth cross-section calculation. The model omits corner redistribution, melt pressure, temperature, drive slip and backlash. These quantities describe an ideal kinematic print.

## Reproduce the checks

From the repository root, build the existing desktop mesh sampler and run the regression.

```sh
make -j2 build/form-study-sample
python3 tests/check_cartesian_printer.py
```

The regression checks 25 native time samples against a separate sequence of commanded moves. It measures all bearing centre axes and bore radii, shaft travel limits, pulley and lead-screw angles, clamp and nut positions, nozzle height, deposited segment geometry in the moving bed frame, and feed-wheel rotation. After the final sample it rechecks the parked machine at 40 seconds.
