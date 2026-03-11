#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
from typing import List, Optional
import re


_HEADING_RE = re.compile(r"^(#{1,6})\s+(.*\S)\s*$")
_FENCE_RE = re.compile(r"^\s*(```|~~~)")
_TABLE_SEPARATOR_RE = re.compile(r"^\|\s*:?-{3,}:?\s*\|\s*:?-{3,}:?\s*\|\s*$")
_GLOSSARY_HEADER_RE = re.compile(r"^\|\s*term\s*\|\s*definition\s*\|\s*$", re.IGNORECASE)
_GLOSSARY_ROW_RE = re.compile(r"^\|\s*(.+?)\s*\|\s*(.+?)\s*\|\s*$")
_GT_FACT_RE = re.compile(r"^\s*-\s*`?(GT-\d{3})`?\s*(.*)$")


@dataclass
class MarkdownChunk:
    path: str
    chunk_id: str
    heading_path: List[str]
    heading_level: int
    title: str
    content: str
    text_for_embedding: str


class MarkdownChunker:
    """
    Chunk markdown documents by heading/subheading blocks.

    Rules:
    - Each heading starts a new section.
    - Section content continues until the next heading of the same or higher level.
    - Child subsection content belongs to the child chunk, not duplicated into the parent.
    - Headings inside fenced code blocks are ignored as section boundaries.
    - Heading hierarchy is preserved in `heading_path`.
    - Heading text is included in `text_for_embedding`.
    - Template/noise chunks are filtered.
    - Selected large sections are split into finer retrieval chunks.
    """

    def __init__(
        self,
        min_content_chars: int = 0,
        min_body_chars: int = 20,
        drop_template_chunks: bool = True,
        split_large_sections: bool = True,
    ) -> None:
        self.min_content_chars = min_content_chars
        self.min_body_chars = min_body_chars
        self.drop_template_chunks = drop_template_chunks
        self.split_large_sections = split_large_sections

    def chunk_file(self, file_path: str | Path) -> List[MarkdownChunk]:
        path = Path(file_path)
        text = path.read_text(encoding="utf-8")
        return self.chunk_text(text=text, path=str(path))

    def chunk_text(self, text: str, path: str = "<memory>") -> List[MarkdownChunk]:
        lines = text.splitlines()
        sections = self._parse_sections(lines)
        sections = self._expand_sections_for_retrieval(path=path, sections=sections)
        chunks: List[MarkdownChunk] = []

        for section in sections:
            cleaned_content = self._normalize_content(section["content"])

            if len(cleaned_content.strip()) < self.min_content_chars:
                continue

            heading_path = section["heading_path"]
            if self._should_skip_chunk(heading_path=heading_path, content=cleaned_content):
                continue

            title = heading_path[-1] if heading_path else "Document"
            heading_level = section["level"]

            text_for_embedding = self._build_embedding_text(
                path=path,
                heading_path=heading_path,
                content=cleaned_content,
            )

            chunks.append(
                MarkdownChunk(
                    path=path,
                    chunk_id=f"{Path(path).stem}-{len(chunks):04d}",
                    heading_path=heading_path,
                    heading_level=heading_level,
                    title=title,
                    content=cleaned_content,
                    text_for_embedding=text_for_embedding,
                )
            )

        return chunks

    def _parse_sections(self, lines: List[str]) -> List[dict]:
        """
        Parse markdown into heading-based sections.

        Returns a list of dicts:
        {
            "level": int,
            "heading_path": List[str],
            "content": str,
        }
        """
        sections: List[dict] = []

        current_stack: List[dict] = []
        current_section: Optional[dict] = None
        preamble_lines: List[str] = []
        in_fence = False

        for line in lines:
            heading_match = None if in_fence else _HEADING_RE.match(line)
            is_fence_line = bool(_FENCE_RE.match(line))

            if heading_match:
                level = len(heading_match.group(1))
                heading_text = heading_match.group(2).strip()

                if current_section is not None:
                    sections.append(current_section)
                    current_section = None

                while current_stack and current_stack[-1]["level"] >= level:
                    current_stack.pop()

                current_stack.append({
                    "level": level,
                    "title": heading_text,
                })

                heading_path = [item["title"] for item in current_stack]

                current_section = {
                    "level": level,
                    "heading_path": heading_path,
                    "content_lines": [self._render_heading_path(current_stack)],
                }
            else:
                if current_section is not None:
                    current_section["content_lines"].append(line)
                else:
                    preamble_lines.append(line)

            if is_fence_line:
                in_fence = not in_fence

        if current_section is not None:
            sections.append(current_section)

        finalized_sections: List[dict] = []

        preamble_text = self._normalize_content("\n".join(preamble_lines))
        if preamble_text.strip():
            finalized_sections.append({
                "level": 0,
                "heading_path": ["Document"],
                "content": preamble_text,
            })

        for section in sections:
            finalized_sections.append({
                "level": section["level"],
                "heading_path": section["heading_path"],
                "content": self._normalize_content("\n".join(section["content_lines"])),
            })

        return finalized_sections

    def _expand_sections_for_retrieval(self, path: str, sections: List[dict]) -> List[dict]:
        if not self.split_large_sections:
            return sections

        expanded: List[dict] = []
        file_name = Path(path).name.lower()

        for section in sections:
            heading_path = section["heading_path"]
            if not heading_path:
                expanded.append(section)
                continue

            leaf_title = heading_path[-1].strip().lower()

            if file_name == "glossary.md" and leaf_title == "rag usage notes":
                expanded.extend(self._split_glossary_usage_notes(section))
                continue

            if file_name == "rag_ground_truth.md" and leaf_title == "canonical facts":
                expanded.extend(self._split_canonical_facts(section))
                continue

            expanded.append(section)

        return expanded

    def _split_glossary_usage_notes(self, section: dict) -> List[dict]:
        body_lines = self._extract_section_body_lines(section)
        table_header_index = self._find_glossary_table_header(body_lines)

        if table_header_index is None:
            return [section]

        intro_lines = body_lines[:table_header_index]
        rows = self._parse_glossary_rows(body_lines[table_header_index:])

        if not rows:
            return [section]

        result: List[dict] = []
        heading_block = self._render_heading_path_from_titles(
            heading_path=section["heading_path"],
            current_level=section["level"],
        )

        intro_text = self._normalize_content("\n".join(intro_lines))
        if intro_text:
            result.append(
                {
                    "level": section["level"],
                    "heading_path": section["heading_path"],
                    "content": self._normalize_content(f"{heading_block}\n\n{intro_text}"),
                }
            )

        sub_level = min(6, section["level"] + 1)
        for term, definition in rows:
            sub_heading = f"Term: {term}"
            sub_path = section["heading_path"] + [sub_heading]
            sub_heading_block = self._render_heading_path_from_titles(
                heading_path=sub_path,
                current_level=sub_level,
            )
            row_body = f"Glossary term: `{term}`\nDefinition: {definition}"
            result.append(
                {
                    "level": sub_level,
                    "heading_path": sub_path,
                    "content": self._normalize_content(f"{sub_heading_block}\n\n{row_body}"),
                }
            )

        return result

    def _split_canonical_facts(self, section: dict) -> List[dict]:
        body_lines = self._extract_section_body_lines(section)

        first_fact_index: Optional[int] = None
        fact_lines: List[str] = []
        for index, line in enumerate(body_lines):
            if _GT_FACT_RE.match(line.strip()):
                if first_fact_index is None:
                    first_fact_index = index
                fact_lines.append(line.strip())

        if first_fact_index is None or not fact_lines:
            return [section]

        result: List[dict] = []
        heading_block = self._render_heading_path_from_titles(
            heading_path=section["heading_path"],
            current_level=section["level"],
        )

        intro_text = self._normalize_content("\n".join(body_lines[:first_fact_index]))
        if intro_text:
            result.append(
                {
                    "level": section["level"],
                    "heading_path": section["heading_path"],
                    "content": self._normalize_content(f"{heading_block}\n\n{intro_text}"),
                }
            )

        sub_level = min(6, section["level"] + 1)
        for fact_line in fact_lines:
            match = _GT_FACT_RE.match(fact_line)
            if not match:
                continue
            fact_id = match.group(1)
            sub_path = section["heading_path"] + [fact_id]
            sub_heading_block = self._render_heading_path_from_titles(
                heading_path=sub_path,
                current_level=sub_level,
            )
            fact_body = f"Fact ID: {fact_id}\n{fact_line}"
            result.append(
                {
                    "level": sub_level,
                    "heading_path": sub_path,
                    "content": self._normalize_content(f"{sub_heading_block}\n\n{fact_body}"),
                }
            )

        return result

    def _find_glossary_table_header(self, lines: List[str]) -> Optional[int]:
        for index, line in enumerate(lines):
            if _GLOSSARY_HEADER_RE.match(line.strip()):
                return index
        return None

    def _parse_glossary_rows(self, lines: List[str]) -> List[tuple[str, str]]:
        rows: List[tuple[str, str]] = []
        in_table = False

        for line in lines:
            stripped = line.strip()
            if not stripped:
                if in_table:
                    break
                continue

            if not stripped.startswith("|"):
                if in_table:
                    break
                continue

            if _GLOSSARY_HEADER_RE.match(stripped):
                in_table = True
                continue

            if _TABLE_SEPARATOR_RE.match(stripped):
                in_table = True
                continue

            match = _GLOSSARY_ROW_RE.match(stripped)
            if not match:
                if in_table:
                    break
                continue

            term = match.group(1).strip().strip("`")
            definition = match.group(2).strip()
            if not term or not definition:
                continue

            rows.append((term, definition))
            in_table = True

        return rows

    def _extract_section_body_lines(self, section: dict) -> List[str]:
        content_lines = section["content"].splitlines()
        heading_lines = self._render_heading_path_from_titles(
            heading_path=section["heading_path"],
            current_level=section["level"],
        ).splitlines()

        if (
            heading_lines
            and len(content_lines) >= len(heading_lines)
            and content_lines[: len(heading_lines)] == heading_lines
        ):
            return content_lines[len(heading_lines):]

        index = 0
        while index < len(content_lines) and _HEADING_RE.match(content_lines[index]):
            index += 1
        return content_lines[index:]

    @staticmethod
    def _render_heading_path_from_titles(heading_path: List[str], current_level: int) -> str:
        if not heading_path:
            return ""

        start_level = max(1, current_level - len(heading_path) + 1)
        rendered: List[str] = []
        for index, title in enumerate(heading_path):
            level = min(6, start_level + index)
            rendered.append(f"{'#' * level} {title}")
        return "\n".join(rendered)

    def _should_skip_chunk(self, heading_path: List[str], content: str) -> bool:
        title = heading_path[-1] if heading_path else "Document"
        body = self._remove_heading_lines(content)

        if len(body) < self.min_body_chars:
            return True

        if self.drop_template_chunks:
            lower_title = title.strip().lower()
            if "template" in lower_title:
                return True
            if "<" in title and ">" in title:
                return True

        return False

    @staticmethod
    def _remove_heading_lines(content: str) -> str:
        body_lines = [line for line in content.splitlines() if not _HEADING_RE.match(line)]
        body = "\n".join(body_lines)
        body = body.replace("```", "").replace("~~~", "")
        return body.strip()

    @staticmethod
    def _render_heading_path(stack: List[dict]) -> str:
        """
        Include the full heading trail in the chunk text.

        Example:
        # API
        ## POST /notes
        """
        rendered = []
        for item in stack:
            hashes = "#" * item["level"]
            rendered.append(f"{hashes} {item['title']}")
        return "\n".join(rendered)

    @staticmethod
    def _normalize_content(text: str) -> str:
        text = text.replace("\r\n", "\n").replace("\r", "\n")

        # Collapse excessive blank lines, but keep paragraph separation.
        text = re.sub(r"\n{3,}", "\n\n", text)

        return text.strip()

    @staticmethod
    def _build_embedding_text(path: str, heading_path: List[str], content: str) -> str:
        heading_str = " > ".join(heading_path) if heading_path else "Document"

        return (
            f"path: {path}\n"
            f"type: markdown_section\n"
            f"heading_path: {heading_str}\n\n"
            f"{content}"
        )
    

