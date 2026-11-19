# Nested LED ring drive

Open **CoaxialRingDisplay.p3d** in the native editor and build it. At 1 second the rings are accelerating, at 12 seconds they are cruising, and at 73 seconds they are braking. Select **OuterRing**, **MiddleRing** or **InnerRing** in the Spatial Object Inspector, or pick one of its parts. Each ring carries the next drive housing. The inspector evaluates the whole chain at the selected animation time and displays results for the picked stage.

The 2,210 primitives reproduce the visible pedestal, open bays, controller modules, cable runs and central hub, with inferred drive housings and electrical transfer parts. The model assumes a shared vertical shaft line and three positive local Y axes. The illustrative outer diameter is 1.8 m. The inertial radii, masses, transmission data and internal mechanisms are design assumptions. The photograph supplies no component ratings.

## Author a stage

Place one declaration inside each driven spatial object. A parent object's transform carries its descendants. Unmarked intermediate objects act as rigid, massless adapters. Separate fixed-base chains can coexist under a fixed assembly.

```text
CoaxialDrive(
    hoop(16 0.82)
    motion(15 7.5)
    transmission(20 0.85 0.80)
    motor(0.0005 0.15)
)
```

| Field | Arguments in order |
|---|---|
| `hoop` | Body mass in kg, effective hoop radius in metres. Both are positive. The body excludes its separately modelled motor rotor. |
| `motion` | Signed joint speed in °/s and acceleration in °/s², relative to the carrier. Both describe the same instant. |
| `transmission` | Signed motor-to-joint speed ratio, driving efficiency and backdrive efficiency. A negative ratio reverses motor sense. Efficiencies range above zero up to one. |
| `motor` | Motor rotor inertia in kg·m² and effective output-side running drag in N·m. Both may be zero. |

The declaration consumes the prescribed motion. The example derives its rendered angles, speeds and accelerations from the same trapezoidal speed profile. Edit the three rates on the `Start` rule to test other speeds. The profile carries completed travel forward across cycles. The default rates make whole turns and return to the original pose every 74 s. Other rates continue from their previous end pose.

## Check the motion and loads

| Default study | Outer | Middle | Inner |
|---|---:|---:|---:|
| Own diameter-axis inertia, kg·m² | 5.3792 | 1.9220 | 0.4410 |
| Cruise speed relative to carrier, °/s | +15 | −35 | +50 |
| Cruise speed relative to room, °/s | +15 | −20 | +30 |
| Signed cruise motor speed, rpm | +50.0 | −116.7 | +166.7 |
| Startup torque from hoop inertia at each joint, N·m | +0.4841 | −0.2200 | +0.1155 |

At cruise the hoops store **0.36189 J** in total. The inspector reports motor rotor energy separately. Torque from ring inertia, total gearbox output torque and motor shaft torque are separate readings. At cruise the running drag still requires motor power.

For stage i, the body inertia is J = mR²/2 about a diameter. Add the relative joint rates along the chain to get each body’s world angular speed Ω. For gear ratio G, the motor's speed relative to the stator is G times the relative joint speed, and its absolute rotor speed adds the carrier's world speed. Apply the same relationships to acceleration. Body and rotor energy each use one half of inertia times absolute angular speed squared.

The return pass sums each descendant hoop’s Jα into the supporting joint torque. The gearbox output also carries the inertial torque of downstream motor rotors and the local running drag. Driving uses output torque divided by G and forward efficiency. Backdriving uses output torque multiplied by reverse efficiency and divided by G. The motor supplies the additional torque needed to accelerate its own rotor in the world frame. Signed shaft power uses motor speed relative to its stator. Negative shaft power indicates mechanical energy arriving at the motor. Electrical recovery depends on the drive and supply.

At zero relative speed the requested acceleration sets the incipient drag direction. At zero speed and zero acceleration, the model uses zero running drag and ideal transmission for any holding load. Each phase boundary uses the incoming phase’s acceleration at its start. The study assumes a fixed base, rigid vertical collinear shafts and centred hoop masses. The model diagnoses displaced or tilted shafts, scaled frames and branching driven chains. Bearing reactions, structural deformation, stiction, compliance, thermal duty and electrical current require additional inputs and analyses.

Run `make drive-check` for the native grammar, pose, inspector, energy balance and failure-case regressions. The underlying inverse-dynamics and geared-inertia formulation is described in [Modern Robotics §8.1.3](https://modernrobotics.northwestern.edu/nu-gm-book-resource/8-1-3-understanding-the-mass-matrix/) and [§8.9](https://modernrobotics.northwestern.edu/nu-gm-book-resource/8-9-actuation-gearing-and-friction/).
