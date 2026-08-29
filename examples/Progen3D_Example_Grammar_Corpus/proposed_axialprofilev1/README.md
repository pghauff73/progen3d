# AxialProfilev1 proposed examples

These files are **design fixtures**, not current executable grammars.

P0 concepts exercised:

- `axis(y)`
- reusable simple closed `polygon` profiles
- `at`
- `hold`
- `linear`
- `step`
- per-section `center`, `scale`, and `rotate`
- `cap(all)`
- grammar expressions evaluated before mesh generation

Recommended P0 restrictions reflected by the examples:

- one closed outer loop only
- no holes
- matching profile vertex correspondence for linear lofting
- containment-safe step transitions
- deterministic mesh construction

`209_coruscant_component24.p3d` is the main real-asset acceptance fixture derived from the prior COLLADA investigation.
