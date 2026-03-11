# API Examples Cookbook

## Purpose

Task-oriented API examples for developer and RAG retrieval.

## RAG Usage Notes

- Prefer retrieving by `Intent`.
- Keep each recipe independent and complete.
- Include both happy path and failure path examples.
- Last verified against code: 2026-03-11.

## Recipe Template

```md
## Recipe: <name>
Intent:
When to use:
Request:
Expected response:
Common mistakes:
```

## Recipe: Create a Note

Intent: create a new note with title/content and optional tags.

When to use: first-time note creation.

Status: `201 Created`

Expected response:

```json
{
  "id": "<generated-id>",
  "title": "Buy groceries",
  "content": "Milk, eggs, bread",
  "tags": ["personal", "shopping"]
}
```

Request:

```bash
curl -X POST http://127.0.0.1:8080/notes -H "Content-Type: application/json" -d '{"title":"Buy groceries","content":"Milk, eggs, bread","tags":["personal","shopping"]}'
```

Common mistakes:

- missing `title` or `content`
- invalid JSON body
- sending `tags: []` (currently may be rejected)

## Recipe: List All Notes

Intent: fetch the whole notes collection.

Request:

```bash
curl http://127.0.0.1:8080/notes
```

Expected response: JSON array of notes.
Status: `200 OK`

## Recipe: List Notes by Tag

Intent: filter notes by an exact tag.

Request:

```bash
curl "http://127.0.0.1:8080/notes?tag=work"
```

Expected response: JSON array of notes containing tag `work`.
Status: `200 OK`

Common mistakes:

- assuming partial match (filter is exact tag match)
- forgetting URL encoding for spaces in tags

## Recipe: Get Note by ID

Intent: read one note.

Request:

```bash
curl http://127.0.0.1:8080/notes/<id>
```

Expected response: note object.
Status: `200 OK`

If missing ID:

- intended: `404 Not Found`
- current implementation can return `500 Internal Server Error` in some flows

## Recipe: Update Note by ID

Intent: replace note fields for an existing note.

Request:

```bash
curl -X PUT http://127.0.0.1:8080/notes/<id> \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Buy groceries today",
    "content": "Milk, eggs, bread, cheese",
    "tags": ["personal", "shopping", "urgent"]
  }'
```

Expected response: updated note object.
Status: `200 OK`

Common mistakes:

- invalid payload format
- expecting partial patch behavior (this is a full replace-style update payload)

## Recipe: Delete Note by ID

Intent: remove a note.

Request:

```bash
curl -X DELETE http://127.0.0.1:8080/notes/<id>
```

Expected response:

```json
{
  "status": "deleted"
}
```

Status: `200 OK`

## Recipe: Method Not Allowed Example

Intent: understand routing/method constraints.

Request:

```bash
curl -X PUT http://127.0.0.1:8080/notes
```

Expected response:

```json
{
  "error": "Method not allowed"
}
```

Status: `405 Method Not Allowed`

## Recipe: Unknown Route Example

Intent: debug wrong endpoint path.

Request:

```bash
curl http://127.0.0.1:8080/note
```

Expected response:

```json
{
  "error": "Not found"
}
```

Status: `404 Not Found`
