# Engineering device physics

Open `examples/EngineeringDevices/RecentEngineeringDevices.p3d` in the native desktop editor. Press **Run**, **Fit** and **Play**. The first rule, `Start -> Study(0)`, displays all four devices. Select 1–4 for a single device. **Inspector → Device physics** shows temperature, contact state, stored and collected water, crawler station positions and lattice angle for the selected study.

The grammar supplies SI parameters to six native expressions. Each reads the scene clock implicitly. The editor resamples the geometry from the physical solution during playback, single stepping and seeking. Native consumers can call `scene.stepSimulation(dt)` and read `scene.devicePhysics().readings()` to obtain the same physical quantities between mesh regenerations. A signal is identified by its function name and ordered parameters. Parameters are held constant during a native step sequence. Regenerating an edited scene binds the new values. A reset restores the snapshot's starting time. A negative timeline sample shows the initial physical state.

## Models and experiments

| Study | Input order and returned quantity | An edit with a measurable consequence |
|---|---|---|
| 1. Thermal switch | `thermalClosed(ambient_K, power_W, capacity_J_per_K, open_G_W_per_K, closed_G_W_per_K, close_K, reopen_K)` returns 0 or 1. `thermalKelvin(...)` returns the node temperature. | Reduce the input from 10 W to 1 W. At 1 W the open-state equilibrium of 303.15 K is below the 315.15 K closing threshold. |
| 2. Water harvester | `gelMass(wet_s, dry_s, uptake_tau_s, release_tau_s, equilibrium_kg, capture_fraction, clock_rate)` returns stored water in kg. `collectedMass(...)` returns cumulative collected kg. | Set capture to 0. The domes still swell and shrink, and the vessel stays empty. The 0.35 kg display vessel saturates visually. The Inspector continues to report total collection. |
| 3. Pipe crawler | `crawlerStation(index, cells, pitch_m, period_s, pneumatic_tau_s, pressure_amplitude_Pa, area_m2, spring_N_per_m, grip)` returns the axial station position in metres. | Change grip from 0.55 to 0 and then −0.55. Compare the first station one full period apart. The net motion disappears and then reverses, while the cells keep deforming. |
| 4. Scissor lattice | `latticeAngle(initial_rad, rest_rad, inertia_kg_m2, damping_N_m_s_per_rad, stiffness_N_m_per_rad, torque_N_m)` returns the tilt from vertical in radians. | Increase damping from 0.17 to 0.60. For this inertia and stiffness, critical damping is approximately 0.566 N m s/rad. The higher setting removes overshoot without changing the final angle. |

**Thermal contact.** A single thermal mass satisfies `C dT/dt = P − G(T − Tamb)`. The model solves each constant-conductance interval exactly and switches at separate heating and cooling thresholds. A weak heater can leave the contact permanently open. A sufficiently strong heater can hold it closed. The model assumes uniform temperature, an ideal instantaneous snap and two effective conductances.

**Water balance.** During uptake, `dm/dt = (M − m)/tau_u`. During release, `dm/dt = −m/tau_d`. The gel starts at the minimum of its periodic steady cycle and the tank starts empty. A fraction of the released mass reaches the tank. The clock rate of 7200 maps 12 playback seconds onto one physical day. The wet capacity and rates are supplied effective properties. Predicting them from humidity, temperature or pane heat transfer requires calibration.

**Crawler traction.** Pressure oscillates about a preload. A first-order pneumatic lag reduces the oscillating amplitude by `1/sqrt(1 + (ωτ)²)` and delays it by `atan(ωτ)`. Each cell's incremental spring force balances its pressure force. The model assumes a straight pipe, quasistatic axial motion and N evenly phased drag pads. Their longitudinal drag coefficients are proportional to `1 − grip·sin(phase)`. Enforcing zero net external axial drag determines body drift. The application renders ten scene units per metre. Radial pipe contact, Coulomb stick-slip, bending, rolling and gripper actuation lie outside this model.

**Scissor deployment.** A constant torque acts on one angular degree of freedom from rest: `J θ″ + b θ′ + k(θ − θrest) = τ`. The closed-form solution covers undamped, underdamped, critical and overdamped responses. The grammar uses `H=L cos θ` and `R=L sin θ/(2 sin(π/N))` to keep the links at fixed length. Parameters should keep the tilt within the sleeve's physical range of 0 to π/2. Real end stops, distributed compliance, varying effective inertia and joint friction require additional data.

These are reduced-order teaching models. The dimensions, loads and coefficients in the example were chosen to expose the mechanisms. The cited prototypes provide the device concepts. Their measured performance was not used to fit these coefficients.

## Reproduce the native checks

From the repository root in a desktop build environment:

```sh
make -j2 all build/form-study-sample check-engineering-devices
./build/form-study-sample examples/EngineeringDevices/RecentEngineeringDevices.p3d 6 /tmp/devices-6s.obj
./progen3d-editor-gui --open examples/EngineeringDevices/RecentEngineeringDevices.p3d
```

The regression checks compare temperatures and angular motion with independently time-stepped balance equations, test water conservation and crawler force balance, and exercise scene compilation, stepping, random access, reset and invalid inputs.

## Prototype papers

- Tan et al., *Bistable origami thermal switch with high switching ratios*, Nature Communications (2026). [Paper](https://doi.org/10.1038/s41467-026-69956-2)
- Liu et al., *A metre-scale vertical origami hydrogel panel for atmospheric water harvesting in Death Valley*, Nature Water (2025). [Paper](https://doi.org/10.1038/s44221-025-00447-2)
- Zhang et al., *A Worm-Inspired Origami Robot with Multimodal Locomotion for Adaptive Mobility in Complex Pipeline Environments*, Advanced Science (2026). [Paper](https://doi.org/10.1002/advs.75500)
- Toyonaga et al., *Collapsible scissored surfaces*, PNAS (2026). [Paper](https://doi.org/10.1073/pnas.2605221123)