'''
DEBUGGING
'''

import json
import sys
from dataclasses import asdict
from pathlib import Path
from typing import Iterable

def iter_markdown_files(input_dir: Path) -> Iterable[Path]:
    for path in sorted(input_dir.rglob("*.md")):
        if path.is_file():
            yield path


def make_output_file_name(input_root: Path, file_path: Path) -> str:
    """
    Example:
        docs/architecture.md
    becomes:
        docs__architecture.md.json

    This helps avoid collisions between files with the same name
    in different subdirectories.
    """
    relative_path = file_path.relative_to(input_root)
    safe_name = "__".join(relative_path.parts)
    return f"{safe_name}.json"


def export_chunks_for_file(
    chunker: MarkdownChunker,
    input_root: Path,
    file_path: Path,
    output_dir: Path,
) -> None:
    chunks = chunker.chunk_file(file_path)

    output_payload = {
        "source_file": str(file_path),
        "chunk_count": len(chunks),
        "chunks": [asdict(chunk) for chunk in chunks],
    }

    output_file = output_dir / make_output_file_name(input_root, file_path)
    output_file.parent.mkdir(parents=True, exist_ok=True)

    with output_file.open("w", encoding="utf-8") as f:
        json.dump(output_payload, f, indent=2, ensure_ascii=False)

    print(f"[OK] {file_path} -> {output_file} ({len(chunks)} chunks)")


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage:")
        print("    python main.py <input_markdown_dir> <output_json_dir>")
        print("")
        print("Example:")
        print("    python main.py ./docs ./chunk_debug")
        return 1

    input_dir = Path(sys.argv[1]).resolve()
    output_dir = Path(sys.argv[2]).resolve()

    if not input_dir.exists():
        print(f"[ERROR] Input directory does not exist: {input_dir}")
        return 1

    if not input_dir.is_dir():
        print(f"[ERROR] Input path is not a directory: {input_dir}")
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)

    chunker = MarkdownChunker(min_content_chars=0)

    markdown_files = list(iter_markdown_files(input_dir))
    if not markdown_files:
        print(f"[INFO] No markdown files found in: {input_dir}")
        return 0

    for file_path in markdown_files:
        export_chunks_for_file(
            chunker=chunker,
            input_root=input_dir,
            file_path=file_path,
            output_dir=output_dir,
        )

    print("")
    print(f"Done. Exported {len(markdown_files)} markdown file(s) into: {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
