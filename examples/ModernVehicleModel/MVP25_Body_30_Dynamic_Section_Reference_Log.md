# MVP25 30-Section Dynamic Body Surface Reference Log

Generated on August 24, 2026.

## Construction

One imagegen atlas provides 30 row-major front-view transverse body silhouettes. The generator extracts each red silhouette into an immutable mask and samples nine semantic vertical levels. A `VehicleEnvelopeSamplingDensity` combines width, roof-height and lower-surface rates with explicit front-bumper, bonnet-rise, pillar, hatch and tail transition boosts. `DynamicBodySectionSchedule` integrates that density and places 30 stations at equal density measure rather than equal physical distance.

The resulting 29 steps range from `0.046659616 m` to `0.303567364 m`, with average `0.150000000 m`. Each station owns one 20-point `Polygon`; all 30 polygons feed one `SurfaceLoft`, and `ShellOffset` realizes a 0.75 mm visual body skin.

## Authority

- the dimension schedule owns vehicle length, width and height;
- the six-view assembly owns the red sporty AWD hatchback design intent;
- imagegen silhouettes are visual evidence only;
- adaptive density is an explicit reproducible construction heuristic;
- the JSON station ledger owns every variable step, mask hash and profile point.

## Limits

This grammar reconstructs a continuous outer body envelope. It does not infer wheel openings, doors, glazing apertures, shut lines, lamps, grille openings, underbody, seals, weld flanges, drivetrain or hidden body-in-white structure. AWD remains a semantic package classification. The result is not Class-A, crash, aero or manufacturing certification.

## Artifacts

- grammar: `MVP25_Body_30_Dynamic_Section_Surface.grammar` SHA-256 `213b7a3729f235547d9a14376fe396506bba72adc029d325ea01aed697a760fe`;
- manifest: `MVP25_Body_30_Dynamic_Section_Definition.json` SHA-256 `47d8dcc59bcc9ab3dd216d6de1e21593f486b5b26e717b525bacff53ccffbd80`;
- validation command: `./tests/run_mvp25_body_30_dynamic_section_surface_checks.sh`.
