from typing import Callable, Iterable
import requests

class OllamaEmbedder:
    def __init__(
        self,
        model: str = "all-minilm",
        base_url: str = "http://localhost:11434",
        timeout: int = 60,
    ) -> None:
        self.model = model
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self._api_mode: str | None = None

    def embed_texts(self, texts: list[str]) -> list[list[float]]:
        if not texts:
            return []

        # Ollama uses /api/embed on newer versions and /api/embeddings on older ones.
        modes = [self._api_mode] if self._api_mode else ["embed", "embeddings"]
        last_not_found: requests.HTTPError | None = None

        for mode in modes:
            if mode is None:
                continue

            try:
                if mode == "embed":
                    embeddings = self._embed_with_api_embed(texts)
                else:
                    embeddings = self._embed_with_api_embeddings(texts)

                self._api_mode = mode

                return embeddings
            except requests.HTTPError as exc:
                status_code = (
                    exc.response.status_code if exc.response is not None else None
                )
                error_message = self._extract_error_message(exc.response)

                # Ollama returns 404 for unknown models, not only missing endpoints.
                if (
                    status_code == 404
                    and isinstance(error_message, str)
                    and "model" in error_message.lower()
                    and "not found" in error_message.lower()
                ):
                    raise RuntimeError(
                        f"Ollama model '{self.model}' was not found at {self.base_url}. "
                        f"Run `ollama pull {self.model}` or use a model listed by `ollama list`."
                    ) from exc

                # If we are auto-detecting and this endpoint does not exist, try the other.
                if self._api_mode is None and status_code == 404:
                    last_not_found = exc
                    continue
                raise

        raise RuntimeError(
            f"Could not find a compatible Ollama embedding endpoint at {self.base_url}. "
            "Tried /api/embed and /api/embeddings."
        ) from last_not_found

    def _extract_error_message(self, response: requests.Response | None) -> str | None:
        if response is None:
            return None

        try:
            payload = response.json()
        except ValueError:
            text = response.text.strip()
            return text or None

        error = payload.get("error")
        if isinstance(error, str) and error.strip():
            return error.strip()

        return None

    def _embed_with_api_embed(self, texts: list[str]) -> list[list[float]]:
        response = requests.post(
            f"{self.base_url}/api/embed",
            json={
                "model": self.model,
                "input": texts,
            },
            timeout=self.timeout,
        )

        response.raise_for_status()
        data = response.json()
        embeddings = data.get("embeddings")

        if (
            not isinstance(embeddings, list)
            or not all(isinstance(item, list) for item in embeddings)
        ):
            raise RuntimeError("Invalid /api/embed response from Ollama")

        return embeddings

    def _embed_with_api_embeddings(self, texts: list[str]) -> list[list[float]]:
        embeddings: list[list[float]] = []

        for text in texts:
            response = requests.post(
                f"{self.base_url}/api/embeddings",
                json={
                    "model": self.model,
                    "prompt": text,
                },
                timeout=self.timeout,
            )
            response.raise_for_status()
            data = response.json()
            embedding = data.get("embedding")

            if not isinstance(embedding, list):
                raise RuntimeError("Invalid /api/embeddings response from Ollama")

            embeddings.append(embedding)

        return embeddings

    def embed_query(self, text: str) -> list[float]:
        result = self.embed_texts([text])
        if not result:
            raise RuntimeError("No embedding returned for query")
        return result[0]

    def embed_in_batches(
        self,
        texts: Iterable[str],
        batch_size: int = 16,
        on_progress: Callable[[int, int], None] | None = None,
    ) -> list[list[float]]:
        texts = list(texts)
        total_texts = len(texts)
        all_embeddings: list[list[float]] = []
        embedded_count = 0

        if on_progress:
            on_progress(embedded_count, total_texts)

        for i in range(0, total_texts, batch_size):
            batch = texts[i:i + batch_size]
            batch_embeddings = self._embed_with_adaptive_splitting(batch)
            all_embeddings.extend(batch_embeddings)
            embedded_count += len(batch_embeddings)
            if on_progress:
                on_progress(embedded_count, total_texts)

        return all_embeddings

    def _embed_with_adaptive_splitting(self, texts: list[str]) -> list[list[float]]:
        """
        CPU-only setups can timeout on larger embedding batches.
        If a batch times out, recursively split it into smaller sub-batches.
        """
        try:
            return self.embed_texts(texts)
        except requests.ReadTimeout as exc:
            if len(texts) <= 1:
                raise RuntimeError(
                    "Embedding request timed out for a single text. "
                    "Increase timeout and/or reduce chunk size."
                ) from exc

            mid = len(texts) // 2
            left = self._embed_with_adaptive_splitting(texts[:mid])
            right = self._embed_with_adaptive_splitting(texts[mid:])
            return left + right
