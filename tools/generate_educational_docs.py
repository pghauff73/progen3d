#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from documentation_site import EducationalDocumentationGenerationService


def parse_arguments() -> argparse.Namespace:
    argument_parser = argparse.ArgumentParser(
        description="Generate the complete ProGen3D educational documentation site."
    )
    argument_parser.add_argument(
        "--check",
        action="store_true",
        help="Generate in a temporary directory and fail if committed output differs.",
    )
    return argument_parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    repository_root = Path(__file__).resolve().parent.parent
    documentation_root = repository_root / "docs"
    generation_service = EducationalDocumentationGenerationService()

    if arguments.check:
        differences = generation_service.check(repository_root, documentation_root)
        if differences:
            print("Documentation output is not deterministic/current:")
            for difference in differences:
                print(f"- {difference}")
            return 1
        print("Documentation output matches deterministic regeneration.")
        return 0

    build_result = generation_service.generate(repository_root, documentation_root)
    print(
        "Generated ProGen3D educational documentation: "
        f"{len(build_result.source_documents)} Markdown sources, "
        f"{len(build_result.source_documents)} lessons, "
        f"{len(build_result.diagram_paths)} SVG diagrams, "
        f"{build_result.validation_result.checks_run} validation checks."
    )
    unresolved_issues = [
        issue
        for issue in build_result.link_issues
        if issue.issue_kind != "relocated-reference"
    ]
    relocated_references = [
        issue
        for issue in build_result.link_issues
        if issue.issue_kind == "relocated-reference"
    ]
    if unresolved_issues:
        print(
            f"Recorded {len(unresolved_issues)} unresolved source "
            "references in docs/markdown-coverage.html."
        )
    if relocated_references:
        print(
            f"Resolved {len(relocated_references)} stale source paths to "
            "unique project files and recorded the relocations."
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
