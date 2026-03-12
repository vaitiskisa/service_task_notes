# Task Notes Microservice + RAG Assistant

This repository contains:

- a C microservice for notes CRUD over HTTP (`/notes`)
- a Python RAG assistant that answers prompts using repository docs and calls GitHub Models for generation
- a helper script `run_service.py` to set up and run the full stack

## What You Get

- Notes API running at `http://127.0.0.1:8080`
- File-based storage in `data/notes` (one JSON file per note)
- Assistant index stored in `assistant/data/chroma_db`
- Interactive terminal mode where your input is sent to the assistant

## Recommended Workflow

### 1. Setup everything

```bash
python3 run_service.py setup
```

`setup` performs:

1. Installs apt dependencies
2. Compiles the C microservice (`make`)
3. Prepares `.env` (from `.env.example` if needed) and ensures `GITHUB_TOKEN`
4. Creates Python venv `py_service_task_notes` and installs Python dependencies
5. Installs Ollama (if missing) and pulls `all-minilm` (if missing)
6. Builds documentation index in `assistant/data/chroma_db`

For non-interactive confirmation:

```bash
python3 run_service.py setup --yes
```

### 2. Start service + assistant loop

```bash
python3 run_service.py start
```

This starts `build/service_task_notes`, then enters an input loop:

- each terminal line is sent to `assistant/main.py ask`
- the assistant response is printed in the same terminal
- stop with `Ctrl+C`

## Alternative to `run_service.py` (Manual Run Commands)

Run all commands from the project root directory.

### 1. Build and run the server

```bash
make
./build/service_task_notes
```

### 2. Run unit tests

```bash
make unit-test
```

### 3. Run coverage tests

```bash
make coverage
```

### 4. Build the chunk vector database

```bash
python3 assistant/main.py index
```

### 5. Send a prompt to the assistant

```bash
export GITHUB_TOKEN="{YOUR-GITHUB-TOKEN-GOES-HERE}"
python3 assistant/main.py ask "{Prompt}"
```

### Important notes

- Use `assistant/main.py` (no leading `/`).
- Keep the same `--persist-dir` value for both `index` and `ask/search` if you override defaults.
- The server process is blocking; run assistant commands from another terminal while the server is running.

## Manual Setup (Fallback)

If you prefer doing setup manually:

### System dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  libmicrohttpd-dev \
  libjansson-dev \
  libcmocka-dev \
  lcov \
  python3 \
  python3-venv \
  python3-pip \
  curl
```

### Build microservice

```bash
make
```

### Python environment

```bash
python3 -m venv py_service_task_notes
source py_service_task_notes/bin/activate
pip install --upgrade pip
pip install chromadb requests azure-ai-inference pyyaml
```

### Environment token

Create/update `.env`:

```env
GITHUB_TOKEN="YOUR_TOKEN"
```

Token generation link used by setup:

`https://github.com/settings/personal-access-tokens/new?description=Used+to+call+GitHub+Models+APIs+to+easily+run+LLMs%3A+https%3A%2F%2Fdocs.github.com%2Fgithub-models%2Fquickstart%23step-2-make-an-api-call&name=GitHub+Models+token&user_models=read`

### Ollama + embedding model

```bash
curl -fsSL https://ollama.com/install.sh | sh
ollama pull all-minilm
```

### Build RAG index

```bash
python3 assistant/main.py index --persist-dir assistant/data/chroma_db
```

## Assistant CLI Examples

Use the same persist dir to avoid vector DB mismatches:

```bash
python3 assistant/main.py search "create note endpoint" --persist-dir assistant/data/chroma_db
python3 assistant/main.py ask "Please provide a command to create a note" --persist-dir assistant/data/chroma_db
```

If documentation changes, re-run index:

```bash
python3 assistant/main.py index --persist-dir assistant/data/chroma_db
```

## API Quick Examples

For prettier JSON output, install/use `jq`.

Create note:

```bash
curl -X POST http://127.0.0.1:8080/notes \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Buy groceries",
    "content": "Milk, eggs, bread",
    "tags": ["personal", "shopping"]
  }'
```

List notes:

```bash
curl http://127.0.0.1:8080/notes | jq
```

Get note by ID:

```bash
curl http://127.0.0.1:8080/notes/{id} | jq
```

List notes by tag:

```bash
curl "http://127.0.0.1:8080/notes?tag=work" | jq
```

Update note:

```bash
curl -X PUT http://127.0.0.1:8080/notes/{id} \
  -H "Content-Type: application/json" \
  -d '{
    "title": "Buy groceries today",
    "content": "Milk, eggs, bread, cheese",
    "tags": ["personal", "shopping", "urgent"]
  }'
```

Delete note:

```bash
curl -X DELETE http://127.0.0.1:8080/notes/{id}
```

## Troubleshooting

- `Service binary not found`: run `make` or `python3 run_service.py setup`.
- Assistant fails at retrieval/query: ensure both `index` and `ask/search` use `--persist-dir assistant/data/chroma_db`.
- Ollama model missing: run `ollama pull all-minilm`.
- Generation fails with auth error: ensure `GITHUB_TOKEN` exists in `.env`.
- `index` is slow: this is expected on CPU-only setups; first run is the slowest.

## Note on Ollama Disk Usage

Ollama stores models in its default model location (depends on your installation mode/user).  
If disk space layout is a concern, you can relocate the model directory and use a symlink at the OS level.

## Note on failure during Ollama installation

If Ollama fails to install due to some reason, and errors of similar nature start appearing:

```bash
Error: could not connect to ollama server, run 'ollama serve' to start it
Command failed with exit code 1.
```

Please follow the uninstall procedures and then try again: <https://docs.ollama.com/linux#uninstall>

## Additional Documentation

- `docs/api.md`
- `docs/openapi.yaml`
- `docs/architecture.md`
- `docs/examples_cookbook.md`
- `docs/known_issues.md`
- `docs/rag_ground_truth.md`
