# EPG v1 and SIC v1 (experimental compiler integration)

Baseline: 583c949a47659f685af0562468bc333df964016c. This change integrates the independently tested C++17 graph and solid checks into GrammarDocument::generateGeometry after spatial finalization. Warnings are recorded separately from fatal diagnostics, propagated into SceneGenerationResult, and printed unless quiet diagnostics is enabled. Full editor highlighting and native application build are not verified.

## EPG
EPG001: no reachable compatible source for a recognized motor input. EPG002: research/speculative source only. EPG003: wrong energy direction/domain. EPG004: unresolved port. EPG005: duplicate identifier. The 72-class/14-chemistry catalog, three illustrative P3D files and algebraic physics helpers are in grammar/engineering/energy-power/epg-v1. Native class and interface matching is exact. Supply is topological, not proof of operating energy capacity, return circuits, voltage, pressure, safety or real availability. Unknown legacy motor classes need explicit mapping. The experimental P3E frontend and complete original standalone test package are not installed as native grammar syntax by this commit.

## SIC

- **SIC001** warns when a mesh has boundary edges with a single incident triangle after vertex welding. Open sheets and uncapped surfaces can be intentional. For parts intended to enclose a volume, check omitted caps, missing faces, and mismatched vertices or gaps along seams. The warning identifies the mesh and its grammar source range. See [the uncapped sweep example](../../grammar/engineering/solid-integrity/sic-v1/OpenMeshWarning.p3d).
- **SIC002** reports a non-manifold mesh as an error. **SIC003** reports invalid geometry such as degenerate faces or inconsistent winding as an error.
- **SIC004** warns when an unassigned closed mesh lacks qualifying planar contact. **SIC005** warns when the contact search reaches its work budget and returns incomplete results.

Open surfaces remain available for rendering. Valid closed meshes participate in solid contact analysis. A procedural shape that explicitly requires closure still enforces it. The contact test compares opposite unit normals with `norm(nA+nB) < 0.01`, relative plane slope at most 0.005, and planar projected intersection of at least 0.02 of the average total boundary area. Its provisional maximum gap is 0.005 times the smaller mesh diagonal, near-touch tolerance is 1e-6 times the diagonal, and weld tolerance is 1e-8 times the diagonal. Self-intersection detection, collision safety, and structural adequacy require separate checks.

## Validation evidence
The original standalone packages were rerun locally before this change: 42 EPG tests, 28 SIC test groups (including 300 analytical overlap cases), and four SIC installer tests passed. The two saved logs here are standalone evidence, not CI evidence for this integration. No full application build was possible because the execution environment could not clone GitHub. Test native build, geometry fixtures, warning presentation and the existing grammar corpus before release. No Meridian routing changes are included.
