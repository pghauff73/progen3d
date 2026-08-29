# MVP25 150-Section Body Surface Reference Log

Generated on August 23, 2026.

## Construction

Five imagegen atlases provide 150 row-major front-view transverse body silhouettes. The generator extracts each red silhouette into an immutable per-station mask, samples nine semantic vertical levels, enforces bilateral correspondence, and smooths image-derived half-width ratios across neighboring stations. Explicit C1 longitudinal width, height and lower-surface envelope curves remove independent atlas scale drift while retaining the authoritative 4350 x 1800 x 1430 mm vehicle package.

The constant longitudinal step is `4.350 / 149 = 0.029194631 m`. Each station owns one 20-point `Polygon`. All 150 corresponding polygons feed one `SurfaceLoft`, and `ShellOffset` realizes a 0.75 mm visual body skin.

## Authority

- the dimension schedule owns vehicle length, width and height;
- the six-view assembly owns the red sporty AWD hatchback design intent;
- imagegen atlases are visual silhouette evidence only;
- extracted masks and the JSON station ledger make the reconstruction deterministic;
- the lower body datum between stations is an explicit construction inference, not measured reference geometry.

## Limits

This grammar reconstructs the continuous outer body envelope. It intentionally does not infer wheel openings, doors, glazing apertures, shut lines, lamps, grille openings, underbody, seals, weld flanges or hidden body-in-white structure. It is not Class-A, crash, aero or manufacturing certification.

## Artifacts

- grammar: `MVP25_Body_150_Section_Surface.grammar` SHA-256 `f9f0ac63f494b75f60a91a347c7b2641ebfe21cd5261c6f84751d5909fe38e13`;
- manifest: `MVP25_Body_150_Section_Definition.json` SHA-256 `637851cd65a6c925bc0fe24e1128e354a2855025bb31ec6ad0e7dae2c7b2c743`;
- validation command: `./tests/run_mvp25_body_150_section_surface_checks.sh`.
