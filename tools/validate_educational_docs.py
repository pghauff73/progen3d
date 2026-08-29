#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path

from documentation_site import validate_existing_documentation


def main() -> int:
    repository_root = Path(__file__).resolve().parent.parent
    documentation_root = repository_root / "docs"
    validation_result = validate_existing_documentation(
        repository_root, documentation_root
    )
    for finding in validation_result.findings:
        print(
            f"{finding.severity.upper()} {finding.code} "
            f"{finding.path}: {finding.message}"
        )
    print(
        "Documentation validation: "
        f"{validation_result.checks_run} checks, "
        f"{validation_result.error_count} errors, "
        f"{validation_result.warning_count} warnings."
    )
    return 0 if validation_result.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
