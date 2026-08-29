# Modern Car MCSMv1 Package

## Included

- `models/*.glb`: full colored 3D scenes
- `models/*.obj`: broadly compatible mesh exports
- `models/*_body.stl`: watertight body-only meshes
- `curves/reference_curve_network.json`: analytic longitudinal curves and 3D sections
- `curves/reference_station_table.csv`: parameter stations
- `MCSMv1_Modern_Car_Family.p3d`: proposed Progen3D/FGKv1 grammar
- `research_data.json`: official-source dimensional data
- `model_parameters.json`: complete variant and station parameters
- `validation.json`: mesh and package checks
- `previews/*.png`: orthographic and isometric engineering previews
- `RESEARCH_REPORT.md`: methods, equations, sources and limitations

## Regenerate

```bash
python modern_car_parametric_model.py --output generated --resolution medium
```

Resolution presets:

- `low`: fastest, suitable for grammar iteration
- `medium`: package default
- `high`: denser body mesh

Dependencies: NumPy, SciPy, scikit-image, trimesh, Pillow. VTK is optional and used only for PNG previews.
