# Task Notes Microservice Architecture

## Purpose

This document describes the high-level design of the Task Notes system, focusing on service boundaries, internal module decomposition, request flow, storage strategy, and technology decisions.

## 1. Service Decomposition

The system is intentionally split into clear responsibilities.

### System-level components

| Component | Responsibility |
|---|---|
| API Client | Calls REST endpoints to create/read/update/delete notes |
| Assistant Service (separate process/service) | Interprets user intent and generates note actions/summaries |
| Task Notes Microservice (this repo) | Deterministic CRUD and filtering over notes |
| Filesystem Storage | Persists note JSON files |

### High-level context

```text
+------------------+        HTTP/JSON        +---------------------------+
| API Client       | ----------------------> | Task Notes Microservice   |
| (UI/CLI/tests)   | <---------------------- | (CRUD + filtering)        |
+------------------+                         +------------+--------------+
                                                         |
                                                         | file I/O
                                                         v
                                              +--------------------------+
                                              | Notes JSON files         |
                                              | (one file per note)      |
                                              +--------------------------+

+------------------+        HTTP/JSON        +---------------------------+
| Assistant Service| ----------------------> | Task Notes Microservice   |
| (optional)       | <---------------------- |                           |
+------------------+                         +---------------------------+
```

## 2. Why the Assistant Is Separate from Notes Service

Keeping assistant capabilities outside the notes microservice is a deliberate boundary choice:

- Reliability isolation: assistant/runtime failures do not break core note CRUD operations.
- Deterministic core: the notes service remains predictable and testable (no model-dependent behavior in the write path).
- Security separation: note storage API can be hardened independently from assistant credentials/prompts/tooling.
- Independent scaling: assistant workloads are bursty and compute-heavy, while notes CRUD is lightweight I/O.
- Clear ownership: notes service owns persistence contract; assistant owns interpretation and orchestration.
- Easier evolution: assistant models/providers can change without forcing storage/API redesign.

## 3. Request Flow

A typical request (for example `POST /notes`) follows this path:

1. `src/http/server.c` receives HTTP traffic via `libmicrohttpd`.
2. Request body is accumulated; `HttpRequest` is built (`method`, `path`, `query`, `body`).
3. `src/http/router.c` classifies route:

- collection routes: `/notes` or `/notes/`
- item routes: `/notes/{id}`

1. Router dispatches to handler in `src/notes_handler/notes_handler.c`.
2. Handler parses/serializes JSON and calls service methods.
3. `src/service/notes_service.c` applies business rules (validation, ID generation, operation semantics).
4. `src/notes_repository/notes_repository.c` performs file-based persistence.
5. Response is returned as JSON (`src/utils/json_utils.c` helpers + HTTP response model).

### Sequence (simplified)

```text
Client
  -> HttpServer (libmicrohttpd)
  -> Router (path + method dispatch)
  -> Notes Handler (HTTP/JSON boundary)
  -> Notes Service (business logic)
  -> Notes Repository (filesystem JSON)
  -> Service -> Handler -> Router -> HttpServer
  <- Client (JSON response)
```

## 4. Main Modules

| Module | Path | Responsibility | Depends On |
|---|---|---|---|
| Entry point | `src/main.c` | Wires dependencies, starts/stops server, signal handling | all core modules |
| HTTP transport | `src/http` | HTTP server abstraction, request/response structures, routing | utils, handler callbacks |
| API handlers | `src/notes_handler` | Convert HTTP JSON <-> domain objects, map service errors to HTTP | service, notes model, utils |
| Business logic | `src/service` | Validation, note ID generation, operation orchestration | repository, notes model |
| Persistence | `src/notes_repository` | Store/load/list/delete notes in filesystem | notes model, jansson, POSIX fs |
| Domain model | `src/notes` | `Note` lifecycle, cloning, JSON conversion, validation, tags | jansson, utils/error codes |
| Shared utilities | `src/utils` | Error codes/macros and JSON/response helpers | http types, jansson |
| Tests | `src/*/unittests` | Module-focused unit tests with mocks | cmocka |

### Dependency direction

```text
HTTP -> Handler -> Service -> Repository -> Filesystem
         |           |
         +---------> Note model / utils
```

The dependency direction is one-way to reduce coupling and keep modules testable in isolation.

## 5. Storage Approach

The service uses a local file-based repository.

- Base directory: configurable at startup (`./data/notes` by default).
- File layout: one note per file named `{id}.json`.
- Write strategy: create/update writes to `*.tmp` then uses `rename(...)` for atomic replacement.
- Read strategy: load JSON file into `Note` model.
- List strategy: scan directory for `*.json` files; load each note.
- Tag filter strategy: full scan + in-memory tag match (`noteHasTag`).

### Implications

- Simple and portable, zero external DB dependency.
- Good for small/medium datasets and local development.
- No secondary indexes: tag queries are O(number of notes).
- Concurrency model is basic file I/O; no cross-process transaction manager.

## 6. Technology Choices and Reasoning

| Technology | Why it was chosen |
|---|---|
| C | Low-level control, minimal runtime dependencies, explicit memory/performance behavior |
| `libmicrohttpd` | Lightweight embedded HTTP server suited for single-binary services |
| `jansson` | Straightforward JSON parse/serialize in C for request/response and persistence |
| Filesystem JSON persistence | Fast to implement, human-readable data, easy local debugging/backups |
| `cmocka` unit tests + module mocks | Isolated testing of each layer and contract |
| Sanitizers in build (`asan`/`lsan`) | Detect memory and lifetime issues early in development |

## 7. Trade-offs and Future Evolution

Current architecture optimizes for clarity and local operability over horizontal scale.

Natural next steps if requirements grow:

- Introduce a DB-backed repository while keeping the `notes_repository` interface stable.
- Add pagination/indexing for listing and tag filtering.
- Keep assistant integration external through API calls to preserve service boundary.
- Ensure valid concurrency when using the service
