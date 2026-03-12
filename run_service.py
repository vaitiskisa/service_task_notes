#!/usr/bin/env python3
"""Service helper script for setup tasks."""

from __future__ import annotations

import argparse
import getpass
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time


PROJECT_ROOT = Path(__file__).resolve().parent
SERVICE_BINARY = PROJECT_ROOT / "build" / "service_task_notes"
ENV_FILE = PROJECT_ROOT / ".env"
ENV_EXAMPLE_FILE = PROJECT_ROOT / ".env.example"
VENV_NAME = "py_service_task_notes"
VENV_DIR = PROJECT_ROOT / VENV_NAME
VENV_PYTHON = VENV_DIR / "bin" / "python3"
ASSISTANT_PERSIST_DIR = PROJECT_ROOT / "assistant" / "data" / "chroma_db"
OLLAMA_EMBEDDING_MODEL = "all-minilm"
OLLAMA_INSTALL_COMMAND = "curl -fsSL https://ollama.com/install.sh | sh"
GITHUB_TOKEN_KEY = "GITHUB_TOKEN"
GITHUB_TOKEN_PLACEHOLDER = "YOUR-GITHUB-TOKEN-GOES-HERE"
GITHUB_TOKEN_URL = (
    "https://github.com/settings/personal-access-tokens/new?"
    "description=Used+to+call+GitHub+Models+APIs+to+easily+run+LLMs%3A+"
    "https%3A%2F%2Fdocs.github.com%2Fgithub-models%2Fquickstart%23step-2-make-an-api-call&"
    "name=GitHub+Models+token&user_models=read"
)

APT_PACKAGES = [
    "build-essential",
    "libmicrohttpd-dev",
    "libjansson-dev",
    "libcmocka-dev",
    "lcov",
    "python3",
    "python3-venv",
    "python3-pip",
    "curl",
]

PIP_PACKAGES = [
    "chromadb",
    "requests",
    "azure-ai-inference",
    "pyyaml",
]

ENV_ASSIGNMENT_RE = re.compile(r"^\s*(?:export\s+)?([A-Za-z_][A-Za-z0-9_]*)\s*=")


def shell_join(parts: list[str]) -> str:
    return " ".join(subprocess.list2cmdline([part]) for part in parts)


def run_command(
    command: list[str],
    *,
    description: str | None = None,
    env: dict[str, str] | None = None,
) -> bool:
    if description:
        print(description)
    print(f"$ {shell_join(command)}")

    try:
        subprocess.run(command, cwd=PROJECT_ROOT, check=True, env=env)
        return True
    except FileNotFoundError:
        print(f"Command not found: {command[0]}")
    except subprocess.CalledProcessError as exc:
        print(f"Command failed with exit code {exc.returncode}.")
    return False


def prompt_yes_no(message: str, *, default: bool = False) -> bool:
    suffix = "[Y/n]" if default else "[y/N]"
    answer = input(f"{message} {suffix}: ").strip().lower()
    if not answer:
        return default
    return answer in {"y", "yes"}


def print_setup_plan() -> None:
    print("All of the following steps can be performed manually if preferred, so please review them carefully.")
    print("The setup attempts to isolate as many dependencies of this project as possible, though some installation (like Ollama or apt packages) can be system-wide.")
    print("Setup will run these steps:")
    print(f"1. Install required apt packages for the microservice:\n\t{', '.join(APT_PACKAGES)}")
    print("2. Compile the microservice with make.")
    print("3. Prepare .env from .env.example and ensure GITHUB_TOKEN entry exists.")
    print(f"4. Create a Python virtual environment if needed. Python dependencies will be installed into it.\n\t{', '.join(PIP_PACKAGES)}")
    print("5. Install Ollama and pull embedding model.")
    print("6. Run the docs indexing command to embed documentation.")


def apt_prefix() -> list[str] | None:
    if not shutil.which("apt"):
        print("apt was not found on this system. Step 1 cannot be completed automatically.")
        return None

    if hasattr(os, "geteuid") and os.geteuid() == 0:
        return []

    if shutil.which("sudo"):
        return ["sudo"]

    print("sudo was not found and this user is not root. Step 1 cannot be completed automatically.")
    return None


