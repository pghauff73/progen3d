# SMB-OMv2 Instance SMB_001

This directory contains the deterministic generator and object-model manifest
for the spatially aware 124-object Small Modern Building fixture.

```text
generate_smb_omv2.py
SMB_OMv2_Spatial_Object_Model.json
```

The generated executable grammar is written beside the source examples:

```text
examples/SMB_OMv2_Instance_SMB_001_SpatiallyAware.p3d
```

Regenerate and verify it with:

```bash
python3 examples/SMB_OMv2_Instance_SMB_001/generate_smb_omv2.py
./tests/run_smb_omv2_checks.sh
```

See `docs/SMB_OMv2_SPATIALLY_AWARE_BUILDING.md` for the geometry ownership,
`CubeY` positioning, connection, constraint, evidence, and P0 limitation
contracts.
