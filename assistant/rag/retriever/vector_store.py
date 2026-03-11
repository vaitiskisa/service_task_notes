from __future__ import annotations

from typing import Any, Optional

import chromadb
from chromadb.api.models.Collection import Collection

from .types import Chunk


def _sanitize_metadata(chunk: Chunk) -> dict[str, Any]:
    """
    Chroma metadata should stay flat and simple.
    Keep only scalar values that are useful for filtering/debugging.
    """
    metadata: dict[str, Any] = {
        "source_path": chunk.source_path,
        "title": chunk.title,
        "start_offset": int(chunk.start_offset),
        "end_offset": int(chunk.end_offset),
        "token_estimate": int(chunk.token_estimate),
    }

    for key, value in chunk.metadata.items():
        if isinstance(value, (str, int, float, bool)) or value is None:
            metadata[key] = value

    return metadata


class ChromaVectorStore:
    def __init__(
        self,
        persist_dir: str = "data/chroma_db",
        collection_name: str = "project_docs",
    ) -> None:
        self.persist_dir = persist_dir
        self.collection_name = collection_name

        self.client = chromadb.PersistentClient(path=persist_dir)
        self.collection: Collection = self.client.get_or_create_collection(
            name=collection_name
        )

    def reset_collection(self) -> None:
        """
        Recreate the collection from scratch.
        Useful for a full rebuild of the RAG index.
        """
        try:
            self.client.delete_collection(name=self.collection_name)
        except Exception:
            pass

        self.collection = self.client.get_or_create_collection(
            name=self.collection_name
        )

    def add_chunks(
        self,
        chunks: list[Chunk],
        embeddings: list[list[float]],
        batch_size: int = 128,
    ) -> None:
        if len(chunks) != len(embeddings):
            raise ValueError("chunks and embeddings length mismatch")

        if not chunks:
            return

        ids = [chunk.chunk_id for chunk in chunks]
        documents = [chunk.text for chunk in chunks]
        metadatas = [_sanitize_metadata(chunk) for chunk in chunks]

        for i in range(0, len(chunks), batch_size):
            self.collection.add(
                ids=ids[i:i + batch_size],
                documents=documents[i:i + batch_size],
                embeddings=embeddings[i:i + batch_size],
                metadatas=metadatas[i:i + batch_size],
            )

    def query(
        self,
        query_embedding: list[float],
        top_k: int = 5,
        where: Optional[dict[str, Any]] = None,
        where_document: Optional[dict[str, Any]] = None,
    ) -> list[dict[str, Any]]:
        result = self.collection.query(
            query_embeddings=[query_embedding],
            n_results=top_k,
            where=where,
            where_document=where_document,
        )

        ids = result.get("ids", [[]])[0]
        docs = result.get("documents", [[]])[0]
        metadatas = result.get("metadatas", [[]])[0]
        distances = result.get("distances", [[]])[0]

        output: list[dict[str, Any]] = []

        for chunk_id, doc, metadata, distance in zip(ids, docs, metadatas, distances):
            item = {
                "chunk_id": chunk_id,
                "text": doc,
                "metadata": metadata or {},
                "distance": distance,
            }
            output.append(item)

        return output

    def get_all(
        self,
        where: Optional[dict[str, Any]] = None,
        where_document: Optional[dict[str, Any]] = None,
    ) -> list[dict[str, Any]]:
        result = self.collection.get(
            where=where,
            where_document=where_document,
            include=["documents", "metadatas"],
        )

        ids = result.get("ids", [])
        docs = result.get("documents", [])
        metadatas = result.get("metadatas", [])

        output: list[dict[str, Any]] = []
        for chunk_id, doc, metadata in zip(ids, docs, metadatas):
            output.append(
                {
                    "chunk_id": chunk_id,
                    "text": doc,
                    "metadata": metadata or {},
                    "distance": None,
                }
            )

        return output

    def get_count(self) -> int:
        return self.collection.count()
