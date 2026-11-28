# Power and motion grammar

Open `Utility_Bench.p3d` for the connected electrical, pneumatic, hydraulic, and rotating systems. Open `Power_Source_Catalog.p3d` to browse the generation and storage vocabulary, battery compositions, and animated solar and supercapacitor examples.

## Motor supply warnings

Declare an available source as `class(PowerSource)`. Use `class(ElectricMotor)` or `taxonomy(Power ElectricMotor ...)` for an electrical load. Established motor classes such as `PermanentMagnetMotorStator` and `ServoMotor` already identify loads. Specify the direction of energy delivery with `Powers` connections between `ElectricalPort` interfaces.

```
Connect(id(DriveFeed) source(MainRegulator Output)
         target(DriveMotor Input) type(Powers))
```

A component that passes power between terminals, such as a closed switch, regulator, transformer, conductor, or brush/slip-ring assembly, uses `taxonomy(Power Transfer ...)`. An open switch stops the route. Use `taxonomy(Power Switch Open)` or omit its outgoing power connection in the open state. Model an unavailable source by conditionally emitting its active source class or its outgoing connection. A source catalog entry is informational.

The compiler checks declared topology during grammar compilation. The Linux editor reports an unreachable motor through its native `EPG001` warning. The Windows CLI reports `MOTOR_WITHOUT_POWER_SOURCE` in `validate` output. Both keep an otherwise valid grammar usable. Ratings, electrical state, grounding, overload and fault current require circuit analysis.

## Quantities and units

| Quantity | Relationship |
| --- | --- |
| Conductor voltage drop and loss | ΔV = IR and P_loss = I²R |
| Electrical input or output power | P = VI |
| Gauge pressure and ideal actuator force | p = F/A and F = Δp × A |
| Shaft power and flywheel energy | P = τω and E = Iω²/2 |
| Flywheel angular momentum | L = Iω and dL/dt = τ_net, including the reaction torque on the rest of the assembly |
| PV panel output | P ≈ ηGA, with a documented efficiency and rating condition |
| Supercapacitor under constant current | V_terminal = V_initial − I_c(ESR + t/C), P = V_terminal I_c, and stored energy E = CV²/2 |

Use pascals for pressure, watts for power, metres for geometry, seconds for time, and radians per second for shaft speed. The utility bench includes data/control interfaces and electrical, gas, liquid and shaft links as separate networks. Pipe dimensions, pressure ratings, flywheel ratings, and loads are illustrative model values.

The catalog lists batteries by their electrode and electrolyte compositions. It distinguishes supplied power, stored energy, and a source description. Nuclear and speculative source entries are layout placeholders. The model uses mechanical, fluid and electrical equations only where it declares the supporting inputs.

## References

- Battery materials and example chemistries. [Argonne life-cycle report](https://afdc.energy.gov/files/u/publication/cradle_to_grave_report.pdf) · [DOE storage supply chain assessment](https://www.energy.gov/sites/default/files/2022-02/Energy%20Storage%20Supply%20Chain%20Report%20-%20final.pdf) · [Panasonic NiMH handbook](https://energy.panasonic.com/dam/master/pdf/eu/material/Panasonic%20Ni-MH%20Batteries%20Handbook.pdf) · [GS Yuasa lead-acid manual](https://news.yuasa.co.uk/wp-content/uploads/2015/05/YuasaEndurance_Manual.pdf)
- Efficiency at standard terrestrial test conditions. [NREL solar efficiency guide](https://nrel.gov/pv/assets/pdfs/nrel-record-cell-efficiency-data-table-guide.pdf)
- Energy, internal resistance, and discharge. [Eaton supercapacitor application notes](https://www.eaton.com/ca/en-gb/products/electronic-components/topics/supercapacitor-applications-guide.html)
- Pressure, electrical power, and rotating storage. [NASA Pascal’s principle](https://www.grc.nasa.gov/www/k-12/WindTunnel/Activities/Pascals_principle.html) · [OpenStax electric power](https://openstax.org/books/college-physics/pages/20-4-electric-power-and-energy) · [DOE flywheel report](https://www.energy.gov/sites/default/files/2017/08/f35/Amber_Kinetics_Final_Technical_Report.pdf)
