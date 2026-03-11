# RAG Ground Truth

## Purpose

Canonical facts for chatbot answers about the Task Notes service.

Use this file to resolve conflicts when multiple docs differ.

## Source Priority (highest to lowest)

1. Implementation code in `src/`
2. `docs/openapi.yaml`
3. `docs/api.md`
4. `docs/examples_cookbook.md`
5. `docs/architecture.md`

If lower-priority docs conflict with higher-priority sources, follow higher-priority sources and flag the mismatch.

## Canonical Facts

Each fact has a stable ID so answers can cite it internally.

- `GT-001` (verified 2026-03-11): Base URL is `http://127.0.0.1:8080` by default.
- `GT-002` (verified 2026-03-11): Supported resources are `POST/GET /notes` and `GET/PUT/DELETE /notes/{id}`.
- `GT-003` (verified 2026-03-11): Success payloads are JSON; errors use shape `{"error":"..."}`.
- `GT-004` (verified 2026-03-11): Collection routes include `/notes` and `/notes/` in runtime routing.
- `GT-005` (verified 2026-03-11): Tag filtering uses query param `tag` with exact tag matching.
- `GT-006` (verified 2026-03-11): Query decoding converts `%xx` URL encoding and treats `+` as space.
- `GT-007` (verified 2026-03-11): `title` and `content` must be non-empty for create/update.
- `GT-008` (verified 2026-03-11): `tags` is optional; if provided, values must be strings.
- `GT-009` (verified 2026-03-11): Empty `tags` array may fail in current parser implementation.
- `GT-010` (verified 2026-03-11): Delete success payload is `{"status":"deleted"}`.
- `GT-011` (verified 2026-03-11): Invalid paths return router-level `404` with `{"error":"Not found"}`.
- `GT-012` (verified 2026-03-11): Unsupported methods on valid paths return `405` with `{"error":"Method not allowed"}`.
- `GT-013` (verified 2026-03-11): Intended behavior for missing note IDs is `404 Note not found`.
- `GT-014` (verified 2026-03-11): Current implementation may return `500 Internal error` for some missing-ID item operations.
- `GT-015` (verified 2026-03-11): Storage is local filesystem JSON (`{id}.json`), one file per note.

## Answering Policy for Chatbot

When answering API questions:

1. Prefer direct, concrete answers using canonical facts.
2. Include endpoint, method, expected status, and minimal request example.
3. For edge cases, mention both intended and current behavior if they differ.
4. If uncertain, say what is unknown and point to the closest source section.

## Response Patterns (for consistency)

- Pattern A (how-to):
  - "Use `<METHOD> <path>` with body `<shape>`. On success you get `<status>` and `<response shape>`."
- Pattern B (error debug):
  - "`<error>` usually means `<cause>`. Check `<field/route>`, then retry with `<fix>`."
- Pattern C (contract mismatch):
  - "Intended behavior is `<expected>`, but current implementation may return `<actual>`."

## Ground Truth Update Checklist

When API behavior changes, update:

1. `docs/openapi.yaml`
2. `docs/api.md`
3. this `docs/rag_ground_truth.md`
4. `docs/known_issues.md` (if mismatch remains)
5. examples in `docs/examples_cookbook.md`
