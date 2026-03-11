from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

from .generator import OllamaGenerator
from ..prompt_builder.prompt_builder import build_rag_prompt
from ..retriever.retrieval import RagRetriever


@dataclass
class AssistantAnswer:
    answer: str
    sources: list[dict[str, Any]]
    retrieved_chunks: list[dict[str, Any]]


class RagAssistantService:
    def __init__(
        self,
        persist_dir: str = "data/chroma_db",
        collection_name: str = "project_docs",
        embedding_model: str = "all-minilm",
        generation_model: str = "meta/Llama-4-Scout-17B-16E-Instruct",
        generation_timeout: int = 120,
    ) -> None:
        self.retriever = RagRetriever(
            persist_dir=persist_dir,
            collection_name=collection_name,
            embedding_model=embedding_model,
        )
        self.generator = OllamaGenerator(
            model=generation_model,
            timeout=generation_timeout,
        )

    @staticmethod
    def _build_source_list(used_chunks: list[dict[str, Any]]) -> list[dict[str, Any]]:
        sources: list[dict[str, Any]] = []

        for idx, chunk in enumerate(used_chunks, start=1):
            metadata = chunk.get("metadata", {}) or {}
            sources.append(
                {
                    "source_id": idx,
                    "source_path": metadata.get("source_path", "unknown"),
                    "title": metadata.get("title", "unknown"),
                    "start_offset": metadata.get("start_offset", 0),
                    "end_offset": metadata.get("end_offset", 0),
                    "distance": chunk.get("distance"),
                    "chunk_id": chunk.get("chunk_id"),
                }
            )

        return sources

    def answer_question(
        self,
        query: str,
        top_k: int = 5,
        max_context_chars: int = 6000,
        max_chunk_chars: int = 1400,
        stream: bool = False,
        on_token: Callable[[str], None] | None = None,
    ) -> AssistantAnswer:
        retrieved = self.retriever.retrieve(query=query, top_k=top_k)

        prompt = build_rag_prompt(
            user_query=query,
            retrieved_chunks=retrieved,
            max_context_chars=max_context_chars,
            max_chunk_chars=max_chunk_chars,
        )

        generation = self.generator.chat(
            system_prompt=prompt.system_prompt,
            user_prompt=prompt.user_prompt,
            stream=stream,
            on_token=on_token,
        )

        sources = self._build_source_list(prompt.used_chunks)

        return AssistantAnswer(
            answer=generation.answer,
            sources=sources,
            retrieved_chunks=retrieved,
        )
