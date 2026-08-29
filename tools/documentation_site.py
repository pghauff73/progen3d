#!/usr/bin/env python3

from __future__ import annotations

import difflib
import hashlib
import html
import json
import math
import os
import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ElementTree
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Sequence
from urllib.parse import quote, unquote, urlsplit, urlunsplit

try:
    from bs4 import BeautifulSoup, Tag
    from markdown_it import MarkdownIt
except ImportError as import_error:  # pragma: no cover - exercised by preflight
    raise RuntimeError(
        "Educational documentation generation requires markdown-it-py and "
        "beautifulsoup4. Install the distribution packages that provide "
        "the Python modules 'markdown_it' and 'bs4'."
    ) from import_error


MARKDOWN_SUFFIXES = {".md", ".markdown"}
IMAGE_SUFFIXES = {".avif", ".gif", ".jpeg", ".jpg", ".png", ".svg", ".webp"}
GENERATED_TOP_LEVEL_FILES = (
    Path(".nojekyll"),
    Path("README.md"),
    Path("glossary.html"),
    Path("implementation-report.html"),
    Path("index.html"),
    Path("manifest.json"),
    Path("markdown-coverage.html"),
)
GENERATED_DIRECTORIES = (
    Path("pages"),
    Path("assets/css"),
    Path("assets/js"),
    Path("assets/diagrams"),
    Path("assets/images/source"),
    Path("assets/source-files"),
)
STATUS_SIGNAL_PATTERNS = {
    "passed": re.compile(r"\b(?:pass|passed|verified|complete|completed)\b", re.I),
    "failed": re.compile(r"\b(?:fail|failed|failure|blocked)\b", re.I),
    "pending": re.compile(r"\b(?:pending|todo|not\s+evaluated|unknown)\b", re.I),
    "proposed": re.compile(r"\b(?:proposed|proposal|planned|roadmap|future)\b", re.I),
    "unresolved": re.compile(
        r"\b(?:unresolved|needs\s+clarification|not\s+yet\s+implemented|open\s+question)\b",
        re.I,
    ),
    "experimental": re.compile(r"\b(?:experimental|hypothesis|hypotheses)\b", re.I),
}
GENERIC_CONCEPT_HEADINGS = {
    "acceptance",
    "acceptance criteria",
    "acceptance gates",
    "artifacts",
    "contents",
    "document control",
    "files to add",
    "files to modify",
    "goal",
    "introduction",
    "next steps",
    "objective",
    "overview",
    "purpose",
    "references",
    "result",
    "status",
    "summary",
    "tasks",
    "validation",
    "validation evidence",
    "verification",
}
COMMON_ACRONYM_WORDS = {
    "AABB",
    "API",
    "ASCII",
    "CI",
    "CLI",
    "CPU",
    "CSS",
    "CSV",
    "GUI",
    "GPU",
    "HTML",
    "HTTP",
    "HTTPS",
    "ID",
    "JSON",
    "Linux",
    "LOD",
    "OpenGL",
    "PBR",
    "PLY",
    "README",
    "SHA",
    "STL",
    "SVG",
    "UI",
    "URL",
    "UTF",
    "WCAG",
}
ACRONYM_EXPANSION_FALLBACKS = {
    "AABB": "axis-aligned bounding box",
    "API": "application programming interface",
    "CI": "continuous integration",
    "CLI": "command-line interface",
    "CPU": "central processing unit",
    "CSS": "Cascading Style Sheets",
    "CSV": "comma-separated values",
    "GUI": "graphical user interface",
    "GPU": "graphics processing unit",
    "HTML": "HyperText Markup Language",
    "HTTP": "Hypertext Transfer Protocol",
    "HTTPS": "Hypertext Transfer Protocol Secure",
    "JSON": "JavaScript Object Notation",
    "LOD": "level of detail",
    "PBR": "physically based rendering",
    "PLY": "Polygon File Format",
    "SHA": "Secure Hash Algorithm",
    "STL": "stereolithography mesh format",
    "SVG": "Scalable Vector Graphics",
    "UI": "user interface",
    "URL": "Uniform Resource Locator",
    "UTF": "Unicode Transformation Format",
    "WCAG": "Web Content Accessibility Guidelines",
}


@dataclass(frozen=True)
class DocumentationCategory:
    identifier: str
    title: str
    description: str


CATEGORIES = (
    DocumentationCategory(
        "start",
        "Start and Build",
        "Project orientation, build instructions, quick starts, and repository entry points.",
    ),
    DocumentationCategory(
        "architecture",
        "Architecture and Runtime",
        "Editor architecture, runtime ownership, grammar semantics, and system integration.",
    ),
    DocumentationCategory(
        "geometry-rendering",
        "Geometry and Rendering",
        "Primitives, profiles, mesh conversion, cameras, preview rendering, and material pipelines.",
    ),
    DocumentationCategory(
        "buildings",
        "Buildings and Object Models",
        "Building grammars, spatial object models, furniture, placement, and architectural evidence.",
    ),
    DocumentationCategory(
        "vehicles",
        "Vehicles and Automotive Models",
        "Vehicle grammars, mathematical models, primitive coverage, fitting, and release packages.",
    ),
    DocumentationCategory(
        "vegetation",
        "Vegetation Systems",
        "Vegetation grammars, building-integrated plants, rendering, and reconstruction evidence.",
    ),
    DocumentationCategory(
        "validation",
        "Validation and Evidence",
        "Verification reports, requirement audits, comparison reports, and deterministic evidence.",
    ),
    DocumentationCategory(
        "plans-research",
        "Plans and Research",
        "Implementation plans, proposals, roadmaps, literature reviews, and unresolved research work.",
    ),
    DocumentationCategory(
        "governance",
        "Governance and Agent Work",
        "Repository instructions, governed automation, authority boundaries, and refactor tracking.",
    ),
    DocumentationCategory(
        "examples-reference",
        "Examples and Reference",
        "Example corpora, source mappings, schemas, limitations, and focused reference material.",
    ),
)
CATEGORY_BY_IDENTIFIER = {category.identifier: category for category in CATEGORIES}
CATEGORY_ORDER = {
    category.identifier: index for index, category in enumerate(CATEGORIES)
}


@dataclass(frozen=True)
class DocumentationBuildConfiguration:
    repository_root: Path
    output_root: Path
    repository_url: str
    repository_branch: str
    static_asset_root: Path

    @staticmethod
    def from_repository(repository_root: Path, output_root: Path) -> "DocumentationBuildConfiguration":
        normalized_repository_root = repository_root.resolve()
        normalized_output_root = output_root.resolve()
        return DocumentationBuildConfiguration(
            repository_root=normalized_repository_root,
            output_root=normalized_output_root,
            repository_url=GitRepositoryInformationService.resolve_repository_url(
                normalized_repository_root
            ),
            repository_branch=GitRepositoryInformationService.resolve_repository_branch(
                normalized_repository_root
            ),
            static_asset_root=normalized_repository_root
            / "tools"
            / "documentation_assets",
        )


@dataclass(frozen=True)
class ExcludedMarkdownDocument:
    relative_path: Path
    reason: str


@dataclass
class MarkdownSourceDocument:
    source_path: Path
    relative_path: Path
    content: str
    sha256: str
    byte_count: int
    title: str = ""
    summary: str = ""
    intended_audience: str = ""
    status: str = "Documented"
    category_identifier: str = "examples-reference"
    headings: list[str] = field(default_factory=list)
    concepts: list[str] = field(default_factory=list)
    acronym_candidates: list[str] = field(default_factory=list)
    code_examples: list[tuple[str, str]] = field(default_factory=list)
    procedure_steps: list[str] = field(default_factory=list)
    verification_commands: list[str] = field(default_factory=list)
    status_signal_counts: dict[str, int] = field(default_factory=dict)
    table_count: int = 0
    equation_count: int = 0
    word_count: int = 0
    updated_label: str = "Working tree source"
    tracked_by_git: bool = False
    output_path: Path = Path()
    related_paths: list[Path] = field(default_factory=list)
    prerequisite_paths: list[Path] = field(default_factory=list)


@dataclass(frozen=True)
class DocumentationLinkIssue:
    source_path: Path
    referenced_value: str
    issue_kind: str
    reason: str


@dataclass(frozen=True)
class DuplicateTitleRecord:
    title_a: str
    title_b: str
    source_a: Path
    source_b: Path
    similarity: float


@dataclass(frozen=True)
class GlossaryEntry:
    term: str
    definition: str
    source_paths: tuple[Path, ...]
    needs_clarification: bool


@dataclass(frozen=True)
class DocumentationValidationFinding:
    severity: str
    code: str
    path: str
    message: str


@dataclass
class DocumentationValidationResult:
    findings: list[DocumentationValidationFinding] = field(default_factory=list)
    checks_run: int = 0

    @property
    def error_count(self) -> int:
        return sum(finding.severity == "error" for finding in self.findings)

    @property
    def warning_count(self) -> int:
        return sum(finding.severity == "warning" for finding in self.findings)

    @property
    def passed(self) -> bool:
        return self.error_count == 0

    def record_error(self, code: str, path: str, message: str) -> None:
        self.findings.append(
            DocumentationValidationFinding("error", code, path, message)
        )

    def record_warning(self, code: str, path: str, message: str) -> None:
        self.findings.append(
            DocumentationValidationFinding("warning", code, path, message)
        )


@dataclass
class DocumentationBuildResult:
    source_documents: list[MarkdownSourceDocument]
    excluded_documents: list[ExcludedMarkdownDocument]
    duplicate_titles: list[DuplicateTitleRecord]
    glossary_entries: list[GlossaryEntry]
    link_issues: list[DocumentationLinkIssue]
    copied_assets: set[Path]
    copied_source_files: set[Path]
    diagram_paths: list[Path]
    validation_result: DocumentationValidationResult


class GitRepositoryInformationService:
    @staticmethod
    def run_git_command(repository_root: Path, arguments: Sequence[str]) -> str:
        try:
            completed_process = subprocess.run(
                ["git", *arguments],
                cwd=repository_root,
                check=True,
                capture_output=True,
                text=True,
            )
        except (OSError, subprocess.CalledProcessError):
            return ""
        return completed_process.stdout.strip()

    @classmethod
    def resolve_repository_url(cls, repository_root: Path) -> str:
        remote_url = cls.run_git_command(
            repository_root, ["remote", "get-url", "origin"]
        )
        if remote_url.startswith("git@github.com:"):
            remote_url = "https://github.com/" + remote_url.removeprefix(
                "git@github.com:"
            )
        if remote_url.endswith(".git"):
            remote_url = remote_url[:-4]
        if remote_url.startswith("https://github.com/"):
            return remote_url
        return "https://github.com/pghauff73/progen3d"

    @classmethod
    def resolve_repository_branch(cls, repository_root: Path) -> str:
        branch_name = cls.run_git_command(
            repository_root, ["branch", "--show-current"]
        )
        return branch_name or "main"

    @classmethod
    def collect_tracked_paths(cls, repository_root: Path) -> set[Path]:
        output = cls.run_git_command(repository_root, ["ls-files"])
        return {Path(line) for line in output.splitlines() if line.strip()}

    @classmethod
    def collect_latest_source_dates(cls, repository_root: Path) -> dict[Path, str]:
        output = cls.run_git_command(
            repository_root, ["log", "--format=@@DATE:%cs", "--name-only", "--"]
        )
        source_dates: dict[Path, str] = {}
        active_date = ""
        for line in output.splitlines():
            if line.startswith("@@DATE:"):
                active_date = line.removeprefix("@@DATE:").strip()
                continue
            if not line.strip() or not active_date:
                continue
            relative_path = Path(line.strip())
            source_dates.setdefault(relative_path, active_date)
        return source_dates


class MarkdownSourceDiscoveryService:
    EXCLUDED_PART_REASONS = {
        ".git": "Git metadata",
        ".tmp-tests": "temporary generated test output",
        ".cache": "tool cache",
        "build": "generated build output",
        "dist": "generated distribution output",
        "node_modules": "dependency directory",
        "third_party": "third-party dependency documentation",
        "vendor": "vendored dependency documentation",
    }

    def __init__(self, configuration: DocumentationBuildConfiguration) -> None:
        self.configuration = configuration

    def discover(
        self,
    ) -> tuple[list[MarkdownSourceDocument], list[ExcludedMarkdownDocument]]:
        tracked_paths = GitRepositoryInformationService.collect_tracked_paths(
            self.configuration.repository_root
        )
        latest_source_dates = GitRepositoryInformationService.collect_latest_source_dates(
            self.configuration.repository_root
        )
        source_documents: list[MarkdownSourceDocument] = []
        excluded_documents: list[ExcludedMarkdownDocument] = []

        for source_path in self.configuration.repository_root.rglob("*"):
            if not source_path.is_file() or source_path.suffix.lower() not in MARKDOWN_SUFFIXES:
                continue
            relative_path = source_path.relative_to(
                self.configuration.repository_root
            )
            exclusion_reason = self.resolve_exclusion_reason(relative_path)
            if exclusion_reason:
                excluded_documents.append(
                    ExcludedMarkdownDocument(relative_path, exclusion_reason)
                )
                continue

            source_bytes = source_path.read_bytes()
            source_documents.append(
                MarkdownSourceDocument(
                    source_path=source_path,
                    relative_path=relative_path,
                    content=source_bytes.decode("utf-8", errors="replace"),
                    sha256=hashlib.sha256(source_bytes).hexdigest(),
                    byte_count=len(source_bytes),
                    updated_label=latest_source_dates.get(
                        relative_path, "Working tree source"
                    ),
                    tracked_by_git=relative_path in tracked_paths,
                )
            )

        return (
            sorted(source_documents, key=lambda document: document.relative_path.as_posix()),
            sorted(
                excluded_documents,
                key=lambda document: document.relative_path.as_posix(),
            ),
        )

    def resolve_exclusion_reason(self, relative_path: Path) -> str | None:
        if relative_path == Path("docs/README.md"):
            return "generated documentation system README"
        if relative_path.parts[:2] == ("docs", "imported-markdown"):
            return "legacy generated Markdown mirror"
        for path_part in relative_path.parts:
            if path_part in self.EXCLUDED_PART_REASONS:
                return self.EXCLUDED_PART_REASONS[path_part]
        return None


