#!/usr/bin/env python3
"""
chunker_yaml.py

OpenAPI YAML chunker for retrieval / RAG.

Design goals:
- Keep the OpenAPI file unchanged.
- Emit one chunk per HTTP operation.
- Emit one chunk for top-level API metadata (info/servers/tags).
- Merge path-level and operation-level parameters.
- Resolve common OpenAPI $ref values.
- Produce normalized text that is good for both embeddings and keyword search.
- Optionally emit schema chunks too.

Usage:
    python chunker_yaml.py /path/to/openapi.yaml
    python chunker_yaml.py /path/to/openapi.yaml --out chunks.json

Output format:
[
  {
    "id": "openapi_operation:GET:/notes",
    "title": "GET /notes",
    "chunk_type": "openapi_operation",
    "text": "... normalized retrieval text ...",
    "raw_fragment": {...},
    "metadata": {...}
  },
  ...
]
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import yaml


HTTP_METHODS = {"get", "post", "put", "delete", "patch", "options", "head"}
PATH_LEVEL_KEYS = {"summary", "description", "servers", "parameters"}
DEFAULT_MAX_EXAMPLE_CHARS = 600


def load_yaml_file(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        data = yaml.safe_load(f)
    if not isinstance(data, dict):
        raise ValueError("Expected top-level YAML object to be a mapping/dict.")
    return data


def make_ref_map(spec: Dict[str, Any]) -> Dict[str, Any]:
    """
    Build a JSON-pointer-like lookup map for local refs such as:
      #/components/parameters/TagQuery
      #/components/responses/InternalError
      #/components/schemas/Note
    """
    ref_map: Dict[str, Any] = {}

    def walk(node: Any, current_path: str) -> None:
        ref_map[current_path] = node
        if isinstance(node, dict):
            for key, value in node.items():
                walk(value, f"{current_path}/{key}")
        elif isinstance(node, list):
            for i, value in enumerate(node):
                walk(value, f"{current_path}/{i}")

    walk(spec, "#")
    return ref_map


def resolve_local_ref(ref: str, ref_map: Dict[str, Any]) -> Any:
    if not ref.startswith("#/"):
        raise ValueError(f"Only local refs are supported right now, got: {ref}")
    if ref not in ref_map:
        raise KeyError(f"Reference not found: {ref}")
    return ref_map[ref]


def resolve_ref_if_needed(node: Any, ref_map: Dict[str, Any]) -> Any:
    """
    If the node is {'$ref': '...'} resolve it once.
    Otherwise return node unchanged.
    """
    if isinstance(node, dict) and "$ref" in node and len(node) == 1:
        return copy.deepcopy(resolve_local_ref(node["$ref"], ref_map))
    return node


def deep_resolve(node: Any, ref_map: Dict[str, Any], seen: Optional[set[str]] = None) -> Any:
    """
    Recursively resolve local $ref.
    Keeps structure otherwise intact.
    """
    if seen is None:
        seen = set()

    if isinstance(node, dict):
        if "$ref" in node and len(node) == 1:
            ref = node["$ref"]
            if ref in seen:
                # Prevent recursive loops; keep unresolved ref in that case.
                return copy.deepcopy(node)
            seen.add(ref)
            resolved = resolve_local_ref(ref, ref_map)
            return deep_resolve(copy.deepcopy(resolved), ref_map, seen)

        return {k: deep_resolve(v, ref_map, seen.copy()) for k, v in node.items()}

    if isinstance(node, list):
        return [deep_resolve(item, ref_map, seen.copy()) for item in node]

    return node


def stable_chunk_id(prefix: str, *parts: str) -> str:
    joined = "|".join(parts)
    digest = hashlib.sha1(joined.encode("utf-8")).hexdigest()[:16]
    safe_parts = ":".join(parts)
    return f"{prefix}:{safe_parts}:{digest}"


def shorten_text(text: str, limit: int = DEFAULT_MAX_EXAMPLE_CHARS) -> str:
    text = " ".join(text.split())
    if len(text) <= limit:
        return text
    return text[: limit - 3] + "..."


def json_compact(value: Any, limit: int = DEFAULT_MAX_EXAMPLE_CHARS) -> str:
    try:
        text = json.dumps(value, ensure_ascii=False, separators=(",", ":"))
    except Exception:
        text = str(value)
    return shorten_text(text, limit=limit)


def method_intent_aliases(method: str, path: str, summary: str = "", description: str = "") -> List[str]:
    method = method.upper()
    aliases: List[str] = [f"{method} {path}", f"{path} {method.lower()}"]

    method_verbs = {
        "GET": ["get", "read", "retrieve", "fetch"],
        "POST": ["create", "add", "insert", "post"],
        "PUT": ["update", "replace", "put"],
        "DELETE": ["delete", "remove"],
        "PATCH": ["patch", "modify", "update"],
    }

    verbs = method_verbs.get(method, [method.lower()])

    # Domain-specific aliases for your notes API
    if method == "GET" and path == "/notes":
        aliases.extend([
            "list notes",
            "get notes",
            "get all notes",
            "retrieve notes",
            "filter notes by tag",
        ])
    elif method == "POST" and path == "/notes":
        aliases.extend([
            "create note",
            "add note",
            "create a new note",
            "post note",
        ])
    elif method == "GET" and path == "/notes/{id}":
        aliases.extend([
            "get note by id",
            "get single note",
            "retrieve one note",
            "fetch a note",
            "fetch single note",
        ])
    elif method == "PUT" and path == "/notes/{id}":
        aliases.extend([
            "update note by id",
            "replace note",
            "edit note",
        ])
    elif method == "DELETE" and path == "/notes/{id}":
        aliases.extend([
            "delete note by id",
            "remove note",
        ])
    else:
        for verb in verbs:
            aliases.append(f"{verb} {path}")
            aliases.append(f"{verb} endpoint {path}")

    if summary:
        aliases.append(summary)
    if description:
        desc = " ".join(description.split())
        if len(desc) < 140:
            aliases.append(desc)

    # dedupe while preserving order
    seen = set()
    result = []
    for item in aliases:
        key = item.strip().lower()
        if key and key not in seen:
            seen.add(key)
            result.append(item.strip())
    return result


def schema_name_from_ref(ref: str) -> Optional[str]:
    prefix = "#/components/schemas/"
    if ref.startswith(prefix):
        return ref[len(prefix):]
    return None


def summarize_schema(schema: Any, ref_map: Dict[str, Any]) -> str:
    schema = deep_resolve(schema, ref_map)

    if not isinstance(schema, dict):
        return str(schema)

    schema_type = schema.get("type")
    if "$ref" in schema:
        name = schema_name_from_ref(schema["$ref"])
        return f"schema ref {name or schema['$ref']}"

    if schema_type == "array":
        items = schema.get("items", {})
        item_summary = summarize_schema(items, ref_map)
        return f"array of {item_summary}"

    if schema_type == "object":
        props = schema.get("properties", {})
        required = schema.get("required", [])
        prop_names = list(props.keys())
        req_text = f", required={required}" if required else ""
        return f"object properties={prop_names}{req_text}"

    if schema_type:
        return schema_type

    if "enum" in schema:
        return f"enum {schema['enum']}"

    return json_compact(schema, limit=160)


def collect_examples_from_media_type(media_type_obj: Dict[str, Any]) -> List[str]:
    lines: List[str] = []

    examples = media_type_obj.get("examples")
    if isinstance(examples, dict):
        for name, ex in examples.items():
            if isinstance(ex, dict):
                summary = ex.get("summary", "")
                value = ex.get("value")
                label = f"{name}: {summary}" if summary else name
                lines.append(f"{label} = {json_compact(value)}")

    example = media_type_obj.get("example")
    if example is not None:
        lines.append(f"example = {json_compact(example)}")

    return lines


def describe_parameter(param: Dict[str, Any], ref_map: Dict[str, Any]) -> str:
    param = deep_resolve(param, ref_map)
    if not isinstance(param, dict):
        return str(param)

    name = param.get("name", "")
    location = param.get("in", "")
    required = param.get("required", False)
    description = " ".join(str(param.get("description", "")).split())
    schema = param.get("schema", {})
    schema_summary = summarize_schema(schema, ref_map)

    parts = [f"{name} ({location})"]
    parts.append(f"required={required}")
    if schema_summary:
        parts.append(f"type={schema_summary}")
    if description:
        parts.append(f"description={description}")
    if "example" in param:
        parts.append(f"example={json_compact(param['example'], limit=120)}")

    return ", ".join(parts)


def describe_request_body(request_body: Dict[str, Any], ref_map: Dict[str, Any]) -> List[str]:
    request_body = deep_resolve(request_body, ref_map)
    if not isinstance(request_body, dict):
        return [str(request_body)]

    lines: List[str] = []
    required = request_body.get("required", False)
    lines.append(f"required={required}")

    content = request_body.get("content", {})
    if isinstance(content, dict):
        for content_type, media_type_obj in content.items():
            lines.append(f"content-type={content_type}")
            if isinstance(media_type_obj, dict):
                schema = media_type_obj.get("schema")
                if schema is not None:
                    lines.append(f"schema={summarize_schema(schema, ref_map)}")
                    if isinstance(schema, dict) and "$ref" in schema:
                        ref_name = schema_name_from_ref(schema["$ref"])
                        if ref_name:
                            lines.append(f"schema-name={ref_name}")

                for ex_line in collect_examples_from_media_type(media_type_obj):
                    lines.append(f"example: {ex_line}")

    return lines


def describe_response(response_obj: Dict[str, Any], ref_map: Dict[str, Any]) -> List[str]:
    response_obj = deep_resolve(response_obj, ref_map)
    if not isinstance(response_obj, dict):
        return [str(response_obj)]

    lines: List[str] = []
    description = " ".join(str(response_obj.get("description", "")).split())
    if description:
        lines.append(f"description={description}")

    content = response_obj.get("content", {})
    if isinstance(content, dict):
        for content_type, media_type_obj in content.items():
            lines.append(f"content-type={content_type}")
            if isinstance(media_type_obj, dict):
                schema = media_type_obj.get("schema")
                if schema is not None:
                    lines.append(f"schema={summarize_schema(schema, ref_map)}")
                    if isinstance(schema, dict) and "$ref" in schema:
                        ref_name = schema_name_from_ref(schema["$ref"])
                        if ref_name:
                            lines.append(f"schema-name={ref_name}")

                for ex_line in collect_examples_from_media_type(media_type_obj):
                    lines.append(f"example: {ex_line}")

    return lines


def merge_parameters(
    path_parameters: List[Any],
    operation_parameters: List[Any],
    ref_map: Dict[str, Any],
) -> List[Dict[str, Any]]:
    """
    Merge path-level and operation-level parameters.

    OpenAPI override rule:
    - operation-level parameter with same (name, in) overrides path-level one.
    """
    merged: Dict[Tuple[str, str], Dict[str, Any]] = {}

    for source_list in (path_parameters, operation_parameters):
        for raw_param in source_list:
            param = deep_resolve(raw_param, ref_map)
            if not isinstance(param, dict):
                continue
            key = (str(param.get("name", "")), str(param.get("in", "")))
            merged[key] = param

    return list(merged.values())


def build_operation_text(
    path: str,
    method: str,
    operation: Dict[str, Any],
    path_item: Dict[str, Any],
    ref_map: Dict[str, Any],
) -> str:
    method = method.upper()
    summary = str(operation.get("summary", "")).strip()
    description = str(operation.get("description", "")).strip()
    operation_id = str(operation.get("operationId", "")).strip()
    tags = operation.get("tags", [])

    path_parameters = path_item.get("parameters", []) if isinstance(path_item, dict) else []
    operation_parameters = operation.get("parameters", []) if isinstance(operation, dict) else []
    merged_parameters = merge_parameters(path_parameters, operation_parameters, ref_map)

    request_body = operation.get("requestBody")
    responses = operation.get("responses", {})

    lines: List[str] = []
    lines.append(f"Title: {method} {path}")
    lines.append("Type: OpenAPI operation")
    lines.append(f"Path: {path}")
    lines.append(f"Method: {method}")

    if tags:
        lines.append(f"Tags: {', '.join(map(str, tags))}")
    if summary:
        lines.append(f"Summary: {summary}")
    if description:
        lines.append("Description:")
        lines.append(description)
    if operation_id:
        lines.append(f"Operation ID: {operation_id}")

    if merged_parameters:
        lines.append("Parameters:")
        for param in merged_parameters:
            lines.append(f"- {describe_parameter(param, ref_map)}")

    if request_body is not None:
        lines.append("Request body:")
        for line in describe_request_body(request_body, ref_map):
            lines.append(f"- {line}")

    if isinstance(responses, dict) and responses:
        lines.append("Responses:")
        for status_code, response_obj in responses.items():
            resolved_response = deep_resolve(response_obj, ref_map)
            lines.append(f"- {status_code}")
            for line in describe_response(resolved_response, ref_map):
                lines.append(f"  - {line}")

    lines.append("Aliases:")
    for alias in method_intent_aliases(method, path, summary=summary, description=description):
        lines.append(f"- {alias}")

    return "\n".join(lines)


def build_schema_text(schema_name: str, schema_obj: Dict[str, Any], ref_map: Dict[str, Any]) -> str:
    schema_obj = deep_resolve(schema_obj, ref_map)
    lines: List[str] = []

    lines.append(f"Title: Schema {schema_name}")
    lines.append("Type: OpenAPI schema")

    schema_type = schema_obj.get("type", "")
    if schema_type:
        lines.append(f"Schema Type: {schema_type}")

    description = str(schema_obj.get("description", "")).strip()
    if description:
        lines.append("Description:")
        lines.append(description)

    required = schema_obj.get("required", [])
    if required:
        lines.append(f"Required fields: {', '.join(map(str, required))}")

    additional_properties = schema_obj.get("additionalProperties")
    if additional_properties is not None:
        lines.append(f"additionalProperties: {additional_properties}")

    properties = schema_obj.get("properties", {})
    if isinstance(properties, dict) and properties:
        lines.append("Properties:")
        for prop_name, prop_obj in properties.items():
            prop_obj = deep_resolve(prop_obj, ref_map)
            prop_type = summarize_schema(prop_obj, ref_map)
            prop_desc = " ".join(str(prop_obj.get("description", "")).split()) if isinstance(prop_obj, dict) else ""
            parts = [f"{prop_name}: {prop_type}"]
            if prop_desc:
                parts.append(f"description={prop_desc}")
            if isinstance(prop_obj, dict) and "example" in prop_obj:
                parts.append(f"example={json_compact(prop_obj['example'], limit=120)}")
            if isinstance(prop_obj, dict) and "enum" in prop_obj:
                parts.append(f"enum={json_compact(prop_obj['enum'], limit=120)}")
            lines.append("- " + ", ".join(parts))

    if "example" in schema_obj:
        lines.append(f"Example: {json_compact(schema_obj['example'])}")

    lines.append("Aliases:")
    lines.append(f"- schema {schema_name}")
    lines.append(f"- {schema_name}")
    lines.append(f"- fields of {schema_name}")

    return "\n".join(lines)


def extract_top_level_metadata_chunks(spec: Dict[str, Any], source_path: str) -> List[Dict[str, Any]]:
    chunks: List[Dict[str, Any]] = []

    info = spec.get("info", {})
    if not isinstance(info, dict):
        info = {}

    title = str(info.get("title", "")).strip()
    version = str(info.get("version", "")).strip()
    description = str(info.get("description", "")).strip()

    contact = info.get("contact", {})
    if not isinstance(contact, dict):
        contact = {}

    servers = spec.get("servers", [])
    if not isinstance(servers, list):
        servers = []

    tags = spec.get("tags", [])
    if not isinstance(tags, list):
        tags = []

    paths = spec.get("paths", {})
    path_count = len(paths) if isinstance(paths, dict) else 0

    operation_count = 0
    if isinstance(paths, dict):
        for path_item in paths.values():
            if not isinstance(path_item, dict):
                continue
            for key in path_item.keys():
                if str(key).lower() in HTTP_METHODS:
                    operation_count += 1

    components = spec.get("components", {})
    if not isinstance(components, dict):
        components = {}

    schema_count = len(components.get("schemas", {})) if isinstance(components.get("schemas"), dict) else 0
    parameter_count = len(components.get("parameters", {})) if isinstance(components.get("parameters"), dict) else 0
    response_count = len(components.get("responses", {})) if isinstance(components.get("responses"), dict) else 0

    lines: List[str] = [
        "Title: API metadata",
        "Type: OpenAPI top-level metadata",
    ]

    openapi_version = str(spec.get("openapi", "")).strip()
    if openapi_version:
        lines.append(f"OpenAPI Version: {openapi_version}")
    if title:
        lines.append(f"API Title: {title}")
    if version:
        lines.append(f"API Version: {version}")
    if description:
        lines.append("API Description:")
        lines.append(description)

    contact_fields: List[str] = []
    for key in ("name", "email", "url"):
        value = str(contact.get(key, "")).strip()
        if value:
            contact_fields.append(f"{key}={value}")
    if contact_fields:
        lines.append("Contact:")
        lines.append("- " + ", ".join(contact_fields))

    if servers:
        lines.append("Servers:")
        for server in servers:
            if not isinstance(server, dict):
                continue
            url = str(server.get("url", "")).strip()
            server_desc = " ".join(str(server.get("description", "")).split())
            if not url and not server_desc:
                continue
            if server_desc:
                lines.append(f"- url={url}, description={server_desc}")
            else:
                lines.append(f"- url={url}")

    if tags:
        lines.append("Tags:")
        for tag in tags:
            if not isinstance(tag, dict):
                continue
            tag_name = str(tag.get("name", "")).strip()
            tag_desc = " ".join(str(tag.get("description", "")).split())
            if not tag_name and not tag_desc:
                continue
            if tag_desc:
                lines.append(f"- {tag_name}: {tag_desc}")
            else:
                lines.append(f"- {tag_name}")

    lines.append("API Shape:")
    lines.append(f"- paths={path_count}")
    lines.append(f"- operations={operation_count}")
    lines.append(f"- component-schemas={schema_count}")
    lines.append(f"- component-parameters={parameter_count}")
    lines.append(f"- component-responses={response_count}")

    lines.append("Aliases:")
    lines.append("- api metadata")
    lines.append("- openapi top level metadata")
    lines.append("- api overview")
    lines.append("- base url")
    lines.append("- server url")

    top_level_fragment = {
        "openapi": copy.deepcopy(spec.get("openapi")),
        "info": copy.deepcopy(spec.get("info")),
        "servers": copy.deepcopy(spec.get("servers")),
        "tags": copy.deepcopy(spec.get("tags")),
    }

    chunks.append({
        "id": stable_chunk_id("openapi_meta", "top_level"),
        "title": "API metadata",
        "chunk_type": "openapi_top_level_metadata",
        "text": "\n".join(lines),
        "raw_fragment": top_level_fragment,
        "metadata": {
            "source": source_path,
            "doc_type": "openapi_top_level_metadata",
            "openapi_version": openapi_version,
            "api_title": title,
            "api_version": version,
            "paths_count": path_count,
            "operations_count": operation_count,
            "schemas_count": schema_count,
        },
    })

    return chunks


def extract_operation_chunks(spec: Dict[str, Any], source_path: str) -> List[Dict[str, Any]]:
    ref_map = make_ref_map(spec)
    chunks: List[Dict[str, Any]] = []

    paths = spec.get("paths", {})
    if not isinstance(paths, dict):
        return chunks

    for path, path_item in paths.items():
        if not isinstance(path_item, dict):
            continue

        for key, operation in path_item.items():
            method = str(key).lower()
            if method not in HTTP_METHODS:
                continue
            if not isinstance(operation, dict):
                continue

            title = f"{method.upper()} {path}"
            text = build_operation_text(path, method, operation, path_item, ref_map)

            chunk = {
                "id": stable_chunk_id("openapi_operation", method.upper(), path),
                "title": title,
                "chunk_type": "openapi_operation",
                "text": text,
                "raw_fragment": {
                    "path": path,
                    "method": method.upper(),
                    "path_item_parameters": copy.deepcopy(path_item.get("parameters", [])),
                    "operation": copy.deepcopy(operation),
                },
                "metadata": {
                    "source": source_path,
                    "doc_type": "openapi_operation",
                    "path": path,
                    "method": method.upper(),
                    "operation_id": operation.get("operationId"),
                    "tags": operation.get("tags", []),
                    "summary": operation.get("summary", ""),
                },
            }
            chunks.append(chunk)

    return chunks


def extract_schema_chunks(spec: Dict[str, Any], source_path: str) -> List[Dict[str, Any]]:
    ref_map = make_ref_map(spec)
    chunks: List[Dict[str, Any]] = []

    schemas = spec.get("components", {}).get("schemas", {})
    if not isinstance(schemas, dict):
        return chunks

    for schema_name, schema_obj in schemas.items():
        if not isinstance(schema_obj, dict):
            continue

        text = build_schema_text(schema_name, schema_obj, ref_map)

        chunk = {
            "id": stable_chunk_id("openapi_schema", schema_name),
            "title": f"Schema {schema_name}",
            "chunk_type": "openapi_schema",
            "text": text,
            "raw_fragment": copy.deepcopy(schema_obj),
            "metadata": {
                "source": source_path,
                "doc_type": "openapi_schema",
                "schema_name": schema_name,
                "title": f"Schema {schema_name}",
            },
        }
        chunks.append(chunk)

    return chunks


def extract_path_summary_chunks(spec: Dict[str, Any], source_path: str) -> List[Dict[str, Any]]:
    """
    Optional helper chunk type:
    one summary chunk per path, listing supported methods.
    """
    chunks: List[Dict[str, Any]] = []
    paths = spec.get("paths", {})
    if not isinstance(paths, dict):
        return chunks

    for path, path_item in paths.items():
        if not isinstance(path_item, dict):
            continue

        methods = [k.upper() for k in path_item.keys() if str(k).lower() in HTTP_METHODS]
        if not methods:
            continue

        lines = [
            f"Title: Path {path}",
            "Type: OpenAPI path summary",
            f"Path: {path}",
            f"Supported methods: {', '.join(methods)}",
            "Aliases:",
            f"- path {path}",
            f"- methods for {path}",
            f"- what can I do with {path}",
        ]

        chunks.append({
            "id": stable_chunk_id("openapi_path", path),
            "title": f"Path {path}",
            "chunk_type": "openapi_path_summary",
            "text": "\n".join(lines),
            "raw_fragment": copy.deepcopy(path_item),
            "metadata": {
                "source": source_path,
                "doc_type": "openapi_path_summary",
                "path": path,
                "methods": methods,
            },
        })

    return chunks


def extract_all_chunks(
    spec: Dict[str, Any],
    source_path: str,
    include_top_level_metadata: bool = True,
    include_schemas: bool = True,
    include_path_summaries: bool = True,
) -> List[Dict[str, Any]]:
    chunks: List[Dict[str, Any]] = []

    if include_top_level_metadata:
        chunks.extend(extract_top_level_metadata_chunks(spec, source_path))

    chunks.extend(extract_operation_chunks(spec, source_path))

    if include_schemas:
        chunks.extend(extract_schema_chunks(spec, source_path))

    if include_path_summaries:
        chunks.extend(extract_path_summary_chunks(spec, source_path))

    return chunks


def main() -> int:
    parser = argparse.ArgumentParser(description="Chunk OpenAPI YAML into retrieval-ready JSON chunks.")
    parser.add_argument("yaml_path", help="Path to openapi.yaml")
    parser.add_argument("--out", help="Output JSON file path. If omitted, prints to stdout.")
    parser.add_argument("--no-top-level-metadata", action="store_true", help="Do not emit top-level OpenAPI metadata chunk.")
    parser.add_argument("--no-schemas", action="store_true", help="Do not emit schema chunks.")
    parser.add_argument("--no-path-summaries", action="store_true", help="Do not emit path summary chunks.")
    parser.add_argument("--pretty", action="store_true", help="Pretty-print JSON output.")
    args = parser.parse_args()

    yaml_path = Path(args.yaml_path)
    spec = load_yaml_file(yaml_path)

    chunks = extract_all_chunks(
        spec=spec,
        source_path=str(yaml_path),
        include_top_level_metadata=not args.no_top_level_metadata,
        include_schemas=not args.no_schemas,
        include_path_summaries=not args.no_path_summaries,
    )

    output = json.dumps(
        chunks,
        ensure_ascii=False,
        indent=2 if args.pretty else None,
    )

    if args.out:
        out_path = Path(args.out)
        out_path.write_text(output, encoding="utf-8")
    else:
        print(output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
