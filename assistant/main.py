# main.py
import argparse
from pathlib import Path

from rag.generate.assistant_service import RagAssistantService
from rag.retriever.indexer import build_rag_index
from rag.retriever.retrieval import RagRetriever

PROJECT_ROOT = Path(__file__).resolve().parents[1]  # .../service_task_notes
DOCS_ROOT = PROJECT_ROOT / "docs"

def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)

    index_parser = sub.add_parser("index")
    index_parser.add_argument("--docs-root", default=str(DOCS_ROOT))
    index_parser.add_argument("--persist-dir", default="data/chroma_db")
    index_parser.add_argument("--collection", default="project_docs")
    index_parser.add_argument("--embedding-model", default="all-minilm")
    index_parser.add_argument("--no-reset", action="store_true")

    search_parser = sub.add_parser("search")
    search_parser.add_argument("query", type=str)
    search_parser.add_argument("--top-k", type=int, default=5)
    search_parser.add_argument("--persist-dir", default="data/chroma_db")
    search_parser.add_argument("--collection", default="project_docs")
    search_parser.add_argument("--embedding-model", default="all-minilm")

    ask_parser = sub.add_parser("ask")
    ask_parser.add_argument("query", type=str)
    ask_parser.add_argument("--top-k", type=int, default=5)
    ask_parser.add_argument("--persist-dir", default="data/chroma_db")
    ask_parser.add_argument("--collection", default="project_docs")
    ask_parser.add_argument("--embedding-model", default="all-minilm")
    ask_parser.add_argument(
        "--generation-model",
        default="meta/Llama-4-Scout-17B-16E-Instruct",
    )
    ask_parser.add_argument("--generation-timeout", type=int, default=300)
    ask_parser.add_argument("--max-context-chars", type=int, default=6000)
    ask_parser.add_argument("--max-chunk-chars", type=int, default=1200)
    ask_parser.add_argument("--stream", action="store_true")

    args = parser.parse_args()

    if args.cmd == "index":
        build_rag_index(
            docs_root=args.docs_root,
            persist_dir=args.persist_dir,
            collection_name=args.collection,
            embedding_model=args.embedding_model,
            reset=not args.no_reset,
        )
        return

    if args.cmd == "search":
        retriever = RagRetriever(
            persist_dir=args.persist_dir,
            collection_name=args.collection,
            embedding_model=args.embedding_model,
        )
        results = retriever.retrieve(args.query, top_k=args.top_k)

        for idx, item in enumerate(results, start=1):
            metadata = item.get("metadata", {})
            print(f"\n[{idx}]")
            print(f"chunk_id: {item['chunk_id']}")
            print(f"distance: {item['distance']}")
            print(f"path: {metadata.get('source_path', '-')}")
            print(f"title: {metadata.get('title', '-')}")
            print(item["text"][:600])
        return

    if args.cmd == "ask":
        service = RagAssistantService(
            persist_dir=args.persist_dir,
            collection_name=args.collection,
            embedding_model=args.embedding_model,
            generation_model=args.generation_model,
            generation_timeout=args.generation_timeout,
        )

        on_token = None
        if args.stream:
            print("\n=== ANSWER ===\n")

            def on_token(token: str) -> None:
                print(token, end="", flush=True)

        print("Query:", args.query)

        result = service.answer_question(
            query=args.query,
            top_k=args.top_k,
            max_context_chars=args.max_context_chars,
            max_chunk_chars=args.max_chunk_chars,
            stream=args.stream,
            on_token=on_token,
        )

        print("\n=== ANSWER ===\n")
        print(result.answer)

        print("\n=== SOURCES ===\n")
        for source in result.sources:
            print(
                f"[SOURCE {source['source_id']}] "
                f"{source['source_path']} | "
                f"{source['title']} | "
                f"{source['start_offset']}-{source['end_offset']}"
            )


if __name__ == "__main__":
    main()
