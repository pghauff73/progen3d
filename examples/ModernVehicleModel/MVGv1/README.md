# MVGv1 Examples

`MVG_Test_EV_001.json` records the canonical acceptance input for the MVGv1
vehicle builder. The executable source of truth is
`ModernEvFastbackBuilder::buildAcceptanceVehicle()`; the JSON exists for human
inspection, editor prototyping, and future grammar lowering.

Generate and validate the vehicle with:

```bash
./tests/run_mvg_vehicle_checks.sh
```

The generated PLY is written to:

```text
/tmp/progen3d-mvg-tests/MVG_Test_EV_001.ply
```
