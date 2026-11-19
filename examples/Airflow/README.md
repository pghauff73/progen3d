# Airflow studies

Choose **Examples** in the desktop editor toolbar and select an airflow study. It opens as an editable copy. Select the loaded primitive and run the physics timeline. The inspector reports the world force and moment about its centre of mass. The cross marks that centre, and arrow lengths show the relative magnitudes of component forces on the selected body. Dynamics use metres, kilograms, seconds, newtons and radians internally. The existing VR action and rotational-velocity inspector use degrees per second.

| Example | Experiment |
|---|---|
| OffsetFan.p3d | Change the lateral mount from +0.08 to 0 or −0.08 m. Forward acceleration stays 0.600 m/s². Initial yaw acceleration is −0.800, 0 or +0.800 rad/s². |
| FanCart.p3d | Run the 2 kg cart using the measured Aeronaut 8.5 × 6 propeller curve at 5,004 rpm. Equilibrium is about 8.16 m/s for the assumed drag area. Roll is free about the thrust axis. |
| DuctedFanStand.p3d | Select the fixed inlet shroud. The synthetic fan and system curves intersect at 0.400 m³/s and 72 Pa. Change the elbow angle to edit the geometry. Installed forces from a changed duct require matched measurements or flow fields. |

## Grammar actions

Use BodyInertia and component actions immediately before the I or !I they describe. A fixed primitive can act as a test stand and report loads. Application points are metre offsets from the primitive's collision-geometry centre of mass in its right-handed principal body frame. A scaled drawing retains metre-based lever arms. AirProperties and Wind apply to the whole scene, with the last declaration taking effect.

| Action | Inputs and effect |
|---|---|
| AirProperties(rho mu), Wind(x y z) | Air density in kg/m³, dynamic viscosity in Pa·s and world air velocity in m/s. Defaults are 1.2, 0.000018 and still air. |
| BodyInertia(Ix Iy Iz) | Positive principal moments in kg·m² satisfying the triangle inequality. Required for every airflow body. Align the body's authored axes with its principal axes. |
| LoadPoint(x y z), LoadAxis(x y z) | Set the offset and direction of subsequent components on this primitive. Default application point is the centre of mass and default axis is +X. |
| BodyForce(x y z), BodyTorque(x y z) | Add body-frame force in N at the current point, or a free couple in N·m. Repeated loads sum. |
| ForceRamp(start end gain) | Scale the body's prescribed forces and free couples from zero at start to the final gain at end, in simulation seconds. Drag and propeller loads keep their own laws. |
| DragSurface(Cd A L), LiftSurface(Cl A L) | Reference area in m² and positive reference length in m. Uses point velocity including angular motion. Lift acts along the projection of LoadAxis normal to relative velocity. Constant coefficients apply for the chosen operating conditions. |
| PropSample(J CT CP), Propeller(D rpm spin) | Supply at least two rows at strictly increasing advance ratio for this operating RPM, followed by diameter in metres, speed and spin sign (+1 or −1). Axial thrust, shaft power and opposing mount torque use linear interpolation. Stopped rotors supply zero powered load. Outside the table the inspector flags the omitted rotor contribution. |
| DuctStation(Q fanPa systemPa) | Supply fan rise and system loss on the same pressure basis and at the same RPM, ordered by increasing volume flow in m³/s. Linear interpolation reports unique, multiple, coincident or missing intersections. A flow result supplies no implicit airframe thrust. |
| PlateFriction(A L regime) | Aligned, smooth plate, one wetted face. Regime 0 requests a laminar reference, 1 assumes transition at Re = 500,000 and 2 assumes a tripped leading edge. Turbulent correlations are limited here to 500,000 through 10,000,000. The plate assumes tangential incident flow. |

Use the physics clock and ForceRamp for a changing prescribed load on a moving body. The editor's grammar-time variable t regenerates geometry as a separate sampling mode. Propeller curves belong to their measured assembly and RPM. The single-speed map does not infer changes in efficiency, leakage, noise or transition from visual blade geometry.

## Reproduce the checks

Run `make airflow-check` using the desktop build dependencies. The harness compiles ordinary grammar text, expands it, generates native geometry and calls the same stepSimulation method as the editor. To record a 30 s CSV trace at 120 Hz use `./build/airflow-test --trace examples/Airflow/FanCart.p3d 3600`. Time zero records the initial pose.

To import a steady CFD surface snapshot, run `python3 tools/import_airflow_faces.py faces.csv`. The fields are patch, x, y, z, area_x, area_y, area_z, pressure, shear_x, shear_y and shear_z. Use metres relative to the centre of mass, outward body area vectors in m², gauge pressure in Pa and fluid-on-body viscous traction in Pa, all in body axes. Insert the resulting actions after BodyInertia and before the body primitive. The importer sums pressure and shear and preserves the distributed moment for each patch as a force at the centre plus a free couple. Select one source per physical surface. This is one-way snapshot coupling.

[Measured propeller data, University of Illinois](https://m-selig.ae.illinois.edu/props/volume-1/data/ance_8.5x6_2850cm_5004.txt) · [Flat-plate correlations, Penn State](https://www.me.psu.edu/cimbala/me320/Lesson_Notes/Fluid_Mechanics_Lesson_13D.pdf)

### Export the duct surface

The native geometry sampler exports triangles in world coordinates. With the supplied metre-scale test stand, the mapping below collects the 27 generated parts into four named surface regions. If the grammar changes the part count or order, review the mapping before solving. Inspect orientation and close the fluid domain, including inlet and outlet boundary planes, in the external mesher.

```sh
make build/form-study-sample
./build/form-study-sample examples/Airflow/DuctedFanStand.p3d 0 stand.obj
python3 tools/export_airflow_patches.py stand.obj stand.stl --patch-map examples/Airflow/DuctedFanStand.patches.json
```

Record the grammar revision, frame, origin, pressure reference, mesh and solver configuration with every surface field. Transform face centres, area vectors and tractions into the intended body's frame before importing.

### Compare with independent flow data

The optional reference study integrates published CFL3D and FUN3D wall-shear profiles with this project's face importer. Their finest-grid plate drag coefficients are recovered to 0.075% and 0.031% respectively. The script reports the three finest published meshes. Its moment check applies a known 0.25-unit offset to the published plate drag.

It also compares the native assumed-transition law against 16 ERCOFTAC T3A stations, using the derivative of integrated native drag to obtain local skin friction. At local Reynolds number 309,300 the native coefficient is 0.001194 and the experiment reports 0.004861. This discrepancy limits use of the correlation for bypass transition and curved duct walls.

```sh
curl -fsSLO https://tmbwg.github.io/turbmodels/FlatPlate/SA/cf_plate.dat
curl -fsSLO https://tmbwg.github.io/turbmodels/FlatPlate/SA/drag_convergence.dat
curl -fsSLO 'https://web.tecnico.ulisboa.pt/~ist12278/Workshop_AVT_313_2D_cases/html_files/plate_grids/cf_rex_data_t3a.dat'
python3 tests/airflow_reference_studies.py cf_plate.dat drag_convergence.dat cf_rex_data_t3a.dat
```

The native trace also records angular velocity in rad/s and the unique duct intersection in m³/s and Pa. Empty duct fields indicate that the body has no unique operating point.

[Turbulence Modeling Resource, flat-plate verification](https://tmbwg.github.io/turbmodels/flatplate_sa.html) · [AVT workshop, ERCOFTAC experimental data](https://web.tecnico.ulisboa.pt/~ist12278/Workshop_AVT_313_2D_cases/html_files/Experimental_data_plate.htm)
