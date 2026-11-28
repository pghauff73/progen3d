# Meridian U11 powered drives

Open `Meridian_U11_Drive_Systems.p3d` and play the grammar timeline. The socket turns 60° and returns, and the opposed screw moves each guide jaw 30 mm. The battery feeds a distribution bus and individual motor inverters. Bearings constrain the authored shaft and jaw axes.

The source uses SI measurements at the original U11 authoring scale. The permanent-magnet motors, gear losses, belt compliance, and jaw loads are represented by inverse-dynamics estimates matched to the authored motion. Legacy material densities and sheet thicknesses are editable estimates in `physical_materials.py`.

Run `python motion_model.py --check` to verify the motion profiles and sizing report. To regenerate the grammar, pass the pinned source and output filename to `integrate_mechanisms.py`. The generator checks the source blob before applying changes. [Original U11 equipment grammar](https://github.com/pghauff73/progen3d/blob/583c949/grammar/spacecraft/meridian/orbital-robot/meridian-u11-hidden-equipment/Meridian_U11_Equipment_Networks.p3d)

The runtime power graph and the utility bench are described in [Power and motion grammar](../README.md).
