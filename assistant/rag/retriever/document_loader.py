from pathlib import Path
from typing import Callable
from .types import SourceDocument

SUPPORTED_SUFFIXES = {".yaml", ".md"}

LoadProgressCallback = Callable[[int, int], None]


def load_documents(
    docs_root: str,
    on_progress: LoadProgressCallback | None = None,
) -> list[SourceDocument]:
    root = Path(docs_root)
    documents: list[SourceDocument] = []

    if not root.exists():
        raise FileNotFoundError(f"Docs directory does not exist: {docs_root}")

    candidate_paths = [
        path
        for path in sorted(root.rglob("*"))
        if path.is_file() and path.suffix.lower() in SUPPORTED_SUFFIXES
    ]
    total_candidates = len(candidate_paths)

    if on_progress:
        on_progress(0, total_candidates)

    for index, path in enumerate(candidate_paths, start=1):
        suffix = path.suffix.lower()

        content = path.read_text(encoding="utf-8", errors="ignore").strip()
        if not content:
            if on_progress:
                on_progress(index, total_candidates)
            continue

        documents.append(
            SourceDocument(
                path=str(path),
                title=path.stem,
                content=content,
                suffix=suffix,
            )
        )
        if on_progress:
            on_progress(index, total_candidates)

    return documents
