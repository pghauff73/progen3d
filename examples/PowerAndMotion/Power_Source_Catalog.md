# Power source catalog

Open `Power_Source_Catalog.p3d` in ProGen3D. Select a module to inspect its object class, taxonomy, and ports. The solar panel has a native `Powers` connection to the telemetry module. The stored gas, fluid, and flywheel examples export gas, fluid, and shaft interfaces. Electrical use of those stores requires a converter with an electrical output.

## Battery material examples

| Cell family | Positive electrode | Negative electrode | Electrolyte example |
| --- | --- | --- | --- |
| LFP lithium-ion | LiFePO₄ | Graphite C | LiPF₆ in organic carbonates |
| NMC811 lithium-ion | LiNi₀.₈Mn₀.₁Co₀.₁O₂ | Graphite C | LiPF₆ in organic carbonates |
| Prussian-white sodium-ion | NaₓM[Fe(CN)₆], with M a framework metal | Hard carbon C | NaPF₆ in carbonate solvent |
| Nickel–metal hydride | NiOOH during discharge, converting to Ni(OH)₂ | Hydrogen storage alloy MH, converting to M | Aqueous KOH |
| Lead–acid | PbO₂ | Pb | Aqueous H₂SO₄ |

In the lead–acid cell the overall discharge reaction is Pb + PbO₂ + 2H₂SO₄ → 2PbSO₄ + 2H₂O. The entries describe representative constituent chemistry. Commercial formulations vary across manufacturers and products. [Argonne BatPaC materials](https://afdc.energy.gov/files/u/publication/cradle_to_grave_report.pdf) · [Argonne NMC811](https://www.anl.gov/argonne-scientific-publications/pub/199958) · [DOE sodium-ion materials](https://www.energy.gov/sites/default/files/2022-02/Energy%20Storage%20Supply%20Chain%20Report%20-%20final.pdf) · [Panasonic NiMH](https://energy.panasonic.com/dam/master/pdf/eu/material/Panasonic%20Ni-MH%20Batteries%20Handbook.pdf) · [GS Yuasa lead–acid](https://news.yuasa.co.uk/wp-content/uploads/2015/05/YuasaEndurance_Manual.pdf)

## Rating and discharge examples

- The photovoltaic rule computes DC nameplate power from efficiency × illuminated area × irradiance. The example is 252 W from 21% efficiency, 1.2 m², and 1,000 W/m². The reference rating uses terrestrial standard test conditions of 25°C cell temperature and AM1.5G spectrum. [NREL module efficiency and test conditions](https://sam.nrel.gov/images/web_page_files/sam-help-2024-12-12.pdf)
- The supercapacitor meters show terminal voltage, internal stored energy, and output power during a constant-current pulse. For capacitance `C`, internal voltage `V`, load current `I`, and series resistance `ESR`, the rule uses `E=C*V²/2`, `Vterminal=V-I*ESR`, `P=I*Vterminal`, and `dV/dt=-I/C`. Display time advances the modeled pulse eight times faster. The initial sample is 23.76 V, 34.56 kJ, and 475.2 W. The power meter disappears at the 18 V cutoff after 34.56 modeled seconds. [Eaton supercapacitor application guide](https://www.eaton.com/ca/en-gb/products/electronic-components/topics/supercapacitor-applications-guide.html)
- The flywheel example exposes a shaft. Its stored energy is `I*omega²/2`, angular momentum is `I*omega`, and the shaft transfers torque × angular velocity. Extraction slows the rotor and requires a net shaft torque. [DOE flywheel demonstration](https://www.energy.gov/oe/articles/arra-sdgp-amber-kinetics-inc-flywheel-energy-storage-demonstration)

Solar-heated steam generation, gas turbines, biodiesel generators, hydrogen/methane turbines, and fuel cells are source interface examples. Laser and microwave receivers represent conversion of externally supplied energy. The violet exhibits are conceptual and fictional catalog entries with no simulated electrical outputs. [DOE solar thermal cycles](https://www.energy.gov/cmei/systems/concentrating-solar-thermal-power-csp-power-cycles) · [DOE gas turbines](https://betterbuildingssolutioncenter.energy.gov/resources/combined-heat-and-power-technology-fact-sheet-gas-turbines) · [DOE fuel cells](https://www.energy.gov/onsite-energy/technologies) · [IBM on quantum entanglement and classical communication](https://quantum.cloud.ibm.com/learning/en/modules/computer-science/quantum-teleportation)
