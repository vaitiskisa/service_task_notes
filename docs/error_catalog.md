# Error Catalog

## Purpose

Use this document as the canonical mapping between API error responses and actionable guidance for users/operators.

## RAG Usage Notes

- Keep each error entry self-contained.
- Prefer returning `User-facing explanation` and `Suggested fix` fields in chatbot answers.
- Update this file whenever error text or status codes change.
- Last verified against code: 2026-03-11.

## Entry Template

```md
### ERR-XXX: <short name>
- HTTP status:
- Error body:
- Emitted by:
- Typical cause:
- User-facing explanation:
- Suggested fix:
- Retry guidance:
- Endpoints:
- Notes:
```

## Catalog

### ERR-001: INVALID_NOTE_DATA

- HTTP status: `400`
- Error body: `{"error":"Invalid note data"}`
- Emitted by: notes handlers (validation mapping)
- Typical cause: missing/invalid JSON, missing required `title`/`content`, wrong field types
- User-facing explanation: The request body does not match the expected note format.
- Suggested fix: Send valid JSON with non-empty `title` and `content`; if `tags` is provided, make it an array of strings.
- Retry guidance: retry after correcting payload
- Endpoints: `POST /notes`, `PUT /notes/{id}`
- Notes: empty `tags: []` may fail in current parser implementation.

### ERR-002: NOTE_NOT_FOUND

- HTTP status: `404` (intended)
- Error body: `{"error":"Note not found"}`
- Emitted by: handler error mapping for service not-found code
- Typical cause: requested ID does not exist
- User-facing explanation: The note ID is unknown or already deleted.
- Suggested fix: verify note ID and retry.
- Retry guidance: retry only with a valid ID
- Endpoints: `GET /notes/{id}`, `PUT /notes/{id}`, `DELETE /notes/{id}`
- Notes: current implementation may return `500` instead for missing IDs in some flows.

### ERR-003: NOTE_ALREADY_EXISTS

- HTTP status: `409`
- Error body: `{"error":"Note already exists"}`
- Emitted by: create flow conflict mapping
- Typical cause: generated ID collision or duplicate-create edge case
- User-facing explanation: A note with this identity already exists.
- Suggested fix: retry create request.
- Retry guidance: safe to retry
- Endpoints: `POST /notes`
- Notes: rare due to UUID-like ID generation.

### ERR-004: METHOD_NOT_ALLOWED

- HTTP status: `405`
- Error body: `{"error":"Method not allowed"}`
- Emitted by: HTTP router
- Typical cause: unsupported HTTP method for a valid path
- User-facing explanation: This endpoint does not support the requested method.
- Suggested fix: use supported methods from OpenAPI (`GET/POST/PUT/DELETE` as applicable).
- Retry guidance: retry with correct method
- Endpoints: valid notes routes with unsupported methods
- Notes: e.g., `PUT /notes`.

### ERR-005: ROUTE_NOT_FOUND

- HTTP status: `404`
- Error body: `{"error":"Not found"}`
- Emitted by: HTTP router
- Typical cause: unknown path or malformed route
- User-facing explanation: The requested endpoint path does not exist.
- Suggested fix: use `/notes` or `/notes/{id}`.
- Retry guidance: retry with correct path
- Endpoints: all unknown routes
- Notes: examples include `/note` and `/notes/{id}/extra`.

### ERR-006: INTERNAL_ERROR

- HTTP status: `500`
- Error body: `{"error":"Internal error"}`
- Emitted by: fallback mapping in handlers/router
- Typical cause: unexpected service/repository/internal failure
- User-facing explanation: The service failed to process the request.
- Suggested fix: retry once, then escalate with request details if it continues.
- Retry guidance: one retry recommended; otherwise investigate
- Endpoints: any
- Notes: currently also used in some missing-note paths.

### ERR-007: STORAGE_ERROR

- HTTP status: `500`
- Error body: `{"error":"Storage error"}`
- Emitted by: handler mapping for storage failures
- Typical cause: file I/O or JSON persistence failure
- User-facing explanation: The note storage layer could not complete the operation.
- Suggested fix: check data directory permissions and disk state.
- Retry guidance: retry may work for transient I/O issues
- Endpoints: any write/read path touching repository
- Notes: mostly infrastructure/operator concern.
