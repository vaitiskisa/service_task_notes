from __future__ import annotations

import hashlib
import re
from pathlib import Path
from typing import Any, Callable

from .chunking import chunker_yaml
from .chunking.chunker_markdown import MarkdownChunker
from .types import Chunk, SourceDocument


def _estimate_tokens(text: str) -> int:
    # crude estimate, enough for metadata/debugging
    return max(1, len(text) // 4)


def _make_chunk_id(source_path: str, start: int, end: int, text: str) -> str:
    raw = f"{source_path}:{start}:{end}:{text}".encode("utf-8", errors="ignore")
    return hashlib.sha1(raw).hexdigest()

def _chunk_openapi_yaml_document(document: SourceDocument) -> list[Chunk]:
    """
    Convert OpenAPI-specific chunker output into the generic Chunk model
    used by the vector store/indexer.
    """
    spec = chunker_yaml.load_yaml_file(Path(document.path))
    raw_chunks = chunker_yaml.extract_all_chunks(
        spec=spec,
        source_path=document.path,
    )

    chunks: list[Chunk] = []
    cursor = 0

    for raw in raw_chunks:
        text = str(raw.get("text", "") or "").strip()
        if not text:
            continue

        raw_metadata = raw.get("metadata", {}) or {}
        metadata: dict[str, Any]
        if isinstance(raw_metadata, dict):
            metadata = dict(raw_metadata)
        else:
            metadata = {}

        chunk_id = str(raw.get("id") or _make_chunk_id(document.path, cursor, cursor + len(text), text))
        title = str(raw.get("title") or metadata.get("title") or document.title)
        source_path = str(metadata.get("source_path") or document.path)

        metadata.setdefault("source_path", source_path)
        metadata.setdefault("title", title)
        metadata.setdefault("yaml_chunk_type", raw.get("chunk_type"))
        metadata.setdefault("yaml_chunk_id", chunk_id)

        start = cursor
        end = start + len(text)
        cursor = end + 1

        chunks.append(
            Chunk(
                chunk_id=chunk_id,
                source_path=source_path,
                title=title,
                text=text,
                start_offset=start,
                end_offset=end,
                token_estimate=_estimate_tokens(text),
                metadata=metadata,
            )
        )

    return chunks


def _chunk_markdown_document(document: SourceDocument) -> list[Chunk]:
    """
    Convert markdown-specific chunker output into the generic Chunk model
    used by the vector store/indexer.
    """
    markdown_chunker = MarkdownChunker()
    raw_chunks = markdown_chunker.chunk_text(
        text=document.content,
        path=document.path,
    )

    chunks: list[Chunk] = []
    cursor = 0

    for raw in raw_chunks:
        text = str(raw.text_for_embedding or "").strip()
        if not text:
            continue

        heading_path = " > ".join(raw.heading_path) if raw.heading_path else "Document"
        metadata: dict[str, Any] = {
            "source_path": document.path,
            "title": raw.title,
            "doc_type": "markdown_section",
            "markdown_heading_path": heading_path,
            "markdown_heading_level": int(raw.heading_level),
            "markdown_chunk_id": raw.chunk_id,
        }

        start = cursor
        end = start + len(text)
        cursor = end + 1

        chunks.append(
            Chunk(
                chunk_id=_make_chunk_id(document.path, start, end, text),
                source_path=document.path,
                title=raw.title,
                text=text,
                start_offset=start,
                end_offset=end,
                token_estimate=_estimate_tokens(text),
                metadata=metadata,
            )
        )

    return chunks


def chunk_documents(
    documents: list[SourceDocument],
    on_progress: Callable[[int, int, int], None] | None = None,
) -> list[Chunk]:
    result: list[Chunk] = []
    total_docs = len(documents)
    if on_progress:
        on_progress(0, total_docs, 0)

    for index, doc in enumerate(documents, start=1):
        if doc.suffix == ".yaml":
            try:
                yaml_chunks = _chunk_openapi_yaml_document(doc)
            except Exception:
                yaml_chunks = []

            if yaml_chunks:
                result.extend(yaml_chunks)
                if on_progress:
                    on_progress(index, total_docs, len(result))
                continue
        elif doc.suffix == ".md":
            try:
                markdown_chunks = _chunk_markdown_document(doc)
            except Exception:
                markdown_chunks = []

            if markdown_chunks:
                result.extend(markdown_chunks)
                if on_progress:
                    on_progress(index, total_docs, len(result))
                continue
        else:
            print(f"Warning: Unrecognized document suffix '{doc.suffix}' for {doc.path}, skipping chunking")
        if on_progress:
            on_progress(index, total_docs, len(result))
    return result
