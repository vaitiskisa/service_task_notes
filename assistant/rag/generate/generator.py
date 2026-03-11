from __future__ import annotations

from dataclasses import dataclass
import os
from typing import Any, Callable

from azure.ai.inference import ChatCompletionsClient
from azure.ai.inference.models import SystemMessage, UserMessage
from azure.core.credentials import AzureKeyCredential
from azure.core.exceptions import (
    ClientAuthenticationError,
    HttpResponseError,
    ServiceRequestError,
)


@dataclass
class GenerationResult:
    answer: str
    raw_response: dict[str, Any]


class OllamaGenerator:
    def __init__(
        self,
        model: str = "meta/Llama-4-Scout-17B-16E-Instruct",
        base_url: str = "https://models.github.ai/inference",
        timeout: int = 300,
        temperature: float = 0.1,
        top_p: float = 1.0,
        max_tokens: int = 1000,
        token: str | None = None,
        token_env_var: str = "GITHUB_TOKEN",
    ) -> None:
        self.model = model
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.temperature = temperature
        self.top_p = top_p
        self.max_tokens = max_tokens
        self.token_env_var = token_env_var

        resolved_token = token or os.getenv(token_env_var)
        if not resolved_token:
            raise RuntimeError(
                f"Missing token for generation. Set environment variable '{token_env_var}'."
            )

        self.client = ChatCompletionsClient(
            endpoint=self.base_url,
            credential=AzureKeyCredential(resolved_token),
        )

    def _build_chat_kwargs(
        self,
        system_prompt: str,
        user_prompt: str,
        stream: bool,
        options: dict[str, Any] | None,
    ) -> dict[str, Any]:
        merged_options = dict(options or {})

        kwargs: dict[str, Any] = {
            "messages": [
                SystemMessage(system_prompt),
                UserMessage(user_prompt),
            ],
            "stream": stream,
            "model": merged_options.pop("model", self.model),
            "temperature": merged_options.pop("temperature", self.temperature),
            "top_p": merged_options.pop("top_p", self.top_p),
            "max_tokens": merged_options.pop("max_tokens", self.max_tokens),
            "timeout": self.timeout,
        }

        passthrough_fields = [
            "frequency_penalty",
            "presence_penalty",
            "response_format",
            "stop",
            "tools",
            "tool_choice",
            "seed",
            "model_extras",
        ]

        for field in passthrough_fields:
            if field in merged_options:
                kwargs[field] = merged_options[field]

        return kwargs

    @staticmethod
    def _extract_response_answer(response: Any) -> str:
        choices = getattr(response, "choices", None)
        if not isinstance(choices, list) or not choices:
            raise RuntimeError("Invalid chat response: missing choices")

        message = getattr(choices[0], "message", None)
        content = getattr(message, "content", None)

        if not isinstance(content, str):
            raise RuntimeError("Invalid chat response: missing message content")

        return content.strip()

    def chat(
        self,
        system_prompt: str,
        user_prompt: str,
        stream: bool = False,
        options: dict[str, Any] | None = None,
        on_token: Callable[[str], None] | None = None,
    ) -> GenerationResult:
        kwargs = self._build_chat_kwargs(
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            stream=stream,
            options=options,
        )

        try:
            response = self.client.complete(**kwargs)
        except ClientAuthenticationError as exc:
            raise RuntimeError(
                f"Authentication failed for generation endpoint '{self.base_url}'. "
                "Check GITHUB_TOKEN."
            ) from exc
        except ServiceRequestError as exc:
            raise RuntimeError(
                f"Could not reach generation endpoint '{self.base_url}'."
            ) from exc
        except HttpResponseError as exc:
            status = getattr(exc, "status_code", "unknown")
            error_message = getattr(exc, "message", str(exc))
            raise RuntimeError(
                f"Generation request failed (status={status}): {error_message}"
            ) from exc

        if not stream:
            return GenerationResult(
                answer=self._extract_response_answer(response),
                raw_response=response.as_dict(),
            )

        full_answer_parts: list[str] = []
        chunks: list[dict[str, Any]] = []

        for update in response:
            chunks.append(update.as_dict())
            choices = getattr(update, "choices", None) or []
            for choice in choices:
                delta = getattr(choice, "delta", None)
                content_piece = getattr(delta, "content", None)
                if isinstance(content_piece, str) and content_piece:
                    full_answer_parts.append(content_piece)
                    if on_token:
                        on_token(content_piece)

        return GenerationResult(
            answer="".join(full_answer_parts).strip(),
            raw_response={"chunks": chunks},
        )
