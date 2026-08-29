# ProGen3D Educational Documentation System

`docs/index.html` is the GitHub Pages entry point for a fully static educational site.

## Build

From the repository root:

```bash
make docs
```

Equivalent direct command:

```bash
python3 tools/generate_educational_docs.py
```

The generator requires Python 3, `markdown-it-py` (module `markdown_it`), and
Beautiful Soup 4 (module `bs4`). It uses no server-side runtime and does not
download external content.

## Validate

```bash
make docs-check
```

Equivalent commands:

```bash
python3 tools/generate_educational_docs.py --check
python3 tools/validate_educational_docs.py
```

The checks rediscover eligible Markdown, verify 100% source-to-page coverage,
check source hashes and required educational metadata, inspect local links and
anchors, validate image alternatives, validate SVG titles/descriptions and
keyboard-focusable nodes, check shared assets, and compare deterministic output.

## Source discovery

The generator recursively includes authored `.md` and `.markdown` files,
including authored Markdown already under `docs/`. It excludes generated site
output, `docs/README.md`, the legacy `docs/imported-markdown/` mirror, temporary
test output, build output, and dependency/vendor directories.

Current generated coverage: **136 of 136 eligible Markdown files (100%)**.

## Generated ownership

The generator owns only these paths:

- `docs/index.html`
- `docs/glossary.html`
- `docs/markdown-coverage.html`
- `docs/implementation-report.html`
- `docs/manifest.json`
- `docs/README.md`
- `docs/.nojekyll`
- `docs/pages/`
- `docs/assets/css/`
- `docs/assets/js/`
- `docs/assets/diagrams/`
- `docs/assets/images/source/`
- `docs/assets/source-files/`

Authored Markdown such as `docs/SMB_OMv1_SMALL_MODERN_BUILDING.md` and manually
maintained screenshots such as `docs/images/readme/` are preserved. Referenced
source images are copied into the generator-owned image tree for publication.

## Traceability

Every lesson records its source path and SHA-256 digest, renders the complete
source Markdown as semantic HTML, rewrites Markdown links to generated lesson
URLs, and visibly distinguishes proposed, planned, experimental, pending,
failed, passed, and unresolved statements. `docs/manifest.json` is the
machine-readable source-to-output mapping.
