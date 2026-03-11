# Known Issues and Limitations

## Purpose

Track behavior gaps between intended API contract and current implementation.

## RAG Usage Notes

- This file should be retrieved before answering edge-case questions.
- When conflicts exist, chatbot should mention both intended behavior and current behavior.
- Last verified against code: 2026-03-11.

## Issue Template

```md
## ISSUE-XXX: <title>
Status: open | fixed | wontfix
Severity: low | medium | high
Area: API | storage | routing | docs
Observed behavior:
Intended behavior:
Impact:
Workaround:
Source references:
```

## ISSUE-001: Missing note may return 500 instead of 404

Status: open
Severity: medium
Area: API

Observed behavior:

- `GET/PUT/DELETE /notes/{id}` can return `500 Internal error` when note ID is missing in storage.

Intended behavior:

- Return `404 Note not found` for missing IDs.

Impact:

- API consumers cannot always distinguish true server faults from missing-resource errors.

Workaround:

- Treat `500` on missing-ID scenarios as potential not-found; verify by listing known IDs.

Source references:

- `src/service/notes_service.c` (repository not-found often collapses to common error)
- `src/notes_handler/notes_handler.c` (`mapServiceError` can only map explicit service not-found code)
- `docs/openapi.yaml` (describes caveat)

## ISSUE-002: Empty tags array may fail validation

Status: open
Severity: low
Area: API/model parsing

Observed behavior:

- Payloads with `"tags": []` may fail during model parsing.

Intended behavior:

- Empty tags should usually be acceptable as a valid empty list.

Impact:

- Clients need to omit `tags` instead of sending empty arrays.

Workaround:

- Omit `tags` when no tags are needed.

Source references:

- `src/notes/note.c` (`noteSetTags` rejects zero `count`)
- `docs/api.md` and `docs/openapi.yaml` notes

## Limitations (Not necessarily bugs)

- No pagination for `GET /notes`.
- Tag filtering is exact match and scans all notes.
- File-based storage has no secondary index for faster queries.
