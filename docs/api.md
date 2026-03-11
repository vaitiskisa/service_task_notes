# Task Notes Microservice API

## Overview

The Task Notes API is a JSON over HTTP service for CRUD operations on notes.

- Default host: `127.0.0.1`
- Default port: `8080`
- Base URL: `http://127.0.0.1:8080`
- Content type: `application/json`

## Quick Start (Minimal)

Use these calls in order:

1. Create:
   `curl -X POST http://127.0.0.1:8080/notes -H "Content-Type: application/json" -d '{"title":"Buy groceries","content":"Milk, eggs, bread","tags":["personal","shopping"]}'`
   Success: `201` + created note object.
2. List:
   `curl http://127.0.0.1:8080/notes`
   Success: `200` + array of notes.
3. Get:
   `curl http://127.0.0.1:8080/notes/{id}`
   Success: `200` + note object.
4. Update:
   `curl -X PUT http://127.0.0.1:8080/notes/{id} -H "Content-Type: application/json" -d '{"title":"Buy groceries today","content":"Milk, eggs, bread, cheese","tags":["personal","shopping","urgent"]}'`
   Success: `200` + updated note object.
5. Delete:
   `curl -X DELETE http://127.0.0.1:8080/notes/{id}`
   Success: `200` + `{"status":"deleted"}`.

## Error Cheat Sheet

- `POST /notes`:
  - `400`: invalid note JSON/data (missing/empty `title` or `content`, invalid `tags`)
  - `409`: note already exists
  - `500`: storage/internal error
- `404 Not Found`: invalid route or missing note ID.
- `405 Method Not Allowed`: route exists but method is unsupported.
- Caveat: missing IDs on `GET/PUT/DELETE /notes/{id}` are intended to be `404`, but current implementation may return `500`.

## Data Model

### Note Object (response)

| Field | Type | Description |
|---|---|---|
| `id` | string | Server-generated note ID (UUID-like string) |
| `title` | string | Note title |
| `content` | string | Note content/body |
| `tags` | array[string] | List of tags |

Example:

```json
{
  "id": "3cbb11f3-665b-4a99-a857-3ae55d1f0baf",
  "title": "Buy groceries",
  "content": "Milk, eggs, bread",
  "tags": ["personal", "shopping"]
}
```

### Create/Update Payload

| Field | Required | Type | Notes |
|---|---|---|---|
| `title` | yes | string | Must be non-empty |
| `content` | yes | string | Must be non-empty |
| `tags` | no | array[string] | Optional; if provided, values must be strings |
| `id` | no | string | Ignored on create; update ID comes from path |

## Error Format

All error responses use:

```json
{
  "error": "Error message"
}
```

## Endpoints

## `POST /notes`

Create a new note.

Request body:

```json
{
  "title": "Buy groceries",
  "content": "Milk, eggs, bread",
  "tags": ["personal", "shopping"]
}
```

Responses:

- `201 Created`: created note object
- `400 Bad Request`: invalid/missing JSON or invalid note fields
- `409 Conflict`: note already exists
- `500 Internal Server Error`: storage/internal error

## `GET /notes`

List all notes.

Optional query parameter:

- `tag`: filter by exact tag value

Examples:

- `GET /notes`
- `GET /notes?tag=work`

Responses:

- `200 OK`: array of note objects
- `500 Internal Server Error`

Response example:

```json
[
  {
    "id": "id-1",
    "title": "T1",
    "content": "C1",
    "tags": ["home"]
  },
  {
    "id": "id-2",
    "title": "T2",
    "content": "C2",
    "tags": ["work"]
  }
]
```

## `GET /notes/{id}`

Get one note by ID.

Responses:

- `200 OK`: note object
- `404 Not Found`: note not found
- `500 Internal Server Error`

## `PUT /notes/{id}`

Replace note fields for an existing ID.

Request body:

```json
{
  "title": "Buy groceries today",
  "content": "Milk, eggs, bread, cheese",
  "tags": ["personal", "shopping", "urgent"]
}
```

Responses:

- `200 OK`: updated note object
- `400 Bad Request`: invalid/missing JSON or invalid note fields
- `404 Not Found`: note not found
- `500 Internal Server Error`

## `DELETE /notes/{id}`

Delete a note by ID.

Responses:

- `200 OK`:

```json
{
  "status": "deleted"
}
```

- `404 Not Found`: note not found
- `500 Internal Server Error`

## Routing and Method Rules

- Collection route: `/notes` and `/notes/`
- Item route: `/notes/{id}`
- Invalid paths (for example `/note`, `/notes/{id}/extra`) return `404 Not Found`
- Unsupported methods on valid routes return `405 Method Not Allowed`
- `404` example: `GET /note`
- `405` example: `PUT /notes`

## cURL Examples

Create:

```bash
curl -X POST http://127.0.0.1:8080/notes \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Buy groceries",
    "content": "Milk, eggs, bread",
    "tags": ["personal", "shopping"]
  }'
```

List all:

```bash
curl http://127.0.0.1:8080/notes
```

List by tag:

```bash
curl "http://127.0.0.1:8080/notes?tag=work"
```

Get by ID:

```bash
curl http://127.0.0.1:8080/notes/{id}
```

Update:

```bash
curl -X PUT http://127.0.0.1:8080/notes/{id} \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Updated title",
    "content": "Updated content",
    "tags": ["work"]
  }'
```

Delete:

```bash
curl -X DELETE http://127.0.0.1:8080/notes/{id}
```

## Current Behavior Notes

- `tag` query values are URL-decoded (for example `foo%20bar` becomes `foo bar`).
- Query values using `+` are treated as spaces during tag decoding.
- JSON request parsing does not strictly require the `Content-Type` header; valid JSON body is what matters.
- An empty `tags` array may currently be rejected by validation in the model parser. Omit `tags` instead if you do not need them.
- `GET/PUT/DELETE /notes/{id}` are intended to return `404` for missing notes, but current service-layer error mapping may return `500` in that case.
