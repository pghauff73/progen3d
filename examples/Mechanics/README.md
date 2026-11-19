# Native mechanics examples

- [Nested LED ring drive](CoaxialRingDisplay.md) covers coupled hoop motion, motor gearing and signed shaft power.
- [Pin and roller members](#pin-and-roller-service-analysis) covers floor and shelf deflection.

## Pin and roller service analysis

Open **PinRollerMembers.p3d** in the native editor and build the grammar. Select **FloorMember** or **BookShelf** in the Spatial Object Inspector, or pick a primitive bound to that member. The inspector shows analytical section mass, span, loads, reactions, individual seat pressures, bending stress and midspan deflection. The centreline diagram overlays the unloaded line and the loaded shape. Its checkbox switches displacement magnification between 1× and 25×.

| Example | Reaction at each seat | Midspan deflection | Mean seat pressure |
|---|---:|---:|---:|
| Steel floor member | 2,492.56 N | 2.007 mm | 0.33234 MPa |
| Shelf carrying 20 kg | 111.84 N | 1.705 mm | 0.01864 MPa |

Change the last parameter of `Shelf(0.8 0.30 0.018 20)` to change the distributed payload in kg. Increasing panel depth changes both self-weight and bending stiffness. The shelf material and service loads in this example are assumptions.

## Declaration

Add a study inside a spatial `Object`. Existing rectangular interfaces supply the span between their centres and the full contact area at either end. The two interface IDs are local to the owning object. Fields accept ordinary grammar expressions in numeric positions. Every field is required exactly once.

```text
PinRollerMember(
    supports(LeftSeat RightSeat)
    metresPerUnit(1)
    material(AssumedPanel 6000000000 650)
    section(0.30 0.018 0)
    load(0 20 0)
)
```

| Field | Meaning |
|---|---|
| supports | Pin interface followed by roller interface. Both must be distinct, coplanar rectangular Seat, Support or Bearing interfaces. The method admits exactly two such interfaces on this member. |
| metresPerUnit | Physical metres per local scene unit before applying the owning object's uniform world scale. |
| material | Engineering material identifier, effective span-direction Young's modulus in Pa, density in kg/m³. The appearance material on each rendered primitive is independent. |
| section | Breadth, vertical depth and uniform wall thickness in local scene units. A zero wall denotes a solid rectangle. A positive wall denotes a sharp-corner rectangular hollow section. |
| load | Additional uniform downward line load in N/m of physical span, distributed payload in kg over the entire span, and a downward central point force in N. The study adds its member self-weight once at 9.80665 m/s². |

The case models short-term, small-deflection bending of a level, prismatic member in a Y-up world. Seat normals must be parallel and equally directed. Rigid placement and uniform object scaling are supported. For an unsupported arrangement the inspector explains the missing condition. All engineering properties are explicit inputs. Section geometry and span define the analytical member mass independently of decorative geometry and preview collision-body masses.

For span L, flexural rigidity EI, line load w and central force F, the reaction per support is (wL + F)/2. Maximum bending moment is wL²/8 + FL/4 and midspan deflection is 5wL⁴/(384EI) + FL³/(48EI). Each contact pressure uses that interface's own area. The plotted shape evaluates the full beam solution along the span. Assessing load capacity also requires applicable material strength, connection, stability and serviceability criteria.

Run the native grammar, selection, sensitivity, unit-conversion and unsupported-case tests from the repository root with `make member-check`.
