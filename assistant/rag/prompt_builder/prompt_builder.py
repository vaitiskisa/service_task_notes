from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class PromptBuildResult:
    system_prompt: str
    user_prompt: str
    used_chunks: list[dict[str, Any]]


def _format_source_label(chunk: dict[str, Any], idx: int) -> str:
    metadata = chunk.get("metadata", {}) or {}
    source_path = metadata.get("source_path", "unknown")
    title = metadata.get("title", "unknown")
    start_offset = metadata.get("start_offset", 0)
    end_offset = metadata.get("end_offset", 0)

    return (
        f"[SOURCE {idx}] "
        f"path={source_path} | title={title} | offsets={start_offset}-{end_offset}"
    )


def _truncate_text(text: str, max_chars: int) -> str:
    if len(text) <= max_chars:
        return text
    return text[:max_chars].rstrip() + "\n...[truncated]"
    

def build_rag_prompt(
    user_query: str,
    retrieved_chunks: list[dict[str, Any]],
    max_context_chars: int = 6000,
    max_chunk_chars: int = 1400,
) -> PromptBuildResult:
    """
    Build a grounded prompt from retrieved chunks.

    max_context_chars:
        Hard cap for total retrieved context inserted into the prompt.

    max_chunk_chars:
        Per-chunk cap so one large chunk does not dominate the context.
    """
    used_chunks: list[dict[str, Any]] = []
    context_parts: list[str] = []
    current_chars = 0

    for idx, chunk in enumerate(retrieved_chunks, start=1):
        raw_text = chunk.get("text", "") or ""
        if not raw_text.strip():
            continue

        clipped = _truncate_text(raw_text.strip(), max_chunk_chars)
        source_label = _format_source_label(chunk, idx)
        block = f"{source_label}\n{clipped}"

        projected = current_chars + len(block) + 2
        if projected > max_context_chars:
            break

        context_parts.append(block)
        used_chunks.append(chunk)
        current_chars = projected

    context_text = "\n\n".join(context_parts)

    system_prompt = (
        "You are a project assistant for a local code/documentation chatbot.\n"
        "Answer using the provided context whenever possible.\n"
        "Do not invent project-specific facts that are not supported by the context.\n"
        "If the context is insufficient, say so clearly.\n"
        "When you rely on context, cite sources inline using [SOURCE N].\n"
        "Prefer precise, implementation-oriented answers."
    )

    user_prompt = (
        "Use the following retrieved project context to answer the question.\n\n"
        f"{context_text}\n\n"
        f"Question: {user_query}\n\n"
        "Return a direct answer. "
        "When referring to retrieved information, cite with [SOURCE N]."
    )

    return PromptBuildResult(
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        used_chunks=used_chunks,
    )