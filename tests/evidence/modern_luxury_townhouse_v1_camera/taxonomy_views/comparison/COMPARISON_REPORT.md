# ModernLuxuryTownhousev1 Taxonomy Camera Comparison

The comparison covers every renderable townhouse taxonomy object in front, back, left, right, top, and bottom orthographic views.
ImageGen cards are design references, not authoritative geometry. Perceptual metrics are advisory; deterministic grammar generation, spatial resolution, and camera capture remain the acceptance authorities.

- Renderable taxonomy objects: 21
- Views per object: 6
- Total comparisons: 126
- Excluded taxonomy nodes: `MLT_V1` (whole scene) and `MLT_PRIMITIVE_COVERAGE_STUDY` (non-building language test)

| Object | Class | Area | Mean appearance NRMSE | Mean edge NRMSE |
|---|---|---|---:|---:|
| `MLT_SITE` | BuildingSite | Site | 0.252355 | 0.152589 |
| `MLT_COURTYARD_PAVING` | PavedSurface | Courtyard | 0.199317 | 0.123463 |
| `MLT_STRUCTURE` | StructuralSystem | Building | 0.232033 | 0.136011 |
| `MLT_STREET_FACADE` | Facade | StreetElevation | 0.190535 | 0.136201 |
| `MLT_STREET_BALCONIES` | BalconySystem | StreetElevation | 0.223357 | 0.124114 |
| `MLT_STREET_PRIVACY_SCREEN` | FacadeFinSystem | StreetElevation | 0.178654 | 0.146599 |
| `MLT_STREET_SHEET_DETAILS` | FacadeDetailSystem | StreetElevation | 0.195018 | 0.161688 |
| `MLT_COURTYARD_FACADE` | Facade | CourtyardElevation | 0.198437 | 0.125364 |
| `MLT_ROOF_TERRACE` | RoofTerrace | RoofTerrace | 0.222715 | 0.146525 |
| `MLT_ROOF_TERRACE_SURFACE` | WalkableRoofFinish | RoofTerrace | 0.200102 | 0.119887 |
| `MLT_ROOF_CANOPY` | RoofCanopy | RoofTerrace | 0.208931 | 0.126918 |
| `MLT_COURTYARD_AMENITIES` | CourtyardAmenitySystem | Courtyard | 0.176782 | 0.126944 |
| `MLT_COURTYARD_DINING_SET` | OutdoorDiningSet | Courtyard | 0.231766 | 0.122029 |
| `MLT_COURTYARD_SINK_CABINET` | OutdoorSinkCabinet | Courtyard | 0.232572 | 0.128479 |
| `MLT_ROOF_LOUNGE_SET` | OutdoorLoungeSet | RoofTerrace | 0.169843 | 0.129751 |
| `MLT_INTERIOR_ACCENT_CHAIR` | Chair | Interior | 0.242173 | 0.125314 |
| `MLT_VEGETATION_SYSTEM` | LandscapeSystem | Landscape | 0.254146 | 0.196116 |
| `MLT_ROOF_SCULPTURAL_PLANTER` | Planter | RoofTerrace | 0.163544 | 0.118223 |
| `MLT_CURVED_LANDSCAPE_FIXTURES` | LandscapeFixtureSystem | Landscape | 0.143059 | 0.156804 |
| `MLT_STREET_GATE` | FenceGateSystem | StreetBoundary | 0.203685 | 0.158095 |
| `MLT_WATER_TANK` | WaterStorageTank | CourtyardServiceZone | 0.239671 | 0.136323 |

## Interpretation

- Review the per-object `imagegen_vs_camera.png` sheets for composition, part count, proportions, and material intent.
- Treat large metric distances as prompts for human inspection, not automatic grammar failures.
- Use the camera renders as reproducible evidence and the ImageGen cards as visual design targets.
