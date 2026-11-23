# Surface contact in the native simulator

Open the gear example in the Linux editor and play the physics timeline. Select the blue wheel to inspect contact torque, angular momentum and rotational kinetic energy. Only the copper pinion has an initial angular velocity. Moving the blue wheel away leaves it at rest.

```sh
make -j2 all
./progen3d-editor-gui --open examples/gear_surface_transfer.p3d
```

For a glancing impact with friction and heat flow, open [the two-sphere example](../examples/surface_contact_transfer.p3d). The hot sphere approaches a cooler stationary sphere off-centre. On contact, tangential force spins the bodies, sliding generates heat, and conduction transfers heat between them.

## Authoring the parts

Each setting applies to the next primitive emitted. Mark both contacting bodies to enable the surface model for their interaction. Supply a closed solid with finite positive mass, volume and inertia. Geometry is in metres. The grammar accepts initial angular velocity in degrees per second.

| Grammar | Physical meaning |
| --- | --- |
| `P(density)` or `M(mass)` | Density in kg/m³ times transformed volume, or an explicit mass in kg. |
| `V(x y z)`, `VR(x y z)` | Initial linear velocity in m/s and angular velocity in degrees/s. |
| `ContactSurface(kn kt mu)` | Effective spring stiffness of this body's surface layer in compression and shear (N/m), and its friction coefficient. Set shear stiffness and friction to zero to isolate the normal force. |
| `ElasticModulus(E nu mu)` with `ContactLayer(A L)` | An isotropic material with Young's modulus in Pa, Poisson ratio and friction coefficient. Effective patch area is in m² and layer thickness in m. The solver uses normal stiffness EA/L and shear stiffness GA/L, where G = E/[2(1 + nu)]. |
| `ContactThermal(T cp H)` | Initial temperature in kelvins, specific heat in J/(kg·K) and layer conductance in W/K. The heat capacity of a body is its mass times its specific heat. |
| `ContactShaft(x y z)` | Constrains the centre of mass and allows rotation about this world-space axis. Start with zero translation and spin parallel to the shaft. |
| `!I(...)` | A fixed body can receive contact reaction, frictional heat and conducted heat. |

To drive a shaft continuously, the desktop airflow/load grammar also accepts `BodyInertia(Ix Iy Iz)` and `BodyTorque(tx ty tz)`. The supplied principal moments (kg·m²) override the uniform-density estimate. The bearing reacts against transverse force and torque.

For example, both gear rules use `ContactShaft(0 0 1) ContactSurface(2000 0 0) P(7800)`. The material defaults for a marked body are 10,000 N/m normal stiffness, 2,500 N/m shear stiffness, friction coefficient 0.4, initial temperature 293.15 K, specific heat 500 J/(kg·K) and zero conductance. Supplying either zero layer conductance insulates the pair.

## Interpreting the inspector

- **Motion.** The solver evaluates contact-point velocities using **v + ω × r**. It applies equal and opposite impulses at the same point. Each body's torque about its centre of mass is **r × F**. The force and torque displayed are averages over the most recent public simulation step. Multiplying them by that step gives the corresponding contact impulses. A shaft can carry the transverse reaction.
- **Mass and energy.** The solver uses a uniform-density inertia tensor, analytic for a sphere and integrated from the closed triangle mesh for other solids. Translational kinetic energy is ½ m|v|², rotational kinetic energy is ½ ω·Iω, and gravitational potential is −m g·x relative to the scene origin.
- **Deformation.** The two layers act in series. Each stores elastic potential during compression or sticking shear. The friction limit is the smaller material friction coefficient times the normal force. The inspector exposes the largest local compression and shear displacement at the end of the step. Providing an effective area and thickness also gives nominal layer stress and strain.
- **Heat.** Sliding work becomes heat, divided equally between the two bodies. A separating contact also releases its remaining tangential spring energy as heat. Conduction uses the two layer conductances in series and moves heat from hot to cold while the pair is in contact. Conducted heat is signed. Frictional heat is cumulative. When checking total energy, count generated heat once, either through the temperature rise of the pair or the cumulative heat ledger.

## Reproducing the numerical checks

```sh
make check-surface-contact
```

| Experiment | Reference or observed result |
| --- | --- |
| Two equal 2 kg cubes, approaching at 1 m/s each | Analytic peak compression per layer is 22.361 mm. The native test measured 22.359 mm and recovered 2.00002 J of kinetic energy after separation. |
| Isolated glancing spheres | Both linear and angular momentum are checked. Sliding and release generated 0.02133 J, and 146.37 J flowed from the hot sphere to the cold sphere. |
| Two fixed contacting bodies at 340 K and 300 K | Temperatures and transferred heat are compared with the exact two-capacity exponential solution. |
| Free fall | The test compares speed with gravitational acceleration times elapsed time and checks kinetic plus gravitational potential energy. |
| 20/40-tooth gears on parallel shafts | With a +90°/s pinion and a stationary wheel, the wheel reached −18.23°/s during the initial engagement. At a 25 µs step the maximum energy discrepancy was 0.31% over 40 ms. Angular impulse and work are checked against the wheel's momentum and energy. A separated wheel stays at rest. |

The gear test measures an unloaded transient with a compliant tooth contact. Its shaft speeds can rebound. The 2 mm module, 60 mm shaft spacing and 8 mm face width specify the geometry. The example's stiffness is chosen to make the transient observable.

The dedicated spur-gear detector samples the lateral boundaries of overlapping, parallel, shaft-mounted solid polygon extrusions and selects one deepest patch per pair. This discretisation limits predictions of load sharing, fast impacts and tooth stress. The deformation model is local to each contact layer. The body mesh and volume remain rigid, and temperature is uniform within each body. Use timestep refinement and a geometry appropriate to the experiment when evaluating new mechanisms.
