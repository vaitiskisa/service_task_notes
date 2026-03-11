from dataclasses import dataclass, asdict
from typing import Any


@dataclass
class SourceDocument:
    path: str
    title: str
    content: str
    suffix: str


@dataclass
class Chunk:
    chunk_id: str
    source_path: str
    title: str
    text: str
    start_offset: int
    end_offset: int
    token_estimate: int
    metadata: dict[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)