def install_apt_packages() -> bool:
    print("\n[1/6] Installing apt packages")
    prefix = apt_prefix()
    if prefix is None:
        return False

    update_cmd = prefix + ["apt", "update"]
    install_cmd = prefix + ["apt", "install", "-y"] + APT_PACKAGES

    return run_command(update_cmd) and run_command(install_cmd)


def compile_microservice() -> bool:
    print("\n[2/6] Compiling microservice")
    return run_command(["make"])


def parse_env_key(line: str) -> str | None:
    match = ENV_ASSIGNMENT_RE.match(line)
    return match.group(1) if match else None


def parse_env_assignment(line: str) -> tuple[str, str] | None:
    stripped = line.strip()
    if not stripped or stripped.startswith("#"):
        return None

    key = parse_env_key(stripped)
    if not key or "=" not in stripped:
        return None

    value = stripped.split("=", 1)[1].strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
        value = value[1:-1]

    return key, value


def load_env_file(path: Path) -> dict[str, str]:
    loaded: dict[str, str] = {}
    if not path.exists():
        return loaded

    for line in path.read_text(encoding="utf-8").splitlines():
        assignment = parse_env_assignment(line)
        if assignment is None:
            continue
        key, value = assignment
        loaded[key] = value

    return loaded


def quote_env_value(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f"\"{escaped}\""


def ensure_env_file() -> None:
    if not ENV_FILE.exists() and ENV_EXAMPLE_FILE.exists():
        shutil.copy2(ENV_EXAMPLE_FILE, ENV_FILE)
        print("Copied .env.example to .env.")
    elif not ENV_FILE.exists():
        ENV_FILE.write_text("", encoding="utf-8")
        print("Created empty .env because .env.example does not exist.")
    else:
        print(".env already exists. Keeping current file.")


def upsert_env_key(key: str, value: str) -> None:
    lines = ENV_FILE.read_text(encoding="utf-8").splitlines() if ENV_FILE.exists() else []
    new_line = f"{key}={quote_env_value(value)}"

    updated = False
    for index, line in enumerate(lines):
        if parse_env_key(line) == key:
            lines[index] = new_line
            updated = True
            break

    if not updated:
        if lines and lines[-1].strip():
            lines.append("")
        lines.append(new_line)

    ENV_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")


def env_has_key(key: str) -> bool:
    if not ENV_FILE.exists():
        return False

    for line in ENV_FILE.read_text(encoding="utf-8").splitlines():
        if parse_env_key(line) == key:
            return True
    return False


def setup_env_and_token() -> None:
    print("\n[3/6] Preparing .env and GitHub token")
    ensure_env_file()
    print("Generate a GitHub token here:")
    print(GITHUB_TOKEN_URL)

    if not env_has_key(GITHUB_TOKEN_KEY):
        upsert_env_key(GITHUB_TOKEN_KEY, GITHUB_TOKEN_PLACEHOLDER)
        print("Added missing GITHUB_TOKEN entry to .env.")
    else:
        print("GITHUB_TOKEN entry already exists in .env.")

    token_value = None
    if ENV_FILE.exists():
        for line in ENV_FILE.read_text(encoding="utf-8").splitlines():
            if parse_env_key(line) == GITHUB_TOKEN_KEY:
                token_value = line.split("=", 1)[1].strip().strip('"')
                break

    if token_value and token_value != GITHUB_TOKEN_PLACEHOLDER:
        print("GITHUB_TOKEN already has a value in .env. Skipping token input.")
        return

    if not prompt_yes_no("Do you want to paste the token now?"):
        print("Skipping token input. Update .env later when ready.")
        return

    try:
        token = getpass.getpass("Paste GITHUB_TOKEN (input is hidden): ").strip()
    except EOFError:
        print("No token received. Keeping current .env value.")
        return

    if not token:
        print("Empty token provided. Keeping current .env value.")
        return

    upsert_env_key(GITHUB_TOKEN_KEY, token)
    print("Stored GITHUB_TOKEN in .env.")


def install_python_dependencies(python_bin: str) -> bool:
    upgrade_ok = run_command([python_bin, "-m", "pip", "install", "--upgrade", "pip"])
    deps_ok = run_command([python_bin, "-m", "pip", "install", *PIP_PACKAGES])
    return upgrade_ok and deps_ok


def resolve_ollama_binary() -> str | None:
    ollama_bin = shutil.which("ollama")
    if ollama_bin:
        return ollama_bin

    fallback = Path("/usr/local/bin/ollama")
    if fallback.exists():
        return str(fallback)

    return None


def is_ollama_model_available(ollama_bin: str, model_name: str) -> bool:
    try:
        result = subprocess.run(
            [ollama_bin, "list"],
            cwd=PROJECT_ROOT,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError:
        return False

    if result.returncode != 0:
        return False

    model, _, _ = model_name.partition(":")
    for raw_line in result.stdout.splitlines():
        line = raw_line.strip()
        if not line or line.lower().startswith("name"):
            continue
        listed_name = line.split(None, 1)[0]
        if listed_name == model_name or listed_name == model or listed_name.startswith(f"{model}:"):
            return True
    return False


def ensure_virtual_environment() -> str | None:
    print("\n[4/6] Setting up Python virtual environment")

    if VENV_DIR.exists():
        print(f"{VENV_NAME} already exists. Skipping creation.")
    else:
        if not run_command(["python3", "-m", "venv", VENV_NAME]):
            print("Failed to create virtual environment automatically.")
            print(
                "Please create it manually with python venv, miniconda, or your preferred method."
            )
            return None

    if not VENV_PYTHON.exists():
        print(f"Expected interpreter is missing at {VENV_PYTHON}.")
        return None

    if not install_python_dependencies(str(VENV_PYTHON)):
        print("Could not install Python packages automatically. Step 6 may fail.")
    return str(VENV_PYTHON)


def install_ollama_and_embedding_model() -> bool:
    print("\n[5/6] Installing Ollama and pulling embedding model")

    ollama_bin = resolve_ollama_binary()
    if ollama_bin:
        print(f"Ollama is already installed at {ollama_bin}. Skipping installer.")
    else:
        install_ok = run_command(["bash", "-lc", OLLAMA_INSTALL_COMMAND])
        if not install_ok and shutil.which("sudo"):
            print("Retrying Ollama installation with sudo.")
            install_ok = run_command(["sudo", "bash", "-lc", OLLAMA_INSTALL_COMMAND])

        if not install_ok:
            print("Failed to install Ollama.")
            return False

        ollama_bin = resolve_ollama_binary()

    if not ollama_bin:
        print("Ollama binary not found after installation.")
        return False

    if is_ollama_model_available(ollama_bin, OLLAMA_EMBEDDING_MODEL):
        print(
            f"Model '{OLLAMA_EMBEDDING_MODEL}' already exists in Ollama. Skipping download."
        )
        return True

    if not run_command([ollama_bin, "pull", OLLAMA_EMBEDDING_MODEL]):
        print(
            f"Failed to pull model '{OLLAMA_EMBEDDING_MODEL}'. "
            "Ensure Ollama service is running, then retry setup."
        )
        return False

    print(f"Model '{OLLAMA_EMBEDDING_MODEL}' is available in Ollama.")
    return True


def run_index_step(python_bin: str | None) -> bool:
    print("\n[6/6] Running documentation index [might take a while, depending on hardware specs]")

    interpreter = python_bin or shutil.which("python3")
    if not interpreter:
        print("python3 is not available, so indexing cannot be run.")
        return False

    ok = run_command(
        [
            interpreter,
            "assistant/main.py",
            "index",
            "--persist-dir",
            str(ASSISTANT_PERSIST_DIR),
            "--embedding-model",
            OLLAMA_EMBEDDING_MODEL,
        ],
    )
    if not ok:
        print(
            "Indexing failed. Ensure Python dependencies are installed and the embedding backend is available."
        )
        return False

    print("Indexing completed.")
    print("If documentation changes later, run the index command again.")
    return True


def run_setup(auto_confirm: bool) -> int:
    print_setup_plan()
    if not auto_confirm and not prompt_yes_no("Proceed with setup now?"):
        print("Setup cancelled.")
        return 0

    if not install_apt_packages():
        return 1

    if not compile_microservice():
        return 1

    setup_env_and_token()
    python_bin = ensure_virtual_environment()
    if not python_bin:
        return 1

    if not install_ollama_and_embedding_model():
        return 1

    if not run_index_step(python_bin):
        return 1

    print("\nSetup completed.")
    return 0


def activate_virtual_environment(env: dict[str, str]) -> tuple[dict[str, str], str | None]:
    if not VENV_PYTHON.exists():
        return env, None

    activated_env = dict(env)
    venv_bin = str(VENV_DIR / "bin")
    activated_env["VIRTUAL_ENV"] = str(VENV_DIR)

    current_path = activated_env.get("PATH", "")
    activated_env["PATH"] = f"{venv_bin}:{current_path}" if current_path else venv_bin

    python_bin = shutil.which("python3", path=activated_env["PATH"])
    return activated_env, python_bin or str(VENV_PYTHON)

def run_assistant_prompt(python_bin: str, prompt: str, env: dict[str, str]) -> bool:
    cmd = [
        python_bin,
        "assistant/main.py",
        "ask",
        "--persist-dir",
        str(ASSISTANT_PERSIST_DIR),
        "--embedding-model",
        OLLAMA_EMBEDDING_MODEL,
        prompt,
    ]

    try:
        result = subprocess.run(
            cmd,
            cwd=PROJECT_ROOT,
            env=env,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        print(f"Failed to run assistant command: {exc}")
        return False

    stdout = result.stdout.strip()
    stderr = result.stderr.strip()

    if stdout:
        print(stdout)
    if stderr:
        print(stderr)

    if result.returncode != 0:
        print("Assistant request failed.")
        return False
    return True


def stop_process(proc: subprocess.Popen) -> None:
    if proc.poll() is not None:
        return

    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


def run_start() -> int:
    if not SERVICE_BINARY.exists():
        print(f"Service binary not found: {SERVICE_BINARY}")
        print("Build it first (e.g. `python3 run_service.py setup` or `make`).")
        return 1

    if not os.access(SERVICE_BINARY, os.X_OK):
        print(f"Service binary is not executable: {SERVICE_BINARY}")
        return 1

    runtime_env = os.environ.copy()
    env_from_file = load_env_file(ENV_FILE)
    if env_from_file:
        runtime_env.update(env_from_file)
        print(f"Loaded {len(env_from_file)} variable(s) from {ENV_FILE}.")
    else:
        print(f"No variables loaded from {ENV_FILE} (file missing or empty).")

    runtime_env, python_bin = activate_virtual_environment(runtime_env)
    if not python_bin:
        print(f"Virtual environment not found: {VENV_PYTHON}")
        print("Run `python3 run_service.py setup` first.")
        return 1

    print(f"Activated virtual environment: {VENV_DIR}")

    print("Starting microservice...")
    try:
        service_proc = subprocess.Popen([str(SERVICE_BINARY)], cwd=PROJECT_ROOT, env=runtime_env)
    except OSError as exc:
        print(f"Failed to start microservice: {exc}")
        return 1

    time.sleep(0.3)
    if service_proc.poll() is not None:
        print(f"Microservice exited immediately with code {service_proc.returncode}.")
        return 1

    print("Microservice is running.")
    print("Type any prompt to send it to the assistant. Press Ctrl+C to exit.")
    print(
        f"Assistant command uses: {python_bin} assistant/main.py ask "
        f"--persist-dir {ASSISTANT_PERSIST_DIR} --embedding-model {OLLAMA_EMBEDDING_MODEL}"
    )

    try:
        while True:
            if service_proc.poll() is not None:
                print(f"Microservice stopped with code {service_proc.returncode}. Exiting.")
                return 1

            try:
                user_input = input("\nassistant> ")
            except EOFError:
                print("\nInput stream closed. Stopping.")
                return 0

            run_assistant_prompt(python_bin, user_input, runtime_env)
    except KeyboardInterrupt:
        print("\nStopping...")
        return 0
    finally:
        stop_process(service_proc)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Helper script to setup and run the Task Notes service."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    setup_parser = subparsers.add_parser("setup", help="Setup local environment and index docs")
    setup_parser.add_argument(
        "--yes",
        action="store_true",
        help="Run setup without waiting for confirmation prompt.",
    )

    subparsers.add_parser("start", help="Start microservice from build/service_task_notes")

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.command == "setup":
        return run_setup(auto_confirm=args.yes)

    if args.command == "start":
        return run_start()

    parser.print_help()
    return 1


if __name__ == "__main__":
    sys.exit(main())
