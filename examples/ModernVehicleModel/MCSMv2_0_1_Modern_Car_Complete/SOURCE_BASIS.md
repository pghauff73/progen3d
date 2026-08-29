# Source Basis

MCSMv2.0.1 is an integrity patch applied to the supplied MCSMv2.0.0 generator,
its generated models, validation records, tests, and the subsequent problem
audit. No new external vehicle claims were introduced for this patch.

The original audit is preserved at:

```text
evidence/MCSMv2_0_0_PROBLEM_AUDIT.json
```

Manufacturer and method references inherited from MCSMv1 remain in:

```text
data/research_data.json
```

Those references continue to support package-level research only. Geometry,
style, wheel-envelope policy, closure ranges, powertrain envelopes, and other
concept-level assumptions retain their own provenance classifications.
