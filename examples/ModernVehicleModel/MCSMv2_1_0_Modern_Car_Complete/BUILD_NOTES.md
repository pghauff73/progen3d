# MCSMv2.1 Build Notes

The released medium-resolution artifacts were generated and validated one vehicle variant per clean Python process, then merged into the final release. This avoids retained native proximity-index caches that can accumulate when several registered-surface builds run in one long-lived process on memory-constrained systems.

For a full release build:

```bash
python source/build_mcsmv21_release.py \
  --output MCSMv2_1_Complete \
  --resolution medium
```

The release builder launches four clean variant workers concurrently and merges their models, correspondence records, UV-domain graphs, validation records, reports, and comparison scene.

For one variant:

```bash
python source/build_mcsmv21_variant.py \
  --output MCSMv2_1_Reference \
  --variant reference \
  --resolution medium
```

Low resolution is intended for fast development and may not meet the final V2 correspondence gate. The distributed release uses medium resolution.
