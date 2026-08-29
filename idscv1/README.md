# IDSCv1

IDSCv1 is the local deterministic shared-state authority for constrained agents.
Agents receive bounded context packs and submit typed proposals. Only
`StateCommitKernel` may append accepted transition events.

```text
PYTHONPATH=idscv1/src python3 -m unittest discover -s idscv1/tests -p 'test_*.py'
PYTHONPATH=idscv1/src python3 -m idsc.cli.main demo --database /tmp/idscv1-demo.sqlite3
PYTHONPATH=idscv1/src python3 -m idsc.cli.main verify --database /tmp/idscv1-demo.sqlite3
```

The implementation uses only the Python standard library. SQLite runs in WAL
mode, canonical state is reconstructable from the hash-chained event stream,
and renderings remain derived evidence rather than geometry truth.
