from __future__ import annotations

import re
from typing import Any, Optional

from .embedder import OllamaEmbedder
from .vector_store import ChromaVectorStore

HTTP_METHODS = {"get", "post", "put", "delete", "patch", "head", "options"}


def _extract_endpoint_paths(query: str) -> list[str]:
    return [m.group(0).lower() for m in re.finditer(r"/[a-zA-Z0-9_/{}/-]+", query)]


def _extract_http_methods(query: str) -> set[str]:
    words = re.findall(r"[a-zA-Z]+", query.lower())
    return {w for w in words if w in HTTP_METHODS}


def _tokenize_for_overlap(text: str) -> set[str]:
    return set(re.findall(r"[a-z0-9_/{}/-]+", text.lower()))


def _normalize_query_for_embedding(query: str) -> str:
    """
    Keep original query while appending routing hints so endpoint-like
    queries (e.g. "/notes post") embed into a more meaningful semantic space.
    """
    paths = _extract_endpoint_paths(query)
    methods = sorted(_extract_http_methods(query))

    hints: list[str] = []
    if paths:
        hints.append("api route " + " ".join(paths))
    if methods:
        hints.append("http method " + " ".join(methods))

    if not hints:
        return query

    return f"{query} {' '.join(hints)}"


def _is_api_query(query: str) -> bool:
    return bool(_extract_endpoint_paths(query) or _extract_http_methods(query))


def _lexical_relevance_score(query: str, item: dict[str, Any]) -> float:
    metadata = item.get("metadata", {}) or {}
    text = (item.get("text", "") or "").lower()
    source_path = str(metadata.get("source_path", "") or "").lower()
    title = str(metadata.get("title", "") or "").lower()
    haystack = f"{source_path}\n{title}\n{text}"

    query_lower = query.lower().strip()
    query_tokens = _tokenize_for_overlap(query_lower)
    haystack_tokens = _tokenize_for_overlap(haystack)

    token_overlap = 0.0
    if query_tokens:
        token_overlap = len(query_tokens & haystack_tokens) / len(query_tokens)

    phrase_bonus = 0.0
    if query_lower and query_lower in haystack:
        phrase_bonus = 0.15

    paths = _extract_endpoint_paths(query_lower)
    methods = _extract_http_methods(query_lower)

    path_bonus = 0.0
    matched_paths = 0
    if paths:
        matched_paths = sum(1 for path in paths if path in haystack)
        path_bonus = 0.3 * (matched_paths / len(paths))

    method_bonus = 0.0
    matched_methods = 0
    if methods:
        matched_methods = sum(1 for method in methods if re.search(rf"\b{method}\b", haystack))
        method_bonus = 0.2 * (matched_methods / len(methods))

    api_query = bool(paths or methods)
    source_bias = 0.0
    if api_query:
        if source_path.endswith("openapi.yaml"):
            source_bias = 1.1
        elif source_path.endswith("api.md"):
            source_bias = 0.35
        elif source_path.endswith("architecture.md"):
            source_bias = -0.15

    strict_match_bonus = 0.0
    if api_query:
        all_paths_matched = (not paths) or (matched_paths == len(paths))
        all_methods_matched = (not methods) or (matched_methods == len(methods))
        if all_paths_matched and all_methods_matched:
            strict_match_bonus = 0.6

    return (
        token_overlap
        + phrase_bonus
        + path_bonus
        + method_bonus
        + source_bias
        + strict_match_bonus
    )


def _top_lexical_candidates(
    query: str,
    items: list[dict[str, Any]],
    top_n: int,
) -> list[dict[str, Any]]:
    scored: list[tuple[float, int, dict[str, Any]]] = []
    for index, item in enumerate(items):
        lexical = _lexical_relevance_score(query, item)
        scored.append((lexical, index, item))
    scored.sort(key=lambda row: (-row[0], row[1]))
    return [item for _, _, item in scored[:top_n]]


def _merge_candidates(
    semantic_candidates: list[dict[str, Any]],
    lexical_candidates: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    merged: dict[str, dict[str, Any]] = {}

    for item in lexical_candidates:
        merged[item["chunk_id"]] = item

    for item in semantic_candidates:
        existing = merged.get(item["chunk_id"])
        if not existing:
            merged[item["chunk_id"]] = item
            continue

        existing_distance = existing.get("distance")
        incoming_distance = item.get("distance")
        if existing_distance is None and incoming_distance is not None:
            existing["distance"] = incoming_distance

    return list(merged.values())


def _hybrid_rerank(query: str, candidates: list[dict[str, Any]]) -> list[dict[str, Any]]:
    scored: list[tuple[float, int, dict[str, Any]]] = []

    for index, item in enumerate(candidates):
        distance = item.get("distance")
        semantic = 0.0
        if isinstance(distance, (int, float)):
            # Distance is lower-is-better; map to a bounded score.
            semantic = 1.0 / (1.0 + max(0.0, float(distance)))

        lexical = _lexical_relevance_score(query, item)
        score = (0.35 * semantic) + (0.65 * lexical)
        scored.append((score, index, item))

    scored.sort(key=lambda row: (-row[0], row[1]))
    return [item for _, _, item in scored]


class RagRetriever:
    def __init__(
        self,
        persist_dir: str = "data/chroma_db",
        collection_name: str = "project_docs",
        embedding_model: str = "all-minilm",
    ) -> None:
        self.store = ChromaVectorStore(
            persist_dir=persist_dir,
            collection_name=collection_name,
        )
        self.embedder = OllamaEmbedder(model=embedding_model)

    def retrieve(
        self,
        query: str,
        top_k: int = 5,
        where: Optional[dict[str, Any]] = None,
        where_document: Optional[dict[str, Any]] = None,
    ) -> list[dict[str, Any]]:
        candidate_count = max(top_k * 8, top_k)
        query_embedding = self.embedder.embed_query(_normalize_query_for_embedding(query))
        semantic_candidates = self.store.query(
            query_embedding=query_embedding,
            top_k=candidate_count,
            where=where,
            where_document=where_document,
        )
        candidates = semantic_candidates

        if _is_api_query(query):
            all_chunks = self.store.get_all(
                where=where,
                where_document=where_document,
            )
            lexical_candidates = _top_lexical_candidates(
                query=query,
                items=all_chunks,
                top_n=candidate_count,
            )
            candidates = _merge_candidates(
                semantic_candidates=semantic_candidates,
                lexical_candidates=lexical_candidates,
            )

        ranked = _hybrid_rerank(query=query, candidates=candidates)
        return ranked[:top_k]
