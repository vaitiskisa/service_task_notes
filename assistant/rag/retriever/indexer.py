import sys

from .chunker import chunk_documents
from .document_loader import load_documents
from .embedder import OllamaEmbedder
from .vector_store import ChromaVectorStore


class InlineProgress:
    def __init__(self, width: int = 30) -> None:
        self.width = width
        self.stream = sys.stdout
        self.enabled = self.stream.isatty()
        self._last_line_len = 0
        self._active_line = False

    def update(
        self,
        label: str,
        current: int,
        total: int,
        detail: str | None = None,
    ) -> None:
        progress_total = total if total > 0 else 1
        bounded_current = min(max(current, 0), progress_total)
        ratio = bounded_current / progress_total
        filled = int(self.width * ratio)
        bar = "#" * filled + "-" * (self.width - filled)
        percent = int(ratio * 100)

        line = f"{label}: [{bar}] {current}/{total} ({percent:3d}%)"
        if detail:
            line += f" | {detail}"

        if self.enabled:
            padding = " " * max(0, self._last_line_len - len(line))
            self.stream.write("\r" + line + padding)
            self.stream.flush()
            self._last_line_len = len(line)
            self._active_line = True

    def finish_stage(self) -> None:
        if self.enabled and self._active_line:
            self.stream.write("\n")
            self.stream.flush()
            self._last_line_len = 0
            self._active_line = False


def build_rag_index(
    docs_root: str = "src/docs",
    persist_dir: str = "data/chroma_db",
    collection_name: str = "project_docs",
    embedding_model: str = "embeddinggemma",
    embedding_batch_size: int = 16,
    embedding_timeout: int = 60,
    db_batch_size: int = 128,
    reset: bool = True,
) -> None:
    progress = InlineProgress()

    documents = load_documents(
        docs_root,
        on_progress=lambda done, total: progress.update("Loading docs", done, total),
    )
    progress.finish_stage()

    chunks = chunk_documents(
        documents,
        on_progress=lambda done, total, count: progress.update(
            "Generating chunks",
            done,
            total,
            detail=f"chunks={count}",
        ),
    )
    progress.finish_stage()

    if not chunks:
        raise RuntimeError("No chunks generated from docs")

    embedder = OllamaEmbedder(
        model=embedding_model,
        timeout=embedding_timeout,
    )
    embeddings = embedder.embed_in_batches(
        [chunk.text for chunk in chunks],
        batch_size=embedding_batch_size,
        on_progress=lambda done, total: progress.update("Embedding chunks", done, total),
    )
    progress.finish_stage()

    if len(embeddings) != len(chunks):
        raise RuntimeError("Embedding count does not match chunk count")

    store = ChromaVectorStore(
        persist_dir=persist_dir,
        collection_name=collection_name,
    )

    if reset:
        store.reset_collection()

    store.add_chunks(
        chunks=chunks,
        embeddings=embeddings,
        batch_size=db_batch_size,
    )

    print(f"Loaded documents: {len(documents)}")
    print(f"Generated chunks: {len(chunks)}")
    print(f"Stored chunks in Chroma: {store.get_count()}")
    print(f"Persist dir: {persist_dir}")
    print(f"Collection: {collection_name}")