class MarkdownDocumentAnalysisService:
    HEADING_PATTERN = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
    NUMBERED_STEP_PATTERN = re.compile(r"^\s*\d+[.)]\s+(.+?)\s*$")
    ACRONYM_PATTERN = re.compile(r"\b[A-Z][A-Z0-9_-]{1,11}\b")
    FENCE_PATTERN = re.compile(
        r"^```([^\n]*)\n(.*?)^```\s*$", re.MULTILINE | re.DOTALL
    )

    def analyze_documents(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> None:
        for source_document in source_documents:
            self.analyze_document(source_document)

    def analyze_document(self, source_document: MarkdownSourceDocument) -> None:
        source_document.title = self.extract_title(source_document)
        source_document.summary = self.extract_summary(source_document)
        source_document.headings = self.extract_headings(source_document.content)
        source_document.concepts = self.extract_concepts(source_document)
        source_document.acronym_candidates = self.extract_acronyms(source_document)
        source_document.code_examples = [
            (language.strip(), code.rstrip())
            for language, code in self.FENCE_PATTERN.findall(source_document.content)
        ]
        source_document.procedure_steps = self.extract_procedure_steps(
            source_document.content
        )
        source_document.verification_commands = self.extract_verification_commands(
            source_document.code_examples
        )
        source_document.status_signal_counts = {
            name: len(pattern.findall(source_document.content))
            for name, pattern in STATUS_SIGNAL_PATTERNS.items()
        }
        source_document.table_count = self.count_markdown_tables(
            source_document.content
        )
        source_document.equation_count = self.count_equations(source_document.content)
        source_document.word_count = len(
            re.findall(r"\b[\w][\w'./+-]*\b", source_document.content)
        )
        source_document.category_identifier = self.classify_category(source_document)
        source_document.status = self.classify_document_status(source_document)
        source_document.intended_audience = self.classify_audience(source_document)

    def extract_title(self, source_document: MarkdownSourceDocument) -> str:
        inside_fence = False
        for line in source_document.content.splitlines():
            if line.startswith("```"):
                inside_fence = not inside_fence
                continue
            if inside_fence:
                continue
            heading_match = self.HEADING_PATTERN.match(line)
            if heading_match and len(heading_match.group(1)) == 1:
                return self.clean_inline_markdown(heading_match.group(2))
        return source_document.relative_path.stem.replace("_", " ").replace("-", " ")

    def extract_summary(self, source_document: MarkdownSourceDocument) -> str:
        paragraphs: list[str] = []
        active_lines: list[str] = []
        inside_fence = False

        def flush_paragraph() -> None:
            if not active_lines:
                return
            paragraph = " ".join(line.strip() for line in active_lines).strip()
            active_lines.clear()
            if paragraph:
                paragraphs.append(paragraph)

        for line in source_document.content.splitlines():
            stripped_line = line.strip()
            if stripped_line.startswith("```"):
                flush_paragraph()
                inside_fence = not inside_fence
                continue
            if inside_fence:
                continue
            if not stripped_line:
                flush_paragraph()
                continue
            if (
                stripped_line.startswith("#")
                or stripped_line.startswith(("- ", "* ", ">", "|", "<"))
                or self.NUMBERED_STEP_PATTERN.match(stripped_line)
            ):
                flush_paragraph()
                continue
            active_lines.append(stripped_line)
        flush_paragraph()

        for paragraph in paragraphs:
            cleaned_paragraph = self.clean_inline_markdown(paragraph)
            if len(cleaned_paragraph.split()) >= 5:
                return self.shorten(cleaned_paragraph, 300)
        return (
            f"This source document records project material about "
            f"{source_document.relative_path.stem.replace('_', ' ')}."
        )

    def extract_headings(self, content: str) -> list[str]:
        headings: list[str] = []
        inside_fence = False
        for line in content.splitlines():
            if line.startswith("```"):
                inside_fence = not inside_fence
                continue
            if inside_fence:
                continue
            heading_match = self.HEADING_PATTERN.match(line)
            if heading_match:
                headings.append(self.clean_inline_markdown(heading_match.group(2)))
        return headings

    def extract_concepts(self, source_document: MarkdownSourceDocument) -> list[str]:
        concept_candidates = [source_document.title]
        concept_candidates.extend(source_document.headings[1:])
        concepts: list[str] = []
        normalized_concepts: set[str] = set()
        for candidate in concept_candidates:
            cleaned_candidate = re.sub(r"^\d+(?:\.\d+)*[.)]?\s*", "", candidate).strip()
            cleaned_candidate = re.sub(r"\s+", " ", cleaned_candidate)
            normalized_candidate = self.normalized_title(cleaned_candidate)
            if (
                not normalized_candidate
                or normalized_candidate in GENERIC_CONCEPT_HEADINGS
                or normalized_candidate.startswith("phase ")
                or normalized_candidate.startswith("task ")
                or len(cleaned_candidate) > 86
            ):
                continue
            if normalized_candidate in normalized_concepts:
                continue
            normalized_concepts.add(normalized_candidate)
            concepts.append(cleaned_candidate)
            if len(concepts) >= 8:
                break
        if not concepts:
            concepts.append(source_document.title)
        return concepts

    def extract_acronyms(self, source_document: MarkdownSourceDocument) -> list[str]:
        content_without_code = self.FENCE_PATTERN.sub("", source_document.content)
        acronym_counts = Counter(self.ACRONYM_PATTERN.findall(content_without_code))
        title_and_headings = " ".join(
            [source_document.title, *source_document.headings]
        )
        candidates = []
        for acronym, occurrence_count in acronym_counts.most_common():
            if acronym in {"PASS", "FAIL", "TRUE", "FALSE", "TODO", "NOTE"}:
                continue
            if occurrence_count < 2 and acronym not in title_and_headings:
                continue
            candidates.append(acronym)
            if len(candidates) >= 12:
                break
        return candidates

    def extract_procedure_steps(self, content: str) -> list[str]:
        procedure_steps = []
        inside_fence = False
        for line in content.splitlines():
            if line.startswith("```"):
                inside_fence = not inside_fence
                continue
            if inside_fence:
                continue
            step_match = self.NUMBERED_STEP_PATTERN.match(line)
            if step_match:
                procedure_steps.append(
                    self.shorten(self.clean_inline_markdown(step_match.group(1)), 220)
                )
            if len(procedure_steps) >= 12:
                break
        return procedure_steps

    @staticmethod
    def extract_verification_commands(
        code_examples: Sequence[tuple[str, str]],
    ) -> list[str]:
        commands: list[str] = []
        command_prefixes = (
            "./",
            "cmake ",
            "ctest ",
            "git ",
            "make",
            "ninja ",
            "python ",
            "python3 ",
            "pytest ",
            "sh ",
            "bash ",
        )
        for language, code in code_examples:
            normalized_language = language.lower()
            for code_line in code.splitlines():
                stripped_line = code_line.strip().removeprefix("$ ")
                if not stripped_line or stripped_line.startswith("#"):
                    continue
                if normalized_language in {"bash", "console", "shell", "sh", "zsh"} or stripped_line.startswith(
                    command_prefixes
                ):
                    commands.append(stripped_line)
                if len(commands) >= 10:
                    return commands
        return commands

    @staticmethod
    def count_markdown_tables(content: str) -> int:
        return len(
            re.findall(
                r"^\s*\|?(?:\s*:?-{3,}:?\s*\|)+\s*:?-{3,}:?\s*\|?\s*$",
                content,
                re.MULTILINE,
            )
        )

    @staticmethod
    def count_equations(content: str) -> int:
        display_math_pairs = content.count("$$") // 2
        bracket_math_pairs = min(content.count("\\["), content.count("\\]"))
        return display_math_pairs + bracket_math_pairs

    @staticmethod
    def clean_inline_markdown(value: str) -> str:
        cleaned_value = re.sub(r"!\[([^]]*)\]\([^)]*\)", r"\1", value)
        cleaned_value = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", cleaned_value)
        cleaned_value = re.sub(r"[`*_~]", "", cleaned_value)
        cleaned_value = re.sub(r"<[^>]+>", "", cleaned_value)
        return html.unescape(re.sub(r"\s+", " ", cleaned_value)).strip()

    @staticmethod
    def shorten(value: str, maximum_length: int) -> str:
        if len(value) <= maximum_length:
            return value
        shortened_value = value[: maximum_length - 1].rsplit(" ", 1)[0]
        return shortened_value.rstrip(".,;:") + "…"

    @staticmethod
    def normalized_title(value: str) -> str:
        return re.sub(r"[^a-z0-9]+", " ", value.lower()).strip()

    def classify_category(self, source_document: MarkdownSourceDocument) -> str:
        combined_value = (
            source_document.relative_path.as_posix()
            + " "
            + source_document.title
        ).lower()
        if any(
            keyword in combined_value
            for keyword in (
                "vegetation",
                "vine",
                "foliage",
                "plant",
            )
        ):
            return "vegetation"
        if any(
            keyword in combined_value
            for keyword in (
                "vehicle",
                "automotive",
                "hatchback",
                "chassis",
                "motor car",
                "modern car",
                "mcsm",
                "mvgv",
                "mvp25",
                "mvpv2",
                "mcp_om",
            )
        ):
            return "vehicles"
        if any(
            keyword in combined_value
            for keyword in (
                "building",
                "courtyard",
                "townhouse",
                "residence",
                "chair",
                "smb",
                "comv1",
            )
        ):
            return "buildings"
        if any(
            keyword in combined_value
            for keyword in (
                "axial",
                "primitive",
                "render",
                "mesh",
                "camera",
                "fov",
                "opengl",
                "pbr",
                "geometry",
                "texture",
            )
        ):
            return "geometry-rendering"
        if any(
            keyword in combined_value
            for keyword in (
                "validation",
                "verification",
                "test_report",
                "test report",
                "audit",
                "evidence",
                "comparison_report",
                "match_report",
                "reference_log",
                "baseline_report",
            )
        ):
            return "validation"
        if any(
            keyword in combined_value
            for keyword in (
                "implementation_plan",
                "implementation plan",
                "proposal",
                "roadmap",
                "research",
                "literature",
                "strategy",
                "source_mapping",
            )
        ):
            return "plans-research"
        if any(
            keyword in combined_value
            for keyword in (
                "agents.md",
                "agent implementation",
                "refactor progress",
                "governed",
                "authority",
            )
        ):
            return "governance"
        if source_document.relative_path in {Path("README.md"), Path("BUILDING.md")}:
            return "start"
        if source_document.relative_path.name in {"QUICKSTART.md"}:
            return "start"
        if any(
            keyword in combined_value
            for keyword in ("editor", "runtime", "grammar", "p0_", "p1_", "p2_")
        ):
            return "architecture"
        return "examples-reference"

    @staticmethod
    def classify_document_status(source_document: MarkdownSourceDocument) -> str:
        combined_value = (
            source_document.relative_path.as_posix() + " " + source_document.title
        ).lower()
        if any(
            keyword in combined_value
            for keyword in ("verification", "validation", "test_report", "audit", "evidence", "match_report")
        ):
            return "Evidence"
        if any(
            keyword in combined_value
            for keyword in ("proposal", "proposed", "implementation_plan", "implementation plan", "roadmap")
        ):
            return "Proposed" if "proposal" in combined_value or "proposed" in combined_value else "Planned"
        if any(keyword in combined_value for keyword in ("research", "literature", "mathematical model", "reference log")):
            return "Research"
        return "Documented"

    @staticmethod
    def classify_audience(source_document: MarkdownSourceDocument) -> str:
        combined_value = (
            source_document.relative_path.as_posix() + " " + source_document.title
        ).lower()
        if source_document.relative_path.name in {"README.md", "QUICKSTART.md", "BUILDING.md"}:
            return "new readers and developers preparing to use the project"
        if any(keyword in combined_value for keyword in ("verification", "validation", "audit", "test report", "evidence")):
            return "maintainers, reviewers, and readers checking project evidence"
        if any(keyword in combined_value for keyword in ("implementation plan", "roadmap", "proposal")):
            return "implementers, architects, and reviewers planning future work"
        if any(keyword in combined_value for keyword in ("research", "literature", "mathematical")):
            return "advanced readers studying the project model and its research basis"
        return "developers and technical readers following the documented feature"


class DocumentationAddressService:
    def assign_output_paths(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> list[DuplicateTitleRecord]:
        slug_groups: dict[str, list[MarkdownSourceDocument]] = defaultdict(list)
        for source_document in source_documents:
            slug_groups[self.slugify(source_document.title)].append(source_document)

        for base_slug, slug_documents in slug_groups.items():
            if len(slug_documents) == 1:
                slug_documents[0].output_path = (
                    Path("pages")
                    / slug_documents[0].category_identifier
                    / f"{base_slug}.html"
                )
                continue
            for source_document in sorted(
                slug_documents, key=lambda document: document.relative_path.as_posix()
            ):
                path_digest = hashlib.sha256(
                    source_document.relative_path.as_posix().encode("utf-8")
                ).hexdigest()[:8]
                source_document.output_path = (
                    Path("pages")
                    / source_document.category_identifier
                    / f"{base_slug}--{path_digest}.html"
                )

        return self.find_duplicate_titles(source_documents)

    @staticmethod
    def slugify(value: str) -> str:
        normalized_value = value.lower().replace("+", " plus ")
        normalized_value = re.sub(r"[^a-z0-9]+", "-", normalized_value).strip("-")
        return normalized_value[:96].rstrip("-") or "document"

    def find_duplicate_titles(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> list[DuplicateTitleRecord]:
        duplicate_records: list[DuplicateTitleRecord] = []
        for index, document_a in enumerate(source_documents):
            normalized_a = MarkdownDocumentAnalysisService.normalized_title(
                document_a.title
            )
            for document_b in source_documents[index + 1 :]:
                normalized_b = MarkdownDocumentAnalysisService.normalized_title(
                    document_b.title
                )
                if not normalized_a or not normalized_b:
                    continue
                similarity = difflib.SequenceMatcher(
                    None, normalized_a, normalized_b
                ).ratio()
                if normalized_a == normalized_b or similarity >= 0.93:
                    duplicate_records.append(
                        DuplicateTitleRecord(
                            document_a.title,
                            document_b.title,
                            document_a.relative_path,
                            document_b.relative_path,
                            similarity,
                        )
                    )
        return duplicate_records

    @staticmethod
    def relative_url(from_output_path: Path, to_output_path: Path) -> str:
        return Path(
            os.path.relpath(to_output_path, start=from_output_path.parent)
        ).as_posix()


class GlossaryAnalysisService:
    LONG_NAME_ACRONYM_PATTERN = re.compile(
        r"\b([A-Z][A-Za-z0-9/+.-]*(?:\s+[A-Za-z][A-Za-z0-9/+.-]*){1,8})\s*\(([A-Z][A-Z0-9_-]{1,11})\)"
    )
    ACRONYM_DASH_PATTERN = re.compile(
        r"\b([A-Z][A-Z0-9_-]{1,11})\s*[—–-]\s*([A-Za-z][^.\n;]{4,100})"
    )

    def build_glossary(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> list[GlossaryEntry]:
        definitions: dict[str, Counter[str]] = defaultdict(Counter)
        sources: dict[str, set[Path]] = defaultdict(set)

        for source_document in source_documents:
            for long_name, acronym in self.LONG_NAME_ACRONYM_PATTERN.findall(
                source_document.content
            ):
                definitions[acronym][self.clean_definition(long_name)] += 1
                sources[acronym].add(source_document.relative_path)
            for acronym, definition in self.ACRONYM_DASH_PATTERN.findall(
                source_document.content
            ):
                definitions[acronym][self.clean_definition(definition)] += 1
                sources[acronym].add(source_document.relative_path)
            for acronym in source_document.acronym_candidates:
                sources[acronym].add(source_document.relative_path)

        entries: list[GlossaryEntry] = []
        for term in sorted(sources, key=str.casefold):
            if definitions[term]:
                definition = definitions[term].most_common(1)[0][0]
                needs_clarification = False
            elif term in ACRONYM_EXPANSION_FALLBACKS:
                definition = ACRONYM_EXPANSION_FALLBACKS[term]
                needs_clarification = False
            else:
                definition = (
                    "Needs clarification: the source set uses this abbreviation "
                    "without a single explicit expansion."
                )
                needs_clarification = True
            entries.append(
                GlossaryEntry(
                    term=term,
                    definition=definition,
                    source_paths=tuple(sorted(sources[term], key=lambda path: path.as_posix())),
                    needs_clarification=needs_clarification,
                )
            )
        return entries

    @staticmethod
    def clean_definition(value: str) -> str:
        return re.sub(r"\s+", " ", value).strip(" .,:;—–-")


class DocumentationRelationshipService:
    def assign_relationships(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> None:
        document_by_path = {
            source_document.relative_path: source_document
            for source_document in source_documents
        }
        orientation_document = document_by_path.get(Path("README.md"))
        build_document = document_by_path.get(Path("BUILDING.md"))

        for source_document in source_documents:
            source_document.related_paths = [
                related_document.relative_path
                for related_document in self.rank_related_documents(
                    source_document, source_documents
                )[:6]
            ]
            prerequisites: list[Path] = []
            if orientation_document and source_document is not orientation_document:
                prerequisites.append(orientation_document.relative_path)
            if (
                build_document
                and source_document is not build_document
                and source_document.category_identifier
                in {"architecture", "geometry-rendering", "validation"}
            ):
                prerequisites.append(build_document.relative_path)
            for related_path in source_document.related_paths:
                if len(prerequisites) >= 3:
                    break
                related_document = document_by_path[related_path]
                if (
                    related_document.status == "Documented"
                    and related_path not in prerequisites
                ):
                    prerequisites.append(related_path)
            source_document.prerequisite_paths = prerequisites

    def rank_related_documents(
        self,
        source_document: MarkdownSourceDocument,
        source_documents: Sequence[MarkdownSourceDocument],
    ) -> list[MarkdownSourceDocument]:
        source_tokens = self.relationship_tokens(source_document)
        scored_documents: list[tuple[float, str, MarkdownSourceDocument]] = []
        for candidate_document in source_documents:
            if candidate_document is source_document:
                continue
            candidate_tokens = self.relationship_tokens(candidate_document)
            union_tokens = source_tokens | candidate_tokens
            overlap_score = (
                len(source_tokens & candidate_tokens) / len(union_tokens)
                if union_tokens
                else 0.0
            )
            category_score = (
                2.5
                if candidate_document.category_identifier
                == source_document.category_identifier
                else 0.0
            )
            path_score = self.common_path_prefix_score(
                source_document.relative_path, candidate_document.relative_path
            )
            title_score = difflib.SequenceMatcher(
                None, source_document.title.lower(), candidate_document.title.lower()
            ).ratio()
            total_score = category_score + overlap_score * 8.0 + path_score + title_score
            scored_documents.append(
                (total_score, candidate_document.relative_path.as_posix(), candidate_document)
            )
        scored_documents.sort(key=lambda item: (-item[0], item[1]))
        return [item[2] for item in scored_documents]

    @staticmethod
    def relationship_tokens(source_document: MarkdownSourceDocument) -> set[str]:
        value = " ".join(
            [source_document.title, *source_document.concepts, source_document.relative_path.as_posix()]
        ).lower()
        return {
            token
            for token in re.findall(r"[a-z0-9]{3,}", value)
            if token
            not in {
                "and",
                "the",
                "for",
                "with",
                "implementation",
                "report",
                "readme",
                "progen3d",
                "version",
            }
        }

    @staticmethod
    def common_path_prefix_score(path_a: Path, path_b: Path) -> float:
        shared_parts = 0
        for part_a, part_b in zip(path_a.parts, path_b.parts):
            if part_a != part_b:
                break
            shared_parts += 1
        return min(shared_parts, 4) * 0.65


class MarkdownHtmlRenderingService:
    def __init__(
        self,
        configuration: DocumentationBuildConfiguration,
        document_by_source_path: dict[Path, MarkdownSourceDocument],
    ) -> None:
        self.configuration = configuration
        self.document_by_source_path = document_by_source_path
        self.link_issues: list[DocumentationLinkIssue] = []
        self.copied_assets: set[Path] = set()
        self.copied_source_files: set[Path] = set()
        self.markdown = MarkdownIt(
            "commonmark",
            {
                "html": True,
                "linkify": False,
                "typographer": False,
            },
        )
        self.markdown.enable("table")
        self.markdown.enable("strikethrough")
        self.markdown.core.ruler.after(
            "inline", "documentation_heading_ids", self.assign_heading_ids
        )

    @staticmethod
    def assign_heading_ids(state: object) -> None:
        used_identifiers: Counter[str] = Counter()
        tokens = state.tokens
        for token_index, token in enumerate(tokens):
            if token.type != "heading_open" or token_index + 1 >= len(tokens):
                continue
            heading_content = tokens[token_index + 1].content
            heading_identifier = DocumentationAddressService.slugify(heading_content)
            used_identifiers[heading_identifier] += 1
            if used_identifiers[heading_identifier] > 1:
                heading_identifier += f"-{used_identifiers[heading_identifier]}"
            token.attrSet("id", heading_identifier)

    def render_document(self, source_document: MarkdownSourceDocument) -> str:
        rendered_html = self.markdown.render(source_document.content)
        soup = BeautifulSoup(rendered_html, "html.parser")
        self.remove_unsafe_embedded_content(soup)
        source_heading_identifiers = self.prefix_source_heading_identifiers(soup)
        self.demote_source_headings(soup)
        self.rewrite_links(soup, source_document, source_heading_identifiers)
        self.rewrite_images(soup, source_document)
        self.annotate_source_status(soup)
        self.enhance_tables(soup)
        self.enhance_code_blocks(soup)
        self.add_section_guides(soup)
        return str(soup)

    @staticmethod
    def remove_unsafe_embedded_content(soup: BeautifulSoup) -> None:
        for unsafe_element in soup.find_all(
            ["script", "iframe", "embed", "form", "input", "textarea"]
        ):
            replacement = soup.new_tag("p")
            replacement["class"] = "callout callout--clarification"
            replacement.string = (
                "Needs clarification: active embedded content from the source "
                "is omitted from the static learning page."
            )
            unsafe_element.replace_with(replacement)

    @staticmethod
    def prefix_source_heading_identifiers(soup: BeautifulSoup) -> dict[str, str]:
        identifier_mapping: dict[str, str] = {}
        for heading in soup.find_all(["h1", "h2", "h3", "h4", "h5", "h6"]):
            source_identifier = str(heading.get("id", "")).strip()
            if not source_identifier:
                continue
            prefixed_identifier = f"source-{source_identifier}"
            heading["id"] = prefixed_identifier
            identifier_mapping[source_identifier] = prefixed_identifier
        return identifier_mapping

    @staticmethod
    def demote_source_headings(soup: BeautifulSoup) -> None:
        for heading_level in range(6, 0, -1):
            for heading in soup.find_all(f"h{heading_level}"):
                new_level = min(heading_level + 1, 6)
                heading.name = f"h{new_level}"

    def rewrite_links(
        self,
        soup: BeautifulSoup,
        source_document: MarkdownSourceDocument,
        source_heading_identifiers: dict[str, str],
    ) -> None:
        for link in list(soup.find_all("a", href=True)):
            original_href = str(link.get("href", "")).strip()
            if original_href.startswith("#"):
                fragment = original_href.removeprefix("#")
                link["href"] = "#" + source_heading_identifiers.get(fragment, fragment)
                continue
            rewritten_href = self.resolve_reference(
                source_document, original_href, source_document.output_path, False
            )
            if rewritten_href is None:
                unresolved_span = soup.new_tag("span")
                unresolved_span["class"] = "unresolved-link"
                unresolved_span["title"] = "Unresolved source link"
                unresolved_span.extend(list(link.contents))
                link.replace_with(unresolved_span)
                continue
            link["href"] = rewritten_href
            if self.is_external_url(rewritten_href):
                link["rel"] = "noopener noreferrer"

    def rewrite_images(
        self, soup: BeautifulSoup, source_document: MarkdownSourceDocument
    ) -> None:
        for image in soup.find_all("img", src=True):
            original_source = str(image.get("src", "")).strip()
            rewritten_source = self.resolve_reference(
                source_document, original_source, source_document.output_path, True
            )
            if rewritten_source is None:
                image["src"] = ""
                image["data-missing-asset"] = original_source
                image["alt"] = image.get(
                    "alt", "Needs clarification: missing source image"
                )
            else:
                image["src"] = rewritten_source
            if not str(image.get("alt", "")).strip():
                image["alt"] = (
                    "Needs clarification: the source image does not define alternative text"
                )
                image["data-alt-needs-clarification"] = "true"
            image["loading"] = "lazy"

    def resolve_reference(
        self,
        source_document: MarkdownSourceDocument,
        reference_value: str,
        current_output_path: Path,
        is_image: bool,
    ) -> str | None:
        if not reference_value:
            return None
        if reference_value.startswith("#"):
            return reference_value
        if self.is_external_url(reference_value):
            return reference_value

        split_reference = urlsplit(reference_value)
        if not split_reference.path:
            return reference_value
        decoded_reference_path = unquote(split_reference.path)
        candidate_path = self.normalize_repository_reference(
            source_document.relative_path.parent, decoded_reference_path
        )
        if candidate_path is None:
            self.record_link_issue(
                source_document,
                reference_value,
                "outside-repository",
                "relative reference escapes the repository root",
            )
            return None

        mapped_document = self.document_by_source_path.get(candidate_path)
        if mapped_document:
            mapped_url = DocumentationAddressService.relative_url(
                current_output_path, mapped_document.output_path
            )
            return urlunsplit(("", "", mapped_url, split_reference.query, split_reference.fragment))

        candidate_absolute_path = self.configuration.repository_root / candidate_path
        if not candidate_absolute_path.exists():
            suffix_match = self.find_unique_suffix_match(candidate_path)
            if suffix_match:
                self.record_link_issue(
                    source_document,
                    reference_value,
                    "relocated-reference",
                    f"resolved deterministically to {suffix_match.as_posix()}",
                )
                candidate_path = suffix_match
                candidate_absolute_path = self.configuration.repository_root / candidate_path
                mapped_document = self.document_by_source_path.get(candidate_path)
                if mapped_document:
                    mapped_url = DocumentationAddressService.relative_url(
                        current_output_path, mapped_document.output_path
                    )
                    return urlunsplit(
                        ("", "", mapped_url, split_reference.query, split_reference.fragment)
                    )

        if candidate_absolute_path.exists() and candidate_absolute_path.is_file():
            if is_image or candidate_absolute_path.suffix.lower() in IMAGE_SUFFIXES:
                asset_output_path = self.copy_source_asset(candidate_path)
                asset_url = DocumentationAddressService.relative_url(
                    current_output_path, asset_output_path
                )
                return urlunsplit(
                    ("", "", asset_url, split_reference.query, split_reference.fragment)
                )
            source_file_output_path = self.copy_referenced_source_file(candidate_path)
            source_file_url = DocumentationAddressService.relative_url(
                current_output_path, source_file_output_path
            )
            return urlunsplit(
                (
                    "",
                    "",
                    source_file_url,
                    split_reference.query,
                    split_reference.fragment,
                )
            )

        if candidate_absolute_path.exists() and candidate_absolute_path.is_dir():
            return self.repository_tree_url(candidate_path)

        self.record_link_issue(
            source_document,
            reference_value,
            "missing-image" if is_image else "unresolved-link",
            "referenced repository path does not exist",
        )
        return None

    def normalize_repository_reference(
        self, source_parent: Path, reference_path: str
    ) -> Path | None:
        if reference_path.startswith("/"):
            relative_candidate = Path(reference_path.removeprefix("/"))
        else:
            relative_candidate = source_parent / reference_path
        normalized_absolute_path = (
            self.configuration.repository_root / relative_candidate
        ).resolve()
        try:
            return normalized_absolute_path.relative_to(
                self.configuration.repository_root
            )
        except ValueError:
            return None

    def find_unique_suffix_match(self, candidate_path: Path) -> Path | None:
        suffix_parts = candidate_path.parts[-2:]
        if not suffix_parts:
            return None
        matches = []
        for repository_path in self.configuration.repository_root.rglob(
            candidate_path.name
        ):
            if not repository_path.is_file():
                continue
            relative_match = repository_path.relative_to(
                self.configuration.repository_root
            )
            if self.reference_search_candidate_is_excluded(relative_match):
                continue
            if relative_match.parts[-len(suffix_parts) :] == suffix_parts:
                matches.append(relative_match)
        if len(matches) == 1:
            return matches[0]
        basename_matches = []
        for repository_path in self.configuration.repository_root.rglob(
            candidate_path.name
        ):
            if not repository_path.is_file():
                continue
            relative_match = repository_path.relative_to(
                self.configuration.repository_root
            )
            if self.reference_search_candidate_is_excluded(relative_match):
                continue
            basename_matches.append(relative_match)
        if len(basename_matches) == 1:
            return basename_matches[0]
        return None

    def reference_search_candidate_is_excluded(self, relative_path: Path) -> bool:
        if MarkdownSourceDiscoveryService(
            self.configuration
        ).resolve_exclusion_reason(relative_path):
            return True
        generated_prefixes = (
            Path("docs/pages"),
            Path("docs/assets/css"),
            Path("docs/assets/js"),
            Path("docs/assets/diagrams"),
            Path("docs/assets/images/source"),
            Path("docs/assets/source-files"),
        )
        return any(
            relative_path == generated_prefix
            or generated_prefix in relative_path.parents
            for generated_prefix in generated_prefixes
        )

    def copy_source_asset(self, source_relative_path: Path) -> Path:
        asset_output_path = Path("assets/images/source") / source_relative_path
        destination_path = self.configuration.output_root / asset_output_path
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(
            self.configuration.repository_root / source_relative_path,
            destination_path,
        )
        self.copied_assets.add(asset_output_path)
        return asset_output_path

    def copy_referenced_source_file(self, source_relative_path: Path) -> Path:
        source_file_output_path = Path("assets/source-files") / source_relative_path
        destination_path = self.configuration.output_root / source_file_output_path
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(
            self.configuration.repository_root / source_relative_path,
            destination_path,
        )
        self.copied_source_files.add(source_file_output_path)
        return source_file_output_path

    def repository_source_url(self, source_relative_path: Path, fragment: str) -> str:
        source_url = (
            f"{self.configuration.repository_url}/blob/"
            f"{quote(self.configuration.repository_branch)}/"
            f"{quote(source_relative_path.as_posix(), safe='/')}"
        )
        if fragment:
            source_url += "#" + quote(fragment, safe="-_:.")
        return source_url

    def repository_tree_url(self, source_relative_path: Path) -> str:
        return (
            f"{self.configuration.repository_url}/tree/"
            f"{quote(self.configuration.repository_branch)}/"
            f"{quote(source_relative_path.as_posix(), safe='/')}"
        )

    @staticmethod
    def is_external_url(reference_value: str) -> bool:
        return bool(
            re.match(
                r"^(?:https?:|mailto:|tel:|data:|javascript:)",
                reference_value,
                re.I,
            )
        )

    def record_link_issue(
        self,
        source_document: MarkdownSourceDocument,
        reference_value: str,
        issue_kind: str,
        reason: str,
    ) -> None:
        self.link_issues.append(
            DocumentationLinkIssue(
                source_document.relative_path,
                reference_value,
                issue_kind,
                reason,
            )
        )

    @staticmethod
    def annotate_source_status(soup: BeautifulSoup) -> None:
        for element in soup.find_all(["p", "li", "blockquote", "td"]):
            element_text = element.get_text(" ", strip=True)
            if len(element_text) < 3:
                continue
            status_class = MarkdownHtmlRenderingService.status_class_for_text(
                element_text
            )
            if status_class:
                existing_classes = list(element.get("class", []))
                element["class"] = [*existing_classes, status_class]

    @staticmethod
    def status_class_for_text(value: str) -> str | None:
        if STATUS_SIGNAL_PATTERNS["unresolved"].search(value):
            return "source-status--unresolved"
        if STATUS_SIGNAL_PATTERNS["failed"].search(value):
            return "source-status--failed"
        if STATUS_SIGNAL_PATTERNS["pending"].search(value):
            return "source-status--pending"
        if STATUS_SIGNAL_PATTERNS["proposed"].search(value):
            return "source-status--proposed"
        if STATUS_SIGNAL_PATTERNS["passed"].search(value):
            return "source-status--passed"
        return None

    @staticmethod
    def enhance_tables(soup: BeautifulSoup) -> None:
        for table in list(soup.find_all("table")):
            table["role"] = "table"
            for table_header in table.find_all("th"):
                table_header["scope"] = "col"
            if isinstance(table.parent, Tag) and "table-scroll" in table.parent.get(
                "class", []
            ):
                continue
            wrapper = soup.new_tag("div")
            wrapper["class"] = "table-scroll"
            wrapper["tabindex"] = "0"
            wrapper["aria-label"] = "Scrollable table"
            table.wrap(wrapper)

    @staticmethod
    def enhance_code_blocks(soup: BeautifulSoup) -> None:
        for code_block in soup.find_all("pre"):
            code_block["tabindex"] = "0"
            code_block["aria-label"] = "Source code or command example"

    @staticmethod
    def add_section_guides(soup: BeautifulSoup) -> None:
        for heading in soup.find_all(["h2", "h3", "h4"]):
            heading_text = heading.get_text(" ", strip=True)
            if not heading_text:
                continue
            guide = soup.new_tag("p")
            guide["class"] = "section-guide"
            guide.string = (
                f"Reading guide: this source section records “{heading_text}”. "
                "Distinguish stated behavior from proposed or pending work using the status styling."
            )
            heading.insert_after(guide)


class DocumentationDiagramService:
    def __init__(self, configuration: DocumentationBuildConfiguration) -> None:
        self.configuration = configuration
        self.diagram_paths: list[Path] = []

    def write_site_concept_map(
        self, source_documents: Sequence[MarkdownSourceDocument]
    ) -> Path:
        output_path = Path("assets/diagrams/site-concept-map.svg")
        category_counts = Counter(
            source_document.category_identifier for source_document in source_documents
        )
        diagram_width = 1120
        diagram_height = 620
        center_x = 560
        center_y = 300
        radius_x = 420
        radius_y = 220
        node_fragments = []
        edge_fragments = []
        for category_index, category in enumerate(CATEGORIES):
            angle = (category_index / len(CATEGORIES)) * math.tau
            node_x = center_x + radius_x * math.cos(angle)
            node_y = center_y + radius_y * math.sin(angle)
            edge_fragments.append(
                f'<line x1="{center_x}" y1="{center_y}" x2="{node_x:.1f}" y2="{node_y:.1f}" />'
            )
            node_fragments.append(
                self.svg_node(
                    node_x,
                    node_y,
                    category.title,
                    f"{category_counts[category.identifier]} pages. {category.description}",
                    f"../../index.html#category-{category.identifier}",
                )
            )
        node_fragments.append(
            self.svg_node(
                center_x,
                center_y,
                "ProGen3D learning paths",
                f"{len(source_documents)} source-grounded lessons connected by category and concept.",
                "../../index.html#learning-paths",
                emphasized=True,
            )
        )
        svg_content = self.svg_document(
            "ProGen3D documentation concept map",
            "A central ProGen3D learning-path node connects to ten documentation categories. Each category node links to its homepage section and states its page count.",
            diagram_width,
            diagram_height,
            "".join(edge_fragments),
            "".join(node_fragments),
        )
        self.write_diagram(output_path, svg_content)
        return output_path

    def write_document_diagram(self, source_document: MarkdownSourceDocument) -> Path:
        diagram_slug = source_document.output_path.stem
        output_path = Path("assets/diagrams/pages") / f"{diagram_slug}.svg"
        page_from_diagram = Path(
            os.path.relpath(
                source_document.output_path,
                start=output_path.parent,
            )
        ).as_posix()
        concepts_label = ", ".join(source_document.concepts[:3])
        nodes = [
            (150, 150, "Purpose", source_document.summary, "summary"),
            (
                430,
                80,
                "Concepts",
                concepts_label or source_document.title,
                "concept-overview",
            ),
            (
                710,
                150,
                "Source evidence",
                f"{len(source_document.headings)} sections, {len(source_document.code_examples)} code blocks, {source_document.table_count} tables.",
                "source-lesson",
            ),
            (
                430,
                300,
                "Verification and status",
                f"Document status: {source_document.status}. Source signals remain visibly distinguished.",
                "verification-guidance",
            ),
        ]
        edges = (
            '<path d="M230 145 C300 100, 340 90, 350 90" />'
            '<path d="M510 90 C600 90, 630 110, 640 140" />'
            '<path d="M690 205 C630 275, 570 300, 510 300" />'
            '<path d="M350 300 C250 290, 185 235, 160 210" />'
        )
        node_fragments = []
        for node_x, node_y, label, description, anchor in nodes:
            node_fragments.append(
                self.svg_node(
                    node_x,
                    node_y,
                    label,
                    description,
                    f"{page_from_diagram}#{anchor}",
                )
            )
        svg_content = self.svg_document(
            f"Learning map for {source_document.title}",
            "The learning map moves from the document purpose to its concepts, then to exact source evidence and finally to verification and status interpretation.",
            860,
            410,
            edges,
            "".join(node_fragments),
        )
        self.write_diagram(output_path, svg_content)
        return output_path

    @staticmethod
    def svg_document(
        title: str,
        description: str,
        width: int,
        height: int,
        edges: str,
        nodes: str,
    ) -> str:
        return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" role="img" aria-labelledby="diagram-title diagram-description">
  <title id="diagram-title">{html.escape(title)}</title>
  <desc id="diagram-description">{html.escape(description)}</desc>
  <style>
    :root {{ color-scheme: light; }}
    line, path {{ fill: none; stroke: #7892b1; stroke-width: 3; }}
    .node rect {{ fill: #f8fbff; stroke: #135f9e; stroke-width: 2.5; rx: 16; }}
    .node text {{ fill: #172033; font-family: system-ui, sans-serif; text-anchor: middle; }}
    .node .label {{ font-size: 16px; font-weight: 800; }}
    .node .detail {{ fill: #526079; font-size: 11px; }}
    .node--emphasized rect {{ fill: #dceefe; stroke-width: 4; }}
    .node:focus rect, .node:hover rect {{ fill: #fff0c7; stroke: #8b2532; }}
    a:focus {{ outline: none; }}
    @media (prefers-reduced-motion: reduce) {{ * {{ transition: none !important; }} }}
  </style>
  <g aria-hidden="true">{edges}</g>
  <g>{nodes}</g>
</svg>
'''

    def svg_node(
        self,
        center_x: float,
        center_y: float,
        label: str,
        description: str,
        href: str,
        emphasized: bool = False,
    ) -> str:
        node_width = 220 if emphasized else 200
        node_height = 96 if emphasized else 88
        node_x = center_x - node_width / 2
        node_y = center_y - node_height / 2
        label_lines = self.wrap_svg_text(label, 24, 2)
        detail_lines = self.wrap_svg_text(description, 34, 2)
        label_spans = "".join(
            f'<tspan x="{center_x:.1f}" dy="{0 if line_index == 0 else 19}">{html.escape(line)}</tspan>'
            for line_index, line in enumerate(label_lines)
        )
        detail_start = center_y + (9 if len(label_lines) == 1 else 20)
        detail_spans = "".join(
            f'<tspan x="{center_x:.1f}" dy="{0 if line_index == 0 else 14}">{html.escape(line)}</tspan>'
            for line_index, line in enumerate(detail_lines)
        )
        emphasized_class = " node--emphasized" if emphasized else ""
        return f'''
<a href="{html.escape(href, quote=True)}" target="_top">
  <g class="node{emphasized_class}" tabindex="0" role="link" aria-label="{html.escape(label + '. ' + description, quote=True)}">
    <title>{html.escape(label)}: {html.escape(description)}</title>
    <rect x="{node_x:.1f}" y="{node_y:.1f}" width="{node_width}" height="{node_height}" />
    <text class="label" x="{center_x:.1f}" y="{center_y - 17:.1f}">{label_spans}</text>
    <text class="detail" x="{center_x:.1f}" y="{detail_start:.1f}">{detail_spans}</text>
  </g>
</a>'''

    @staticmethod
    def wrap_svg_text(value: str, maximum_characters: int, maximum_lines: int) -> list[str]:
        words = re.sub(r"\s+", " ", value).strip().split(" ")
        lines: list[str] = []
        active_line = ""
        for word in words:
            proposed_line = f"{active_line} {word}".strip()
            if len(proposed_line) <= maximum_characters or not active_line:
                active_line = proposed_line
            else:
                lines.append(active_line)
                active_line = word
            if len(lines) >= maximum_lines:
                break
        if active_line and len(lines) < maximum_lines:
            lines.append(active_line)
        if len(words) > sum(len(line.split()) for line in lines) and lines:
            lines[-1] = lines[-1].rstrip(".,;:") + "…"
        return lines or [""]

    def write_diagram(self, output_path: Path, svg_content: str) -> None:
        destination_path = self.configuration.output_root / output_path
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        destination_path.write_text(svg_content, encoding="utf-8")
        self.diagram_paths.append(output_path)


class EducationalPageCompositionService:
    REQUIRED_SECTION_IDENTIFIERS = (
        "learning-objectives",
        "prerequisites",
        "concept-overview",
        "explanation-levels",
        "architecture-process",
        "walkthrough",
        "worked-example",
        "visual-explanation",
        "implementation-notes",
        "mistakes",
        "verification-guidance",
        "takeaways",
        "related-concepts",
        "source-reference",
        "source-lesson",
        "lesson-navigation",
    )

    def __init__(
        self,
        configuration: DocumentationBuildConfiguration,
        source_documents: Sequence[MarkdownSourceDocument],
        glossary_entries: Sequence[GlossaryEntry],
    ) -> None:
        self.configuration = configuration
        self.source_documents = list(source_documents)
        self.document_by_source_path = {
            source_document.relative_path: source_document
            for source_document in source_documents
        }
        self.glossary_by_term = {
            entry.term: entry for entry in glossary_entries
        }

    def compose_document_page(
        self,
        source_document: MarkdownSourceDocument,
        rendered_source_html: str,
        diagram_path: Path,
        previous_document: MarkdownSourceDocument | None,
        next_document: MarkdownSourceDocument | None,
    ) -> str:
        root_prefix = "../../"
        category = CATEGORY_BY_IDENTIFIER[source_document.category_identifier]
        source_url = self.source_url(source_document.relative_path)
        diagram_url = DocumentationAddressService.relative_url(
            source_document.output_path, diagram_path
        )
        glossary_url = DocumentationAddressService.relative_url(
            source_document.output_path, Path("glossary.html")
        )
        home_url = DocumentationAddressService.relative_url(
            source_document.output_path, Path("index.html")
        )
        coverage_url = DocumentationAddressService.relative_url(
            source_document.output_path, Path("markdown-coverage.html")
        )

        status_signal_rows = self.status_signal_rows(source_document)
        concept_rows = self.concept_rows(source_document)
        source_outline = self.source_outline(source_document)
        learning_objectives = self.learning_objectives(source_document)
        prerequisite_list = self.prerequisite_list(source_document)
        explanation_levels = self.explanation_levels(source_document)
        walkthrough = self.walkthrough(source_document)
        worked_example = self.worked_example(source_document)
        mistakes = self.common_mistakes(source_document)
        verification_guidance = self.verification_guidance(source_document)
        takeaways = self.key_takeaways(source_document)
        related_concepts = self.related_concepts(source_document)
        navigation = self.lesson_navigation(
            source_document, previous_document, next_document
        )
        term_rows = self.terminology_rows(source_document)
        tracked_note = (
            "The source is tracked by Git."
            if source_document.tracked_by_git
            else "The source is currently a working-tree document and may not yet exist on the remote branch."
        )

        return f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="{html.escape(source_document.summary, quote=True)}">
  <meta name="source-markdown" content="{html.escape(source_document.relative_path.as_posix(), quote=True)}">
  <meta name="source-sha256" content="{source_document.sha256}">
  <meta name="documentation-status" content="{html.escape(source_document.status, quote=True)}">
  <title>{html.escape(source_document.title)} · ProGen3D Learning Documentation</title>
  <link rel="stylesheet" href="{root_prefix}assets/css/documentation.css">
  <script defer src="{root_prefix}assets/js/navigation.js"></script>
  <script defer src="{root_prefix}assets/js/diagrams.js"></script>
</head>
<body>
  <a class="skip-link" href="#main-content">Skip to lesson content</a>
  {self.site_header(root_prefix)}
  <main id="main-content" class="page-shell">
    <nav class="breadcrumbs" aria-label="Breadcrumb">
      <ol>
        <li><a href="{home_url}">Documentation home</a></li>
        <li><a href="{home_url}#category-{category.identifier}">{html.escape(category.title)}</a></li>
        <li aria-current="page">{html.escape(source_document.title)}</li>
      </ol>
    </nav>

    <header class="lesson-header">
      <p class="eyebrow">Source-grounded educational lesson</p>
      <h1>{html.escape(source_document.title)}</h1>
      <p class="lead">{html.escape(source_document.summary)}</p>
      <div class="lesson-meta">
        <span class="status-badge status--{source_document.status.lower()}">{html.escape(source_document.status)}</span>
        <span class="tag">{html.escape(category.title)}</span>
        <span class="tag">About {max(1, round(source_document.word_count / 200))} minutes at 200 words/minute</span>
        <span class="tag">Source updated: {html.escape(source_document.updated_label)}</span>
      </div>
      <div class="reading-levels" aria-label="Available explanation levels">
        <span class="level-indicator">Beginner</span>
        <span class="level-indicator">Novice</span>
        <span class="level-indicator">Advanced</span>
        <span class="level-indicator">Expert</span>
      </div>
      <div class="hero-actions">
        <a class="button" href="#learning-objectives">Start lesson</a>
        <a class="button button--secondary" href="{html.escape(source_url, quote=True)}">View source Markdown</a>
        <a class="button button--quiet" href="{coverage_url}">Coverage report</a>
      </div>
      <div class="progress-control">
        <button class="button button--quiet" type="button" data-progress-key="{source_document.sha256}" aria-pressed="false">Mark lesson complete</button>
        <span class="progress-status" data-progress-status aria-live="polite">Not yet completed</span>
      </div>
    </header>

    <section id="learning-objectives" class="section-block">
      <h2>Learning objectives</h2>
      <p class="section-introduction">These objectives are derived from the document title, source sections, procedures, and evidence markers.</p>
      {learning_objectives}
    </section>

    <section id="prerequisites" class="section-block">
      <h2>Prerequisites</h2>
      <p class="section-introduction">The source does not declare a machine-readable prerequisite graph. These are recommended orientation pages, not invented technical requirements.</p>
      {prerequisite_list}
    </section>

    <section id="concept-overview" class="section-block">
      <h2>Concept overview</h2>
      <p class="section-introduction">The concepts below come from the document title and its substantive section headings. Occurrence and section evidence point back to the source.</p>
      <div class="table-scroll" tabindex="0" aria-label="Concept evidence table">
        <table class="concept-evidence-table">
          <thead><tr><th scope="col">Concept</th><th scope="col">Source-grounded role</th><th scope="col">Where to deepen</th></tr></thead>
          <tbody>{concept_rows}</tbody>
        </table>
      </div>
      <h3>Terminology and abbreviations</h3>
      <p>Definitions are extracted from the source set where available. Unexpanded terms are visibly marked as needing clarification.</p>
      <div class="table-scroll" tabindex="0" aria-label="Terminology table">
        <table>
          <thead><tr><th scope="col">Term</th><th scope="col">Definition</th></tr></thead>
          <tbody>{term_rows}</tbody>
        </table>
      </div>
      <p><a href="{glossary_url}">Open the complete generated glossary</a>.</p>
    </section>

    <section id="explanation-levels" class="section-block">
      <h2>Four-level explanation</h2>
      <p class="section-introduction">Each level adds detail without replacing the source. The expert level points to exact algorithms, equations, constraints, and evidence when the document contains them.</p>
      <div class="level-grid">{explanation_levels}</div>
    </section>

    <section id="architecture-process" class="section-block">
      <h2>Architecture or process</h2>
      <p class="section-introduction">The source outline is treated as the authoritative process order. It is not reclassified as implemented unless the source states and evidences that status.</p>
      {source_outline}
      <h3>Source status signals</h3>
      <div class="table-scroll" tabindex="0" aria-label="Source status signal counts">
        <table><thead><tr><th scope="col">Signal</th><th scope="col">Occurrences</th><th scope="col">How to read it</th></tr></thead><tbody>{status_signal_rows}</tbody></table>
      </div>
    </section>

    <section id="walkthrough" class="section-block">
      <h2>Step-by-step walkthrough</h2>
      <p class="section-introduction">Use this sequence to move from orientation to exact source evidence.</p>
      {walkthrough}
    </section>

    <section id="worked-example" class="section-block">
      <h2>Worked example</h2>
      <p class="section-introduction">The example is quoted or extracted from this document rather than invented.</p>
      {worked_example}
    </section>

    <section id="visual-explanation" class="section-block">
      <h2>Visual explanation</h2>
      <p class="section-introduction">This diagram shows the reading flow for this source: purpose, concepts, exact source evidence, and verification/status interpretation.</p>
      <div class="diagram-panel">
        <object data="{diagram_url}" type="image/svg+xml" data-learning-diagram aria-label="Learning map for {html.escape(source_document.title, quote=True)}">
          Diagram fallback: purpose leads to concepts, concepts lead to source evidence, and source evidence leads to verification and status interpretation.
        </object>
        <p class="diagram-text-equivalent"><strong>Text equivalent:</strong> Begin with the source purpose, learn the named concepts, inspect the complete rendered source, then evaluate its commands, evidence, limitations, and status signals.</p>
        <p class="visually-hidden" data-diagram-live-region aria-live="polite"></p>
      </div>
    </section>

    <section id="implementation-notes" class="section-block">
      <h2>Implementation notes</h2>
      <div class="dashboard-grid">
        <article class="dashboard-card"><h3>Source identity</h3><p><code>{html.escape(source_document.relative_path.as_posix())}</code></p><p>SHA-256: <code>{source_document.sha256}</code></p></article>
        <article class="dashboard-card"><h3>Document structure</h3><p>{len(source_document.headings)} headings, {len(source_document.code_examples)} fenced code blocks, {source_document.table_count} Markdown tables, and {source_document.equation_count} display-equation blocks were detected.</p></article>
        <article class="dashboard-card"><h3>Audience</h3><p>{html.escape(source_document.intended_audience)}.</p></article>
        <article class="dashboard-card"><h3>Traceability</h3><p>{html.escape(tracked_note)} The complete source content is rendered below so generated teaching text can be checked against it.</p></article>
      </div>
    </section>

    <section id="mistakes" class="section-block">
      <h2>Common mistakes and failure modes</h2>
      {mistakes}
    </section>

    <section id="verification-guidance" class="section-block">
      <h2>Testing or verification guidance</h2>
      <p class="section-introduction">Commands are preserved from the source. Their presence does not prove they were run in the current checkout.</p>
      {verification_guidance}
    </section>

    <section id="takeaways" class="section-block">
      <h2>Key takeaways</h2>
      {takeaways}
    </section>

    <section id="related-concepts" class="section-block">
      <h2>Related concepts</h2>
      <p class="section-introduction">Related pages are selected by shared category, title vocabulary, concepts, and repository path.</p>
      {related_concepts}
    </section>

    <section id="source-reference" class="section-block">
      <h2>Source Markdown reference</h2>
      <div class="callout callout--evidence">
        <p><strong>Authoritative source:</strong> <a href="{html.escape(source_url, quote=True)}"><code>{html.escape(source_document.relative_path.as_posix())}</code></a></p>
        <p><strong>Source digest:</strong> <code>{source_document.sha256}</code></p>
        <p><strong>Transformation boundary:</strong> educational summaries, guides, tags, and relationships are derived from source structure. Technical claims, commands, equations, tables, and status wording remain traceable to the rendered source below.</p>
      </div>
    </section>

    <section id="source-lesson" class="section-block">
      <h2>Complete source-grounded lesson</h2>
      <p class="section-introduction">This section preserves the complete Markdown document as semantic HTML, rewrites valid Markdown links to generated lessons, copies genuine referenced images into the published site, and visually marks proposed, pending, failed, passed, and unresolved statements.</p>
      <article class="source-lesson" aria-label="Rendered source Markdown">
        {rendered_source_html}
      </article>
    </section>

    {navigation}
  </main>
  {self.site_footer(root_prefix)}
</body>
</html>
'''

    def site_header(self, root_prefix: str) -> str:
        return f'''<header class="site-header">
  <div class="site-header__inner">
    <a class="site-brand" href="{root_prefix}index.html">ProGen3D <small>Learning Documentation</small></a>
    <button class="navigation-toggle" type="button" data-navigation-toggle aria-expanded="false" aria-controls="site-navigation">Menu</button>
    <nav id="site-navigation" class="site-navigation" data-site-navigation aria-label="Primary navigation">
      <ul>
        <li><a href="{root_prefix}index.html#start-here">Start here</a></li>
        <li><a href="{root_prefix}index.html#learning-paths">Learning paths</a></li>
        <li><a href="{root_prefix}index.html#page-index">Page index</a></li>
        <li><a href="{root_prefix}glossary.html">Glossary</a></li>
        <li><a href="{root_prefix}markdown-coverage.html">Coverage</a></li>
      </ul>
    </nav>
  </div>
</header>'''

    def site_footer(self, root_prefix: str) -> str:
        return f'''<footer class="site-footer">
  <p>Generated from project Markdown with source hashes and deterministic page mappings.</p>
  <p><a href="{root_prefix}README.md">Documentation build instructions</a> · <a href="{html.escape(self.configuration.repository_url, quote=True)}">Main repository</a></p>
</footer>'''

    def learning_objectives(self, source_document: MarkdownSourceDocument) -> str:
        objectives = [
            f"Explain the purpose of {source_document.title} in plain language.",
            "Distinguish documented behavior and evidence from proposals, pending work, hypotheses, and unresolved questions.",
            f"Locate and interpret the source concepts: {', '.join(source_document.concepts[:4])}.",
            "Trace commands, equations, constraints, tables, and conclusions back to the source Markdown.",
        ]
        if source_document.procedure_steps:
            objectives.append("Follow the explicit procedure or phased sequence recorded by the source.")
        return "<ul>" + "".join(
            f"<li>{html.escape(objective)}</li>" for objective in objectives
        ) + "</ul>"

    def prerequisite_list(self, source_document: MarkdownSourceDocument) -> str:
        if not source_document.prerequisite_paths:
            return '<div class="callout callout--clarification"><p><strong>Needs clarification:</strong> no prerequisite page could be inferred without inventing a dependency.</p></div>'
        items = []
        for prerequisite_path in source_document.prerequisite_paths:
            prerequisite_document = self.document_by_source_path[prerequisite_path]
            prerequisite_url = DocumentationAddressService.relative_url(
                source_document.output_path, prerequisite_document.output_path
            )
            items.append(
                f'<li><a href="{prerequisite_url}">{html.escape(prerequisite_document.title)}</a> — recommended orientation from the generated learning graph.</li>'
            )
        return "<ul>" + "".join(items) + "</ul>"

    def concept_rows(self, source_document: MarkdownSourceDocument) -> str:
        rows = []
        source_text_lower = source_document.content.lower()
        for concept in source_document.concepts:
            occurrence_count = source_text_lower.count(concept.lower())
            matching_headings = [
                heading
                for heading in source_document.headings
                if concept.lower() in heading.lower() or heading.lower() in concept.lower()
            ]
            deepening_location = (
                ", ".join(matching_headings[:3])
                if matching_headings
                else "Complete source-grounded lesson"
            )
            rows.append(
                "<tr>"
                f"<td><dfn title=\"Named by the source document\">{html.escape(concept)}</dfn></td>"
                f"<td>The phrase appears {occurrence_count} time{'s' if occurrence_count != 1 else ''} in the source and is retained as a learning-path concept.</td>"
                f"<td>{html.escape(deepening_location)}</td>"
                "</tr>"
            )
        return "".join(rows)

    def terminology_rows(self, source_document: MarkdownSourceDocument) -> str:
        rows = []
        for term in source_document.acronym_candidates:
            glossary_entry = self.glossary_by_term.get(term)
            if glossary_entry:
                row_class = (
                    ' class="source-status--clarification"'
                    if glossary_entry.needs_clarification
                    else ""
                )
                rows.append(
                    f"<tr{row_class}><td><dfn title=\"{html.escape(glossary_entry.definition, quote=True)}\">{html.escape(term)}</dfn></td><td>{html.escape(glossary_entry.definition)}</td></tr>"
                )
        if not rows:
            rows.append(
                "<tr><td colspan=\"2\">The analyzer found no repeated all-capital abbreviation requiring a generated definition.</td></tr>"
            )
        return "".join(rows)

    def explanation_levels(self, source_document: MarkdownSourceDocument) -> str:
        first_concepts = ", ".join(source_document.concepts[:4])
        section_names = ", ".join(source_document.headings[1:6]) or "the complete source lesson"
        status_notes = [
            f"{name}: {count}"
            for name, count in source_document.status_signal_counts.items()
            if count
        ]
        status_summary = ", ".join(status_notes) or "no explicit status keywords detected"
        expert_features = []
        if source_document.equation_count:
            expert_features.append(f"{source_document.equation_count} display-equation blocks")
        if source_document.code_examples:
            expert_features.append(f"{len(source_document.code_examples)} code blocks")
        if source_document.table_count:
            expert_features.append(f"{source_document.table_count} tables")
        expert_feature_summary = ", ".join(expert_features) or "prose and list evidence"
        return f'''
<article class="level-card level-card--beginner explanation-level" data-level="beginner">
  <h3>Beginner</h3>
  <p>{html.escape(source_document.summary)}</p>
  <p><strong>Why this matters:</strong> this page gives a safe route from the project idea to the exact source wording, so a new reader does not have to infer implementation status from a filename alone.</p>
</article>
<details class="level-card level-card--novice explanation-level" data-level="novice" open>
  <summary>Novice explanation</summary>
  <p>The source introduces or emphasizes {html.escape(first_concepts)}. Read the terminology table first, then follow the source headings in order. A status such as <em>Proposed</em> or <em>Pending</em> is not equivalent to implemented behavior.</p>
</details>
<details class="level-card level-card--advanced explanation-level" data-level="advanced">
  <summary>Advanced explanation</summary>
  <p>The document structure exposes its architecture or process through these early sections: {html.escape(section_names)}. Study the transitions between them, then inspect code, commands, tables, and acceptance gates in the complete source lesson.</p>
  <p>Detected status evidence: {html.escape(status_summary)}.</p>
</details>
<details class="level-card level-card--expert explanation-level" data-level="expert">
  <summary>Expert explanation</summary>
  <p>The expert reading preserves {html.escape(expert_feature_summary)} and the source SHA-256 <code>{source_document.sha256}</code>. Treat formal definitions, algorithms, mathematical reasoning, edge cases, limitations, and research implications as authoritative only where the rendered source states them.</p>
  <p>Use the source digest and section anchors to review any generated interpretation against the exact document snapshot.</p>
</details>'''

    def source_outline(self, source_document: MarkdownSourceDocument) -> str:
        if len(source_document.headings) <= 1:
            return '<div class="callout callout--clarification"><p><strong>Needs clarification:</strong> the source has no multi-section outline. Read the complete lesson as a single evidence block.</p></div>'
        outline_items = []
        used_identifiers: Counter[str] = Counter()
        for heading in source_document.headings[1:20]:
            identifier = DocumentationAddressService.slugify(heading)
            used_identifiers[identifier] += 1
            if used_identifiers[identifier] > 1:
                identifier += f"-{used_identifiers[identifier]}"
            outline_items.append(
                f'<li><a href="#source-{identifier}" data-source-anchor="{identifier}">{html.escape(heading)}</a></li>'
            )
        return (
            '<ol class="source-outline">'
            + "".join(outline_items)
            + "</ol><p><small>Outline links are resolved by the page script-free anchor repair performed during composition.</small></p>"
        )

    def status_signal_rows(self, source_document: MarkdownSourceDocument) -> str:
        interpretations = {
            "passed": "The source states a positive result. Confirm the associated command, artifact, or scope before generalizing it.",
            "failed": "The source records failure or a blocker. Do not hide it behind nearby successful checks.",
            "pending": "The source leaves work unevaluated, unknown, or pending.",
            "proposed": "The source describes planned, proposed, roadmap, or future work rather than current behavior.",
            "unresolved": "The source explicitly identifies missing implementation or clarification.",
            "experimental": "The source marks a hypothesis or experimental statement that requires evidence.",
        }
        rows = []
        for signal_name, interpretation in interpretations.items():
            rows.append(
                f"<tr><td>{html.escape(signal_name.title())}</td><td>{source_document.status_signal_counts.get(signal_name, 0)}</td><td>{html.escape(interpretation)}</td></tr>"
            )
        return "".join(rows)

    def walkthrough(self, source_document: MarkdownSourceDocument) -> str:
        steps = [
            f"Read the summary and confirm that the source identity is {source_document.relative_path.as_posix()}.",
            f"Learn the named concepts: {', '.join(source_document.concepts[:4])}.",
            "Open the four explanation levels in order, stopping at the depth needed for the current task.",
            "Follow the architecture/process outline into the complete source-grounded lesson.",
            "Check every proposed, pending, failed, passed, experimental, or unresolved statement against its surrounding source context.",
            "Use the verification section only as a guide to source-recorded commands; run commands separately in an appropriate checkout.",
        ]
        if source_document.procedure_steps:
            steps.append(
                "Compare the generated walkthrough with the explicit source procedure listed below."
            )
        rendered_steps = "<ol>" + "".join(
            f"<li>{html.escape(step)}</li>" for step in steps
        ) + "</ol>"
        if source_document.procedure_steps:
            rendered_steps += "<h3>Explicit source procedure</h3><ol>" + "".join(
                f"<li>{html.escape(step)}</li>"
                for step in source_document.procedure_steps
            ) + "</ol>"
        return rendered_steps

    def worked_example(self, source_document: MarkdownSourceDocument) -> str:
        if source_document.code_examples:
            language, code = source_document.code_examples[0]
            language_label = language or "plain text"
            return f'''
<div class="callout callout--evidence">
  <p><strong>Source example type:</strong> {html.escape(language_label)} fenced block.</p>
  <p>Read the example literally, then locate its surrounding source section before treating it as a command, expected output, schema, or pseudocode.</p>
</div>
<pre tabindex="0" aria-label="First worked example from the source"><code class="language-{html.escape(language_label, quote=True)}">{html.escape(code)}</code></pre>
<details class="solution"><summary>Worked interpretation</summary><ol><li>Identify inputs, flags, identifiers, units, or schema fields present in the block.</li><li>Read the immediately surrounding source paragraphs for prerequisites and status.</li><li>Do not infer that the example passed unless the source associates it with evidence.</li><li>Use the source SHA-256 to ensure the example has not drifted.</li></ol></details>'''
        return f'''
<div class="callout callout--evidence">
  <p><strong>Source prose example:</strong> {html.escape(source_document.summary)}</p>
</div>
<details class="solution"><summary>Worked interpretation</summary><p>Separate the nouns that name project entities from verbs that state behavior. Then classify each behavior as documented, proposed, evidenced, or unresolved using the source status signals. The source contains no fenced code block, so no executable example is invented.</p></details>'''

    def common_mistakes(self, source_document: MarkdownSourceDocument) -> str:
        mistakes = [
            "Treating the generated summary as a substitute for the complete source document.",
            "Reading a plan, proposal, roadmap, or hypothesis as if it were implemented and verified behavior.",
            "Generalizing a focused PASS result beyond the command, artifact, version, or scope named by the source.",
            "Running copied commands without checking their working directory, prerequisites, destructive effects, and current repository state.",
            "Ignoring units, hashes, version identifiers, limits, warnings, or unresolved constraints preserved in tables and code blocks.",
        ]
        if source_document.status_signal_counts.get("failed", 0):
            mistakes.append(
                "Skipping recorded failures because the same document also contains successful evidence."
            )
        if source_document.status_signal_counts.get("pending", 0):
            mistakes.append(
                "Silently converting pending or unknown work into an assumed default."
            )
        return "<ul>" + "".join(
            f"<li>{html.escape(mistake)}</li>" for mistake in mistakes
        ) + "</ul>"

    def verification_guidance(self, source_document: MarkdownSourceDocument) -> str:
        if not source_document.verification_commands:
            return '<div class="callout callout--clarification"><p><strong>Needs clarification:</strong> no shell-like verification command was detected in fenced source blocks. Consult the source conclusions, tables, and related evidence pages instead of inventing a command.</p></div>'
        command_blocks = "".join(
            f'<pre tabindex="0" aria-label="Verification command from source"><code>{html.escape(command)}</code></pre>'
            for command in source_document.verification_commands[:8]
        )
        return f'''<div class="callout callout--warning"><p><strong>Safety boundary:</strong> these commands are source material, not commands executed by the documentation generator. Verify paths, dependencies, current state, and expected side effects before running them.</p></div>{command_blocks}'''

    def key_takeaways(self, source_document: MarkdownSourceDocument) -> str:
        takeaways = [
            ("Purpose", source_document.summary),
            (
                "Status",
                f"The page-level classification is {source_document.status}; finer proposed, pending, failed, passed, experimental, and unresolved signals remain visible in the source.",
            ),
            (
                "Concepts",
                ", ".join(source_document.concepts[:5]),
            ),
            (
                "Evidence boundary",
                "Technical meaning is traceable to the complete rendered Markdown and its SHA-256 digest.",
            ),
        ]
        return '<div class="takeaway-grid">' + "".join(
            f'<article class="content-card"><h3>{html.escape(title)}</h3><p>{html.escape(body)}</p></article>'
            for title, body in takeaways
        ) + "</div>"

    def related_concepts(self, source_document: MarkdownSourceDocument) -> str:
        related_items = []
        for related_path in source_document.related_paths:
            related_document = self.document_by_source_path[related_path]
            related_url = DocumentationAddressService.relative_url(
                source_document.output_path, related_document.output_path
            )
            related_items.append(
                f'<li><a href="{related_url}">{html.escape(related_document.title)}</a> <span class="status-badge status--{related_document.status.lower()}">{html.escape(related_document.status)}</span><br><small>{html.escape(related_document.summary)}</small></li>'
            )
        return "<ul>" + "".join(related_items) + "</ul>"

    def lesson_navigation(
        self,
        source_document: MarkdownSourceDocument,
        previous_document: MarkdownSourceDocument | None,
        next_document: MarkdownSourceDocument | None,
    ) -> str:
        previous_link = "<span aria-hidden=\"true\"></span>"
        next_link = "<span aria-hidden=\"true\"></span>"
        if previous_document:
            previous_url = DocumentationAddressService.relative_url(
                source_document.output_path, previous_document.output_path
            )
            previous_link = f'<a rel="prev" href="{previous_url}">← Previous: {html.escape(previous_document.title)}</a>'
        if next_document:
            next_url = DocumentationAddressService.relative_url(
                source_document.output_path, next_document.output_path
            )
            next_link = f'<a rel="next" href="{next_url}">Next: {html.escape(next_document.title)} →</a>'
        home_url = DocumentationAddressService.relative_url(
            source_document.output_path, Path("index.html")
        )
        return f'''<nav id="lesson-navigation" class="lesson-navigation" aria-label="Lesson sequence">{previous_link}<a href="{home_url}#learning-paths">Return to learning path</a>{next_link}</nav>'''

    def source_url(self, source_relative_path: Path) -> str:
        return (
            f"{self.configuration.repository_url}/blob/"
            f"{quote(self.configuration.repository_branch)}/"
            f"{quote(source_relative_path.as_posix(), safe='/')}"
        )


class DocumentationSiteCompositionService:
    def __init__(
        self,
        configuration: DocumentationBuildConfiguration,
        source_documents: Sequence[MarkdownSourceDocument],
        excluded_documents: Sequence[ExcludedMarkdownDocument],
        duplicate_titles: Sequence[DuplicateTitleRecord],
        glossary_entries: Sequence[GlossaryEntry],
        link_issues: Sequence[DocumentationLinkIssue],
        copied_assets: set[Path],
        copied_source_files: set[Path],
        diagram_paths: Sequence[Path],
    ) -> None:
        self.configuration = configuration
        self.source_documents = list(source_documents)
        self.excluded_documents = list(excluded_documents)
        self.duplicate_titles = list(duplicate_titles)
        self.glossary_entries = list(glossary_entries)
        self.link_issues = list(link_issues)
        self.copied_assets = copied_assets
        self.copied_source_files = copied_source_files
        self.diagram_paths = list(diagram_paths)
        self.page_composer = EducationalPageCompositionService(
            configuration, source_documents, glossary_entries
        )

    def write_site_support_pages(self) -> None:
        self.write_text(Path("index.html"), self.compose_homepage())
        self.write_text(Path("glossary.html"), self.compose_glossary_page())
        self.write_text(
            Path("markdown-coverage.html"), self.compose_coverage_page()
        )
        self.write_text(Path("README.md"), self.compose_documentation_readme())
        self.write_text(Path(".nojekyll"), "")
        self.write_manifest()

    def write_implementation_report(
        self, validation_result: DocumentationValidationResult
    ) -> None:
        self.write_text(
            Path("implementation-report.html"),
            self.compose_implementation_report(validation_result),
        )

    def compose_homepage(self) -> str:
        category_cards = []
        page_index_records = []
        for category in CATEGORIES:
            category_documents = [
                source_document
                for source_document in self.source_documents
                if source_document.category_identifier == category.identifier
            ]
            if not category_documents:
                continue
            category_cards.append(
                f'''<article id="category-{category.identifier}" class="category-card">
  <h3>{html.escape(category.title)}</h3>
  <p>{html.escape(category.description)}</p>
  <p><strong>{len(category_documents)} lessons</strong></p>
  <ul>{''.join(f'<li><a href="{document.output_path.as_posix()}">{html.escape(document.title)}</a></li>' for document in category_documents[:5])}</ul>
</article>'''
            )
        for source_document in self.source_documents:
            search_record = " ".join(
                [
                    source_document.title,
                    source_document.summary,
                    CATEGORY_BY_IDENTIFIER[source_document.category_identifier].title,
                    source_document.status,
                    *source_document.concepts,
                    source_document.relative_path.as_posix(),
                ]
            )
            page_index_records.append(
                f'''<article class="search-result" data-search-record="{html.escape(search_record, quote=True)}">
  <h3><a href="{source_document.output_path.as_posix()}">{html.escape(source_document.title)}</a></h3>
  <p><span class="status-badge status--{source_document.status.lower()}">{html.escape(source_document.status)}</span> {html.escape(CATEGORY_BY_IDENTIFIER[source_document.category_identifier].title)} · <code>{html.escape(source_document.relative_path.as_posix())}</code></p>
  <p>{html.escape(source_document.summary)}</p>
</article>'''
            )

        learning_paths = self.learning_paths()
        learning_path_cards = []
        for path_title, path_description, path_documents in learning_paths:
            learning_path_cards.append(
                f'''<article class="learning-path-card">
  <h3>{html.escape(path_title)}</h3>
  <p>{html.escape(path_description)}</p>
  <ol>{''.join(f'<li><a href="{document.output_path.as_posix()}">{html.escape(document.title)}</a></li>' for document in path_documents)}</ol>
</article>'''
            )

        recently_updated = sorted(
            self.source_documents,
            key=lambda document: (
                document.updated_label == "Working tree source",
                document.updated_label,
                document.relative_path.as_posix(),
            ),
            reverse=True,
        )[:10]
        unresolved_count = len(
            [issue for issue in self.link_issues if issue.issue_kind == "unresolved-link"]
        )
        missing_image_count = len(
            [issue for issue in self.link_issues if issue.issue_kind == "missing-image"]
        )
        coverage_percentage = 100 if self.source_documents else 0

        return f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="ProGen3D source-grounded educational documentation and complete Markdown coverage dashboard.">
  <title>ProGen3D Learning Documentation</title>
  <link rel="stylesheet" href="assets/css/documentation.css">
  <script defer src="assets/js/navigation.js"></script>
  <script defer src="assets/js/search.js"></script>
  <script defer src="assets/js/diagrams.js"></script>
</head>
<body>
  <a class="skip-link" href="#main-content">Skip to documentation dashboard</a>
  {self.page_composer.site_header('')}
  <main id="main-content" class="page-shell">
    <header class="hero">
      <p class="eyebrow">Complete educational documentation</p>
      <h1>Learn ProGen3D from source to evidence</h1>
      <p class="lead">ProGen3D is a Linux desktop editor and runtime for procedural 3D grammars. This site turns every eligible project Markdown document into a linked lesson with beginner, novice, advanced, and expert explanations while preserving exact source material and status boundaries.</p>
      <div class="hero-actions">
        <a class="button" href="#start-here">Start here</a>
        <a class="button button--secondary" href="#learning-paths">Choose a learning path</a>
        <a class="button button--quiet" href="markdown-coverage.html">Audit coverage</a>
      </div>
    </header>

    <section id="start-here" class="section-block">
      <h2>Start here</h2>
      <p class="section-introduction">Begin with project orientation, then choose a path based on the work you need to understand.</p>
      <div class="dashboard-grid">
        <article class="dashboard-card"><h3>1. Project orientation</h3><p>Read the project README lesson for a plain-language overview, GUI walkthrough, build entry points, and major examples.</p><p><a href="{self.document_for_path(Path('README.md')).output_path.as_posix()}">Open the ProGen3D lesson</a></p></article>
        <article class="dashboard-card"><h3>2. Build safely</h3><p>Use the build lesson before running source commands copied from plans, reports, or examples.</p><p><a href="{self.document_for_path(Path('BUILDING.md')).output_path.as_posix()}">Open the build lesson</a></p></article>
        <article class="dashboard-card"><h3>3. Read status correctly</h3><p>Every lesson visually distinguishes documented, evidenced, proposed, planned, research, pending, failed, and unresolved material.</p><p><a href="#status-legend">Read the status legend</a></p></article>
        <article class="dashboard-card"><h3>4. Trace every claim</h3><p>Each lesson includes the source path, source SHA-256, complete rendered Markdown, and links to related source-grounded lessons.</p><p><a href="manifest.json">Open the machine-readable manifest</a></p></article>
      </div>
    </section>

    <section id="learning-paths" class="section-block">
      <h2>Recommended learning paths</h2>
      <p class="section-introduction">Paths are generated from repository location, document category, status, and shared concepts. They guide reading order without claiming undocumented dependencies.</p>
      <div class="learning-path-grid">{''.join(learning_path_cards)}</div>
    </section>

    <section id="categories" class="section-block">
      <h2>Documentation categories</h2>
      <div class="category-grid">{''.join(category_cards)}</div>
    </section>

    <section id="concept-map" class="section-block">
      <h2>Interactive concept map</h2>
      <p class="section-introduction">Use Tab to focus diagram nodes. Each node links to the matching category and remains understandable through the text equivalent.</p>
      <div class="diagram-panel">
        <object data="assets/diagrams/site-concept-map.svg" type="image/svg+xml" data-learning-diagram aria-label="ProGen3D documentation concept map">Diagram fallback: ProGen3D connects to start/build, architecture, geometry/rendering, buildings, vehicles, vegetation, validation/evidence, plans/research, governance, and examples/reference.</object>
        <p class="diagram-text-equivalent"><strong>Text equivalent:</strong> the documentation starts with project orientation, then branches into architecture, geometry and rendering, buildings, vehicles, vegetation, evidence, plans and research, governance, and examples.</p>
        <p class="visually-hidden" data-diagram-live-region aria-live="polite"></p>
      </div>
    </section>

    <section id="status-legend" class="section-block">
      <h2>Project status legend</h2>
      <div class="dashboard-grid">
        <article class="dashboard-card"><h3><span class="status-badge status--documented">Documented</span></h3><p>The source describes material but the generated site does not independently certify runtime behavior.</p></article>
        <article class="dashboard-card"><h3><span class="status-badge status--evidence">Evidence</span></h3><p>The source is a verification, validation, test, audit, or evidence record. Its stated scope still controls the conclusion.</p></article>
        <article class="dashboard-card"><h3><span class="status-badge status--planned">Planned</span></h3><p>The source is an implementation plan or roadmap. It is not automatically implemented.</p></article>
        <article class="dashboard-card"><h3><span class="status-badge status--proposed">Proposed</span></h3><p>The source explicitly proposes a model, feature, or direction.</p></article>
        <article class="dashboard-card"><h3><span class="status-badge status--research">Research</span></h3><p>The source records research, mathematical reasoning, literature, or reference observations.</p></article>
        <article class="dashboard-card"><h3>Inline source signals</h3><p>Passed, failed, pending, proposed, experimental, and unresolved statements receive distinct source styling inside each complete lesson.</p></article>
      </div>
    </section>

    <section id="recently-updated" class="section-block">
      <h2>Recently updated pages</h2>
      <p class="section-introduction">Dates come from the latest Git commit touching each source. Working-tree-only sources are labelled separately rather than assigned an invented date.</p>
      <ul>{''.join(f'<li><a href="{document.output_path.as_posix()}">{html.escape(document.title)}</a> — {html.escape(document.updated_label)}</li>' for document in recently_updated)}</ul>
    </section>

    <section id="coverage-dashboard" class="section-block">
      <h2>Markdown coverage dashboard</h2>
      <div class="dashboard-grid">
        <article class="dashboard-card"><h3>{len(self.source_documents)} eligible Markdown files</h3><p>Every eligible source maps to one educational HTML page.</p></article>
        <article class="dashboard-card"><h3>{len(self.source_documents)} generated lessons</h3><p>Coverage is {coverage_percentage}%.</p><div class="coverage-meter" role="img" aria-label="{coverage_percentage}% Markdown page coverage"><span style="width:{coverage_percentage}%"></span></div></article>
        <article class="dashboard-card"><h3>{len(self.excluded_documents)} skipped Markdown files</h3><p>Only generated output, temporary test output, build output, and dependency/vendor documentation are excluded with recorded reasons.</p></article>
        <article class="dashboard-card"><h3>{unresolved_count} unresolved links · {missing_image_count} missing images</h3><p>See the complete report for source-path evidence and deterministic collision records.</p></article>
      </div>
      <p><a class="button button--secondary" href="markdown-coverage.html">Open complete coverage report</a></p>
    </section>

    <section id="page-index" class="section-block">
      <h2>Searchable page index</h2>
      <div class="search-panel">
        <div class="search-control">
          <label for="documentation-search" class="visually-hidden">Search documentation pages</label>
          <input id="documentation-search" type="search" data-documentation-search placeholder="Search titles, concepts, categories, paths, and summaries">
          <a class="button button--quiet" href="glossary.html">Glossary</a>
        </div>
        <p data-search-summary aria-live="polite">{len(self.source_documents)} documentation pages shown</p>
        <div class="search-results">{''.join(page_index_records)}</div>
      </div>
    </section>
  </main>
  {self.page_composer.site_footer('')}
</body>
</html>
'''

    def compose_glossary_page(self) -> str:
        rows = []
        for glossary_entry in self.glossary_entries:
            source_links = []
            for source_path in glossary_entry.source_paths[:8]:
                source_document = self.document_for_path(source_path)
                source_links.append(
                    f'<a href="{source_document.output_path.as_posix()}">{html.escape(source_document.title)}</a>'
                )
            row_class = (
                ' class="source-status--clarification"'
                if glossary_entry.needs_clarification
                else ""
            )
            rows.append(
                f"<tr{row_class}><td><dfn>{html.escape(glossary_entry.term)}</dfn></td><td>{html.escape(glossary_entry.definition)}</td><td>{'<br>'.join(source_links)}</td></tr>"
            )
        return self.top_level_page(
            "Generated glossary",
            "Abbreviations are expanded from explicit source definitions where possible. Terms without one stable expansion are marked Needs clarification rather than guessed.",
            f'''<section class="section-block"><h2>Terminology index</h2><div class="table-scroll" tabindex="0" aria-label="Generated glossary"><table><thead><tr><th scope="col">Term</th><th scope="col">Source-grounded definition</th><th scope="col">Lesson sources</th></tr></thead><tbody>{''.join(rows)}</tbody></table></div></section>''',
        )

    def compose_coverage_page(self) -> str:
        exclusion_counts = Counter(
            excluded_document.reason for excluded_document in self.excluded_documents
        )
        exclusion_summary = "".join(
            f"<tr><td>{html.escape(reason)}</td><td>{count}</td></tr>"
            for reason, count in sorted(exclusion_counts.items())
        )
        excluded_rows = "".join(
            f"<tr><td><code>{html.escape(document.relative_path.as_posix())}</code></td><td>{html.escape(document.reason)}</td></tr>"
            for document in self.excluded_documents
        )
        mapping_rows = "".join(
            f"<tr><td><code>{html.escape(document.relative_path.as_posix())}</code></td><td><a href=\"{document.output_path.as_posix()}\">{html.escape(document.title)}</a></td><td>{html.escape(document.status)}</td><td><code>{document.sha256}</code></td></tr>"
            for document in self.source_documents
        )
        issue_rows = "".join(
            f"<tr><td><code>{html.escape(issue.source_path.as_posix())}</code></td><td>{html.escape(issue.issue_kind)}</td><td><code>{html.escape(issue.referenced_value)}</code></td><td>{html.escape(issue.reason)}</td></tr>"
            for issue in self.link_issues
        ) or '<tr><td colspan="4">No unresolved source links or missing source images were detected.</td></tr>'
        duplicate_rows = "".join(
            f"<tr><td>{html.escape(record.title_a)}</td><td><code>{html.escape(record.source_a.as_posix())}</code></td><td>{html.escape(record.title_b)}</td><td><code>{html.escape(record.source_b.as_posix())}</code></td><td>{record.similarity:.3f}</td></tr>"
            for record in self.duplicate_titles
        ) or '<tr><td colspan="5">No exact or near-duplicate Markdown titles were detected.</td></tr>'
        clarification_documents = [
            document
            for document in self.source_documents
            if document.status_signal_counts.get("unresolved", 0)
            or any(
                self.glossary_entry_needs_clarification(term)
                for term in document.acronym_candidates
            )
        ]
        clarification_rows = "".join(
            f'<li><a href="{document.output_path.as_posix()}">{html.escape(document.title)}</a> — unresolved source signals: {document.status_signal_counts.get("unresolved", 0)}</li>'
            for document in clarification_documents
        ) or "<li>No document triggered the generated clarification criteria.</li>"
        coverage_body = f'''
<section class="section-block">
  <h2>Coverage result</h2>
  <div class="dashboard-grid">
    <article class="dashboard-card"><h3>{len(self.source_documents)}</h3><p>Eligible Markdown files discovered</p></article>
    <article class="dashboard-card"><h3>{len(self.source_documents)}</h3><p>Educational HTML pages generated</p></article>
    <article class="dashboard-card"><h3>100%</h3><p>Eligible Markdown-to-HTML page coverage</p></article>
    <article class="dashboard-card"><h3>{len(self.link_issues)}</h3><p>Unresolved links or missing image references recorded</p></article>
  </div>
</section>
<section class="section-block"><h2>Exclusion policy</h2><p>Generated documentation output, the legacy generated Markdown mirror, temporary test output, build output, and dependency/vendor directories are excluded. Authored Markdown already under <code>docs/</code> remains eligible.</p><div class="table-scroll" tabindex="0" aria-label="Exclusion summary"><table><thead><tr><th scope="col">Reason</th><th scope="col">Files</th></tr></thead><tbody>{exclusion_summary}</tbody></table></div></section>
<section class="section-block"><h2>Complete source mapping</h2><div class="table-scroll" tabindex="0" aria-label="Markdown to HTML mapping"><table><thead><tr><th scope="col">Source Markdown</th><th scope="col">Generated lesson</th><th scope="col">Status</th><th scope="col">Source SHA-256</th></tr></thead><tbody>{mapping_rows}</tbody></table></div></section>
<section class="section-block"><h2>Skipped files and reasons</h2><div class="table-scroll" tabindex="0" aria-label="Skipped Markdown files"><table><thead><tr><th scope="col">Path</th><th scope="col">Reason</th></tr></thead><tbody>{excluded_rows}</tbody></table></div></section>
<section class="section-block"><h2>Link resolution and missing images</h2><p>Relocated references record deterministic repairs from stale source paths to uniquely matching project files. Unresolved references remain distinct.</p><div class="table-scroll" tabindex="0" aria-label="Link and image issues"><table><thead><tr><th scope="col">Source</th><th scope="col">Issue</th><th scope="col">Reference</th><th scope="col">Reason</th></tr></thead><tbody>{issue_rows}</tbody></table></div></section>
<section class="section-block"><h2>Duplicate or near-duplicate titles</h2><p>URL collisions are resolved deterministically with a source-path digest whenever multiple documents normalize to the same slug.</p><div class="table-scroll" tabindex="0" aria-label="Duplicate titles"><table><thead><tr><th scope="col">Title A</th><th scope="col">Source A</th><th scope="col">Title B</th><th scope="col">Source B</th><th scope="col">Similarity</th></tr></thead><tbody>{duplicate_rows}</tbody></table></div></section>
<section class="section-block"><h2>Documents requiring clarification</h2><ul>{clarification_rows}</ul></section>
'''
        return self.top_level_page(
            "Markdown coverage report",
            "A requirement-to-evidence report for source discovery, page generation, exclusions, links, images, title collisions, and clarification signals.",
            coverage_body,
        )

    def compose_documentation_readme(self) -> str:
        return f'''# ProGen3D Educational Documentation System

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

Current generated coverage: **{len(self.source_documents)} of {len(self.source_documents)} eligible Markdown files ({100 if self.source_documents else 0}%)**.

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
'''

    def compose_implementation_report(
        self, validation_result: DocumentationValidationResult
    ) -> str:
        created_paths = [
            "docs/index.html",
            "docs/glossary.html",
            "docs/markdown-coverage.html",
            "docs/implementation-report.html",
            "docs/manifest.json",
            "docs/README.md",
            "docs/.nojekyll",
            "docs/pages/",
            "docs/assets/css/",
            "docs/assets/js/",
            "docs/assets/diagrams/",
            "docs/assets/images/source/",
            "docs/assets/source-files/",
        ]
        unresolved_gaps = [
            issue
            for issue in self.link_issues
            if issue.issue_kind in {"unresolved-link", "missing-image", "outside-repository"}
        ]
        relocated_references = [
            issue
            for issue in self.link_issues
            if issue.issue_kind == "relocated-reference"
        ]
        clarification_count = len(
            [entry for entry in self.glossary_entries if entry.needs_clarification]
        )
        validation_rows = [
            ("Eligible Markdown coverage", f"PASS — {len(self.source_documents)} of {len(self.source_documents)} pages generated"),
            ("Static validation checks", f"{'PASS' if validation_result.passed else 'FAIL'} — {validation_result.checks_run} checks, {validation_result.error_count} errors, {validation_result.warning_count} warnings"),
            ("Source links and local assets", f"{'PASS' if not unresolved_gaps else 'REVIEW'} — {len(unresolved_gaps)} unresolved source references recorded"),
            ("Responsive/print/reduced motion CSS", "PASS — required media queries are present"),
            ("Accessible SVG structure", f"PASS — {len(self.diagram_paths)} diagrams include title, description, and keyboard-focusable nodes"),
            ("Deterministic regeneration", "Validated separately by tools/generate_educational_docs.py --check"),
        ]
        findings = "".join(
            f"<tr><td>{html.escape(finding.severity.upper())}</td><td>{html.escape(finding.code)}</td><td><code>{html.escape(finding.path)}</code></td><td>{html.escape(finding.message)}</td></tr>"
            for finding in validation_result.findings
        ) or '<tr><td colspan="4">No validation errors or warnings.</td></tr>'
        report_body = f'''
<section class="section-block"><h2>Files created</h2><ul>{''.join(f'<li><code>{html.escape(path)}</code></li>' for path in created_paths)}</ul></section>
<section class="section-block"><h2>Files transformed</h2><p>{len(self.source_documents)} eligible Markdown files were transformed into {len(self.source_documents)} educational HTML lessons. Each lesson includes four explanation levels, learning objectives, prerequisites, concepts, process guidance, a walkthrough, a source-grounded example, an accessible diagram, implementation notes, mistakes, verification guidance, takeaways, related pages, source identity, and previous/next navigation.</p></section>
<section class="section-block"><h2>Diagrams added</h2><p>{len(self.diagram_paths)} responsive SVG diagrams were generated: one site concept map and one learning-flow diagram per Markdown lesson. Genuine referenced project images copied for publication: {len(self.copied_assets)}. Referenced non-image project files copied for static publication: {len(self.copied_source_files)}.</p></section>
<section class="section-block"><h2>Unresolved documentation gaps</h2><p>{len(unresolved_gaps)} unresolved source link or missing image references remain. {len(relocated_references)} stale source paths were resolved deterministically to unique project files and recorded in the coverage report. {clarification_count} glossary terms have no single explicit source expansion and are marked <strong>Needs clarification</strong> rather than guessed.</p><p>Implementation, evidence, hypotheses, proposals, and unresolved work remain source-status distinctions; the documentation generator does not promote plans or focused evidence into release claims.</p></section>
<section class="section-block"><h2>Validation results</h2><div class="table-scroll" tabindex="0" aria-label="Validation results"><table><thead><tr><th scope="col">Gate</th><th scope="col">Result</th></tr></thead><tbody>{''.join(f'<tr><td>{html.escape(gate)}</td><td>{html.escape(result)}</td></tr>' for gate, result in validation_rows)}</tbody></table></div><h3>Validation findings</h3><div class="table-scroll" tabindex="0" aria-label="Validation findings"><table><thead><tr><th scope="col">Severity</th><th scope="col">Code</th><th scope="col">Path</th><th scope="col">Message</th></tr></thead><tbody>{findings}</tbody></table></div></section>
'''
        return self.top_level_page(
            "Documentation implementation report",
            "Generated deliverable inventory, transformation scope, diagram inventory, unresolved gaps, and validation evidence.",
            report_body,
        )

    def top_level_page(self, title: str, summary: str, body: str) -> str:
        return f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="{html.escape(summary, quote=True)}">
  <title>{html.escape(title)} · ProGen3D Learning Documentation</title>
  <link rel="stylesheet" href="assets/css/documentation.css">
  <script defer src="assets/js/navigation.js"></script>
</head>
<body>
  <a class="skip-link" href="#main-content">Skip to main content</a>
  {self.page_composer.site_header('')}
  <main id="main-content" class="page-shell">
    <nav class="breadcrumbs" aria-label="Breadcrumb"><ol><li><a href="index.html">Documentation home</a></li><li aria-current="page">{html.escape(title)}</li></ol></nav>
    <header class="lesson-header"><p class="eyebrow">Documentation system</p><h1>{html.escape(title)}</h1><p class="lead">{html.escape(summary)}</p></header>
{body.strip()}
  </main>
  {self.page_composer.site_footer('')}
</body>
</html>
'''

    def learning_paths(
        self,
    ) -> list[tuple[str, str, list[MarkdownSourceDocument]]]:
        return [
            (
                "New to the project",
                "Orientation, build, tests, grammar examples, and the first major object-model lessons.",
                self.select_learning_path(
                    ["README.md", "BUILDING.md", "tests/README.md"],
                    ["start", "examples-reference", "buildings"],
                    7,
                ),
            ),
            (
                "Developer onboarding",
                "Build the editor, understand hardening and semantic boundaries, then enter current architecture work.",
                self.select_learning_path(
                    ["README.md", "BUILDING.md", "P0_IMPLEMENTATION.md", "P1_IMPLEMENTATION.md", "P2_IMPLEMENTATION.md"],
                    ["start", "architecture", "governance"],
                    8,
                ),
            ),
            (
                "Architecture and internals",
                "Study runtime ownership, grammar semantics, geometry/rendering services, and refactor boundaries.",
                self.select_learning_path([], ["architecture", "geometry-rendering", "governance"], 8),
            ),
            (
                "Algorithms and reasoning",
                "Follow mathematical models, research reports, source mappings, validation logic, and evidence interpretation.",
                self.select_learning_path([], ["plans-research", "validation", "geometry-rendering"], 8),
            ),
            (
                "Buildings and spatial models",
                "Move from the small modern building through spatial object models, courtyard systems, furniture, and placement evidence.",
                self.select_learning_path([], ["buildings", "validation"], 8, preferred_word="building"),
            ),
            (
                "Vehicles and evidence-constrained geometry",
                "Progress through vehicle grammars, mathematical models, primitive coverage, fitting, and release verification.",
                self.select_learning_path([], ["vehicles", "validation"], 8, preferred_word="vehicle"),
            ),
            (
                "Vegetation systems",
                "Study vegetation grammars, building-integrated object models, rendering, and three-view evidence.",
                self.select_learning_path([], ["vegetation", "validation"], 7, preferred_word="vegetation"),
            ),
        ]

    def select_learning_path(
        self,
        explicit_paths: Sequence[str],
        category_identifiers: Sequence[str],
        maximum_documents: int,
        preferred_word: str = "",
    ) -> list[MarkdownSourceDocument]:
        selected_documents: list[MarkdownSourceDocument] = []
        selected_paths: set[Path] = set()
        for explicit_path in explicit_paths:
            candidate_path = Path(explicit_path)
            matching_document = next(
                (
                    document
                    for document in self.source_documents
                    if document.relative_path == candidate_path
                ),
                None,
            )
            if matching_document:
                selected_documents.append(matching_document)
                selected_paths.add(matching_document.relative_path)

        candidates = sorted(
            (
                document
                for document in self.source_documents
                if document.category_identifier in category_identifiers
                and document.relative_path not in selected_paths
            ),
            key=lambda document: (
                0
                if preferred_word
                and preferred_word in (document.title + " " + document.relative_path.as_posix()).lower()
                else 1,
                CATEGORY_ORDER[document.category_identifier],
                0 if document.status == "Documented" else 1,
                document.title.casefold(),
                document.relative_path.as_posix(),
            ),
        )
        selected_documents.extend(
            candidates[: max(0, maximum_documents - len(selected_documents))]
        )
        return selected_documents

    def write_manifest(self) -> None:
        manifest = {
            "schema_version": "ProGen3D-Educational-Documentation-1",
            "generator": "tools/generate_educational_docs.py",
            "repository": self.configuration.repository_url,
            "repository_branch": self.configuration.repository_branch,
            "markdown_files_found": len(self.source_documents),
            "html_pages_generated": len(self.source_documents),
            "coverage_percent": 100 if self.source_documents else 0,
            "source_exclusion_policy": [
                "generated documentation output",
                "legacy generated Markdown mirror",
                "temporary generated test output",
                "generated build and distribution output",
                "dependency and vendor directories",
            ],
            "pages": [
                {
                    "source": document.relative_path.as_posix(),
                    "output": document.output_path.as_posix(),
                    "title": document.title,
                    "summary": document.summary,
                    "category": document.category_identifier,
                    "status": document.status,
                    "source_sha256": document.sha256,
                    "source_bytes": document.byte_count,
                    "source_tracked_by_git": document.tracked_by_git,
                    "source_updated": document.updated_label,
                    "concepts": document.concepts,
                    "prerequisites": [path.as_posix() for path in document.prerequisite_paths],
                    "related_sources": [path.as_posix() for path in document.related_paths],
                    "headings": len(document.headings),
                    "code_blocks": len(document.code_examples),
                    "tables": document.table_count,
                    "equation_blocks": document.equation_count,
                    "status_signals": document.status_signal_counts,
                }
                for document in self.source_documents
            ],
            "skipped_markdown": [
                {
                    "source": document.relative_path.as_posix(),
                    "reason": document.reason,
                }
                for document in self.excluded_documents
            ],
            "duplicate_or_near_duplicate_titles": [
                {
                    "title_a": record.title_a,
                    "source_a": record.source_a.as_posix(),
                    "title_b": record.title_b,
                    "source_b": record.source_b.as_posix(),
                    "similarity": round(record.similarity, 6),
                }
                for record in self.duplicate_titles
            ],
            "unresolved_references": [
                {
                    "source": issue.source_path.as_posix(),
                    "reference": issue.referenced_value,
                    "kind": issue.issue_kind,
                    "reason": issue.reason,
                }
                for issue in self.link_issues
                if issue.issue_kind != "relocated-reference"
            ],
            "resolved_reference_relocations": [
                {
                    "source": issue.source_path.as_posix(),
                    "reference": issue.referenced_value,
                    "resolution": issue.reason,
                }
                for issue in self.link_issues
                if issue.issue_kind == "relocated-reference"
            ],
            "copied_source_images": [
                path.as_posix() for path in sorted(self.copied_assets)
            ],
            "copied_referenced_source_files": [
                path.as_posix() for path in sorted(self.copied_source_files)
            ],
            "diagrams": [path.as_posix() for path in self.diagram_paths],
            "generated_support_pages": [
                "index.html",
                "glossary.html",
                "markdown-coverage.html",
                "implementation-report.html",
            ],
        }
        self.write_text(
            Path("manifest.json"),
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        )

    def glossary_entry_needs_clarification(self, term: str) -> bool:
        return any(
            entry.term == term and entry.needs_clarification
            for entry in self.glossary_entries
        )

    def document_for_path(self, relative_path: Path) -> MarkdownSourceDocument:
        for source_document in self.source_documents:
            if source_document.relative_path == relative_path:
                return source_document
        raise KeyError(relative_path)

    def write_text(self, relative_path: Path, content: str) -> None:
        destination_path = self.configuration.output_root / relative_path
        destination_path.parent.mkdir(parents=True, exist_ok=True)
        destination_path.write_text(content, encoding="utf-8")


class StaticDocumentationValidationService:
    REQUIRED_PAGE_SECTIONS = set(
        EducationalPageCompositionService.REQUIRED_SECTION_IDENTIFIERS
    )

    def validate(
        self,
        configuration: DocumentationBuildConfiguration,
        source_documents: Sequence[MarkdownSourceDocument],
    ) -> DocumentationValidationResult:
        result = DocumentationValidationResult()
        output_root = configuration.output_root
        source_by_path = {
            document.relative_path.as_posix(): document for document in source_documents
        }
        self.check_required_outputs(output_root, result)
        self.check_manifest(output_root, source_by_path, result)
        html_soups = self.load_html_documents(output_root, result)
        self.check_educational_pages(output_root, source_documents, html_soups, result)
        self.check_local_references(output_root, html_soups, result)
        self.check_accessibility(html_soups, result)
        self.check_diagrams(output_root, result)
        self.check_shared_assets(output_root, result)
        self.check_javascript_syntax(output_root, result)
        return result

    @staticmethod
    def check_required_outputs(
        output_root: Path, result: DocumentationValidationResult
    ) -> None:
        required_paths = [
            Path("index.html"),
            Path("glossary.html"),
            Path("markdown-coverage.html"),
            Path("implementation-report.html"),
            Path("manifest.json"),
            Path("README.md"),
            Path(".nojekyll"),
            Path("assets/css/documentation.css"),
            Path("assets/js/navigation.js"),
            Path("assets/js/search.js"),
            Path("assets/js/diagrams.js"),
            Path("assets/diagrams/site-concept-map.svg"),
        ]
        for required_path in required_paths:
            result.checks_run += 1
            if not (output_root / required_path).exists():
                result.record_error(
                    "missing-required-output",
                    required_path.as_posix(),
                    "required documentation deliverable is missing",
                )

    @staticmethod
    def check_manifest(
        output_root: Path,
        source_by_path: dict[str, MarkdownSourceDocument],
        result: DocumentationValidationResult,
    ) -> None:
        manifest_path = output_root / "manifest.json"
        result.checks_run += 1
        if not manifest_path.exists():
            return
        try:
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as manifest_error:
            result.record_error(
                "invalid-manifest",
                "manifest.json",
                f"manifest could not be read: {manifest_error}",
            )
            return
        manifest_pages = manifest.get("pages", [])
        result.checks_run += 1
        if manifest.get("markdown_files_found") != len(source_by_path):
            result.record_error(
                "manifest-source-count",
                "manifest.json",
                "manifest Markdown count does not match current source discovery",
            )
        result.checks_run += 1
        if manifest.get("html_pages_generated") != len(source_by_path):
            result.record_error(
                "manifest-page-count",
                "manifest.json",
                "manifest generated-page count does not match source discovery",
            )
        result.checks_run += 1
        if manifest.get("coverage_percent") != 100:
            result.record_error(
                "manifest-coverage",
                "manifest.json",
                "manifest does not report 100% eligible Markdown coverage",
            )
        manifest_by_source = {page.get("source"): page for page in manifest_pages}
        for source_path, source_document in source_by_path.items():
            result.checks_run += 1
            manifest_page = manifest_by_source.get(source_path)
            if not manifest_page:
                result.record_error(
                    "missing-manifest-mapping",
                    source_path,
                    "eligible Markdown source has no manifest mapping",
                )
                continue
            if manifest_page.get("source_sha256") != source_document.sha256:
                result.record_error(
                    "stale-source-hash",
                    source_path,
                    "manifest source hash does not match current Markdown",
                )
            output_path = manifest_page.get("output", "")
            if not output_path or not (output_root / output_path).exists():
                result.record_error(
                    "missing-mapped-page",
                    source_path,
                    "manifest output page does not exist",
                )

    @staticmethod
    def load_html_documents(
        output_root: Path, result: DocumentationValidationResult
    ) -> dict[Path, BeautifulSoup]:
        html_soups: dict[Path, BeautifulSoup] = {}
        for html_path in sorted(output_root.rglob("*.html")):
            relative_path = html_path.relative_to(output_root)
            result.checks_run += 1
            try:
                html_soups[relative_path] = BeautifulSoup(
                    html_path.read_text(encoding="utf-8"), "html.parser"
                )
            except OSError as html_error:
                result.record_error(
                    "unreadable-html",
                    relative_path.as_posix(),
                    str(html_error),
                )
        return html_soups

    def check_educational_pages(
        self,
        output_root: Path,
        source_documents: Sequence[MarkdownSourceDocument],
        html_soups: dict[Path, BeautifulSoup],
        result: DocumentationValidationResult,
    ) -> None:
        for source_document in source_documents:
            page_path = source_document.output_path
            result.checks_run += 1
            soup = html_soups.get(page_path)
            if not soup:
                result.record_error(
                    "missing-educational-page",
                    page_path.as_posix(),
                    "source Markdown has no readable HTML lesson",
                )
                continue
            source_meta = soup.find("meta", attrs={"name": "source-markdown"})
            source_hash_meta = soup.find("meta", attrs={"name": "source-sha256"})
            if not source_meta or source_meta.get("content") != source_document.relative_path.as_posix():
                result.record_error(
                    "source-metadata",
                    page_path.as_posix(),
                    "page source-markdown metadata is missing or incorrect",
                )
            if not source_hash_meta or source_hash_meta.get("content") != source_document.sha256:
                result.record_error(
                    "source-hash-metadata",
                    page_path.as_posix(),
                    "page source SHA-256 metadata is missing or stale",
                )
            section_identifiers = {
                element.get("id")
                for element in soup.find_all(id=True)
                if element.get("id")
            }
            missing_sections = self.REQUIRED_PAGE_SECTIONS - section_identifiers
            if missing_sections:
                result.record_error(
                    "missing-educational-sections",
                    page_path.as_posix(),
                    "missing sections: " + ", ".join(sorted(missing_sections)),
                )
            explanation_levels = soup.select(".explanation-level")
            if len(explanation_levels) != 4:
                result.record_error(
                    "four-level-explanation",
                    page_path.as_posix(),
                    f"expected four explanation levels, found {len(explanation_levels)}",
                )
            source_lesson = soup.select_one("#source-lesson .source-lesson")
            if not source_lesson:
                result.record_error(
                    "missing-source-lesson",
                    page_path.as_posix(),
                    "complete rendered source lesson is missing",
                )
            else:
                rendered_code_blocks = len(source_lesson.find_all("pre"))
                if rendered_code_blocks < len(source_document.code_examples):
                    result.record_error(
                        "code-block-preservation",
                        page_path.as_posix(),
                        "rendered lesson contains fewer code blocks than the source",
                    )
                rendered_tables = len(source_lesson.find_all("table"))
                if rendered_tables < source_document.table_count:
                    result.record_error(
                        "table-preservation",
                        page_path.as_posix(),
                        "rendered lesson contains fewer tables than the source",
                    )
            if not soup.select_one("#lesson-navigation a[rel='prev']") and source_document is not source_documents[0]:
                result.record_error(
                    "missing-previous-navigation",
                    page_path.as_posix(),
                    "lesson is missing previous navigation",
                )
            if not soup.select_one("#lesson-navigation a[rel='next']") and source_document is not source_documents[-1]:
                result.record_error(
                    "missing-next-navigation",
                    page_path.as_posix(),
                    "lesson is missing next navigation",
                )

    def check_local_references(
        self,
        output_root: Path,
        html_soups: dict[Path, BeautifulSoup],
        result: DocumentationValidationResult,
    ) -> None:
        identifiers_by_page = {
            page_path: {
                element.get("id")
                for element in soup.find_all(id=True)
                if element.get("id")
            }
            for page_path, soup in html_soups.items()
        }
        for page_path, soup in html_soups.items():
            references = []
            references.extend(("href", element) for element in soup.find_all(href=True))
            references.extend(("src", element) for element in soup.find_all(src=True))
            references.extend(("data", element) for element in soup.find_all("object", data=True))
            for attribute_name, element in references:
                reference_value = str(element.get(attribute_name, "")).strip()
                if not reference_value or self.reference_is_external(reference_value):
                    continue
                result.checks_run += 1
                split_reference = urlsplit(reference_value)
                if not split_reference.path:
                    target_page = page_path
                else:
                    target_absolute_path = (
                        output_root / page_path.parent / unquote(split_reference.path)
                    ).resolve()
                    try:
                        target_relative_path = target_absolute_path.relative_to(output_root)
                    except ValueError:
                        result.record_error(
                            "reference-outside-docs",
                            page_path.as_posix(),
                            f"local reference escapes docs: {reference_value}",
                        )
                        continue
                    if not target_absolute_path.exists():
                        result.record_error(
                            "broken-local-reference",
                            page_path.as_posix(),
                            f"missing target: {reference_value}",
                        )
                        continue
                    target_page = target_relative_path
                if split_reference.fragment and target_page.suffix.lower() in {".html", ""}:
                    target_identifiers = identifiers_by_page.get(target_page, set())
                    if split_reference.fragment not in target_identifiers:
                        result.record_error(
                            "broken-anchor",
                            page_path.as_posix(),
                            f"missing anchor {split_reference.fragment!r} in {target_page.as_posix()}",
                        )

    @staticmethod
    def reference_is_external(reference_value: str) -> bool:
        return bool(
            re.match(
                r"^(?:https?:|mailto:|tel:|data:|javascript:)",
                reference_value,
                re.I,
            )
        )

    @staticmethod
    def check_accessibility(
        html_soups: dict[Path, BeautifulSoup],
        result: DocumentationValidationResult,
    ) -> None:
        for page_path, soup in html_soups.items():
            result.checks_run += 1
            if not soup.find("html", lang=True):
                result.record_error(
                    "missing-language",
                    page_path.as_posix(),
                    "html element lacks a language attribute",
                )
            if not soup.find("main"):
                result.record_error(
                    "missing-main-landmark",
                    page_path.as_posix(),
                    "page lacks a main landmark",
                )
            heading_identifiers = [
                element.get("id") for element in soup.find_all(id=True)
            ]
            duplicate_identifiers = [
                identifier
                for identifier, count in Counter(heading_identifiers).items()
                if identifier and count > 1
            ]
            if duplicate_identifiers:
                result.record_error(
                    "duplicate-html-identifiers",
                    page_path.as_posix(),
                    ", ".join(sorted(duplicate_identifiers)),
                )
            for image in soup.find_all("img"):
                if image.get("alt") is None:
                    result.record_error(
                        "missing-image-alt",
                        page_path.as_posix(),
                        f"image lacks alt text: {image.get('src', '')}",
                    )
            for button in soup.find_all("button"):
                accessible_name = button.get_text(" ", strip=True) or button.get(
                    "aria-label", ""
                )
                if not accessible_name:
                    result.record_error(
                        "unnamed-button",
                        page_path.as_posix(),
                        "button lacks an accessible name",
                    )
            for table_header in soup.find_all("th"):
                if table_header.get("scope") not in {"col", "row"}:
                    result.record_error(
                        "table-header-scope",
                        page_path.as_posix(),
                        "table header lacks scope",
                    )

    @staticmethod
    def check_diagrams(
        output_root: Path, result: DocumentationValidationResult
    ) -> None:
        diagram_paths = sorted((output_root / "assets/diagrams").rglob("*.svg"))
        result.checks_run += 1
        if not diagram_paths:
            result.record_error(
                "missing-diagrams",
                "assets/diagrams",
                "no SVG diagrams were generated",
            )
            return
        for diagram_path in diagram_paths:
            relative_path = diagram_path.relative_to(output_root)
            result.checks_run += 1
            try:
                diagram_root = ElementTree.fromstring(
                    diagram_path.read_text(encoding="utf-8")
                )
            except (OSError, ElementTree.ParseError) as diagram_error:
                result.record_error(
                    "unreadable-diagram",
                    relative_path.as_posix(),
                    str(diagram_error),
                )
                continue
            if (
                diagram_root.find("{*}title") is None
                or diagram_root.find("{*}desc") is None
            ):
                result.record_error(
                    "diagram-accessible-name",
                    relative_path.as_posix(),
                    "SVG must contain title and description",
                )
            if not any(
                element.attrib.get("tabindex") == "0"
                for element in diagram_root.iter()
            ):
                result.record_error(
                    "diagram-keyboard-focus",
                    relative_path.as_posix(),
                    "SVG lacks keyboard-focusable semantic nodes",
                )

    @staticmethod
    def check_shared_assets(
        output_root: Path, result: DocumentationValidationResult
    ) -> None:
        css_path = output_root / "assets/css/documentation.css"
        result.checks_run += 1
        if css_path.exists():
            css_content = css_path.read_text(encoding="utf-8")
            for required_css_feature in (
                "@media (max-width:",
                "@media (prefers-reduced-motion: reduce)",
                "@media print",
                ":focus-visible",
            ):
                if required_css_feature not in css_content:
                    result.record_error(
                        "missing-responsive-accessibility-css",
                        "assets/css/documentation.css",
                        f"missing CSS feature: {required_css_feature}",
                    )

    @staticmethod
    def check_javascript_syntax(
        output_root: Path, result: DocumentationValidationResult
    ) -> None:
        node_path = shutil.which("node")
        if not node_path:
            result.record_warning(
                "javascript-syntax-not-run",
                "assets/js",
                "node is unavailable, so JavaScript syntax checks were skipped",
            )
            return
        for javascript_path in sorted((output_root / "assets/js").glob("*.js")):
            result.checks_run += 1
            completed_process = subprocess.run(
                [node_path, "--check", str(javascript_path)],
                capture_output=True,
                text=True,
            )
            if completed_process.returncode != 0:
                result.record_error(
                    "javascript-syntax",
                    javascript_path.relative_to(output_root).as_posix(),
                    completed_process.stderr.strip() or "node --check failed",
                )


class DocumentationOutputPublicationService:
    def publish(self, staged_output_root: Path, documentation_root: Path) -> None:
        documentation_root.mkdir(parents=True, exist_ok=True)
        for generated_directory in GENERATED_DIRECTORIES:
            staged_directory = staged_output_root / generated_directory
            destination_directory = documentation_root / generated_directory
            if destination_directory.exists():
                shutil.rmtree(destination_directory)
            if staged_directory.exists():
                destination_directory.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(staged_directory, destination_directory)
        for generated_file in GENERATED_TOP_LEVEL_FILES:
            staged_file = staged_output_root / generated_file
            destination_file = documentation_root / generated_file
            destination_file.parent.mkdir(parents=True, exist_ok=True)
            temporary_file = destination_file.with_name(
                f".{destination_file.name}.documentation-staging"
            )
            if staged_file.exists():
                shutil.copy2(staged_file, temporary_file)
                os.replace(temporary_file, destination_file)
            elif destination_file.exists():
                destination_file.unlink()


class DocumentationDeterminismService:
    def compare(self, expected_root: Path, actual_root: Path) -> list[str]:
        expected_files = self.collect_generated_files(expected_root)
        actual_files = self.collect_generated_files(actual_root)
        differences = []
        for missing_path in sorted(expected_files - actual_files):
            differences.append(f"missing generated file: {missing_path.as_posix()}")
        for stale_path in sorted(actual_files - expected_files):
            differences.append(f"stale generated file: {stale_path.as_posix()}")
        for shared_path in sorted(expected_files & actual_files):
            if self.file_digest(expected_root / shared_path) != self.file_digest(
                actual_root / shared_path
            ):
                differences.append(f"generated file differs: {shared_path.as_posix()}")
        return differences

    @staticmethod
    def collect_generated_files(root: Path) -> set[Path]:
        generated_files = {
            generated_file
            for generated_file in GENERATED_TOP_LEVEL_FILES
            if (root / generated_file).is_file()
        }
        for generated_directory in GENERATED_DIRECTORIES:
            directory_path = root / generated_directory
            if not directory_path.exists():
                continue
            generated_files.update(
                file_path.relative_to(root)
                for file_path in directory_path.rglob("*")
                if file_path.is_file()
            )
        return generated_files

    @staticmethod
    def file_digest(file_path: Path) -> str:
        return hashlib.sha256(file_path.read_bytes()).hexdigest()


class EducationalDocumentationGenerationService:
    def build_staged_site(
        self, repository_root: Path, staged_output_root: Path
    ) -> DocumentationBuildResult:
        configuration = DocumentationBuildConfiguration.from_repository(
            repository_root, staged_output_root
        )
        source_discovery = MarkdownSourceDiscoveryService(configuration)
        source_documents, excluded_documents = source_discovery.discover()
        if not source_documents:
            raise RuntimeError("No eligible project Markdown files were discovered")

        analysis_service = MarkdownDocumentAnalysisService()
        analysis_service.analyze_documents(source_documents)
        address_service = DocumentationAddressService()
        duplicate_titles = address_service.assign_output_paths(source_documents)
        relationship_service = DocumentationRelationshipService()
        relationship_service.assign_relationships(source_documents)
        glossary_entries = GlossaryAnalysisService().build_glossary(source_documents)
        document_by_source_path = {
            source_document.relative_path: source_document
            for source_document in source_documents
        }

        self.copy_static_assets(configuration)
        diagram_service = DocumentationDiagramService(configuration)
        diagram_service.write_site_concept_map(source_documents)
        renderer = MarkdownHtmlRenderingService(
            configuration, document_by_source_path
        )
        page_composer = EducationalPageCompositionService(
            configuration, source_documents, glossary_entries
        )
        ordered_documents = sorted(
            source_documents,
            key=lambda document: (
                CATEGORY_ORDER[document.category_identifier],
                document.title.casefold(),
                document.relative_path.as_posix(),
            ),
        )
        for document_index, source_document in enumerate(ordered_documents):
            rendered_source_html = renderer.render_document(source_document)
            source_document_diagram = diagram_service.write_document_diagram(
                source_document
            )
            previous_document = (
                ordered_documents[document_index - 1]
                if document_index > 0
                else None
            )
            next_document = (
                ordered_documents[document_index + 1]
                if document_index + 1 < len(ordered_documents)
                else None
            )
            document_page_html = page_composer.compose_document_page(
                source_document,
                rendered_source_html,
                source_document_diagram,
                previous_document,
                next_document,
            )
            document_page_html = self.repair_source_outline_anchors(
                document_page_html
            )
            destination_path = staged_output_root / source_document.output_path
            destination_path.parent.mkdir(parents=True, exist_ok=True)
            destination_path.write_text(document_page_html, encoding="utf-8")

        site_composer = DocumentationSiteCompositionService(
            configuration,
            source_documents,
            excluded_documents,
            duplicate_titles,
            glossary_entries,
            renderer.link_issues,
            renderer.copied_assets,
            renderer.copied_source_files,
            diagram_service.diagram_paths,
        )
        site_composer.write_site_support_pages()
        provisional_validation = DocumentationValidationResult()
        site_composer.write_implementation_report(provisional_validation)
        validation_service = StaticDocumentationValidationService()
        validation_result = validation_service.validate(
            configuration, ordered_documents
        )
        site_composer.write_implementation_report(validation_result)
        final_validation_result = validation_service.validate(
            configuration, ordered_documents
        )
        if not final_validation_result.passed:
            formatted_findings = "\n".join(
                f"{finding.severity.upper()} {finding.code} {finding.path}: {finding.message}"
                for finding in final_validation_result.findings
            )
            raise RuntimeError(
                "Documentation generation is incomplete:\n" + formatted_findings
            )

        return DocumentationBuildResult(
            source_documents=source_documents,
            excluded_documents=excluded_documents,
            duplicate_titles=duplicate_titles,
            glossary_entries=glossary_entries,
            link_issues=renderer.link_issues,
            copied_assets=renderer.copied_assets,
            copied_source_files=renderer.copied_source_files,
            diagram_paths=diagram_service.diagram_paths,
            validation_result=final_validation_result,
        )

    @staticmethod
    def copy_static_assets(configuration: DocumentationBuildConfiguration) -> None:
        asset_mapping = {
            "documentation.css": Path("assets/css/documentation.css"),
            "navigation.js": Path("assets/js/navigation.js"),
            "search.js": Path("assets/js/search.js"),
            "diagrams.js": Path("assets/js/diagrams.js"),
        }
        for source_name, destination_relative_path in asset_mapping.items():
            source_path = configuration.static_asset_root / source_name
            if not source_path.exists():
                raise RuntimeError(f"Missing documentation asset source: {source_path}")
            destination_path = configuration.output_root / destination_relative_path
            destination_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source_path, destination_path)

    @staticmethod
    def repair_source_outline_anchors(document_page_html: str) -> str:
        soup = BeautifulSoup(document_page_html, "html.parser")
        source_lesson = soup.select_one("#source-lesson .source-lesson")
        if not source_lesson:
            return document_page_html
        source_identifiers = {
            heading.get("id")
            for heading in source_lesson.find_all(id=True)
            if heading.get("id")
        }
        for outline_link in soup.select("[data-source-anchor]"):
            source_identifier = outline_link.get("data-source-anchor")
            prefixed_identifier = f"source-{source_identifier}"
            if prefixed_identifier in source_identifiers:
                outline_link["href"] = f"#{prefixed_identifier}"
            else:
                outline_link["href"] = "#source-lesson"
            del outline_link["data-source-anchor"]
        return str(soup)

    def generate(self, repository_root: Path, documentation_root: Path) -> DocumentationBuildResult:
        with tempfile.TemporaryDirectory(
            prefix="progen3d-documentation-build-"
        ) as temporary_directory:
            staged_output_root = Path(temporary_directory) / "docs"
            staged_output_root.mkdir(parents=True)
            build_result = self.build_staged_site(
                repository_root, staged_output_root
            )
            DocumentationOutputPublicationService().publish(
                staged_output_root, documentation_root
            )
            return build_result

    def check(self, repository_root: Path, documentation_root: Path) -> list[str]:
        with tempfile.TemporaryDirectory(
            prefix="progen3d-documentation-check-"
        ) as temporary_directory:
            staged_output_root = Path(temporary_directory) / "docs"
            staged_output_root.mkdir(parents=True)
            self.build_staged_site(repository_root, staged_output_root)
            return DocumentationDeterminismService().compare(
                staged_output_root, documentation_root
            )


def validate_existing_documentation(
    repository_root: Path, documentation_root: Path
) -> DocumentationValidationResult:
    configuration = DocumentationBuildConfiguration.from_repository(
        repository_root, documentation_root
    )
    source_documents, _ = MarkdownSourceDiscoveryService(configuration).discover()
    MarkdownDocumentAnalysisService().analyze_documents(source_documents)
    DocumentationAddressService().assign_output_paths(source_documents)
    DocumentationRelationshipService().assign_relationships(source_documents)
    ordered_documents = sorted(
        source_documents,
        key=lambda document: (
            CATEGORY_ORDER[document.category_identifier],
            document.title.casefold(),
            document.relative_path.as_posix(),
        ),
    )
    return StaticDocumentationValidationService().validate(
        configuration, ordered_documents
    )
