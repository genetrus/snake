from __future__ import annotations

from pathlib import Path
from typing import Any, Dict, List

from money_map.core.models import UserProfile


def _parse_value(raw: str) -> Any:
    if raw == "[]":
        return []
    if raw.isdigit():
        return int(raw)
    if raw.lower() in {"true", "false"}:
        return raw.lower() == "true"
    return raw


def parse_simple_yaml(text: str) -> Dict[str, Any]:
    data: Dict[str, Any] = {}
    current_list_key: str | None = None

    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped.endswith(":"):
            key = stripped[:-1].strip()
            data[key] = []
            current_list_key = key
            continue
        if stripped.startswith("- "):
            if current_list_key is not None:
                data[current_list_key].append(_parse_value(stripped[2:].strip()))
            continue
        if ":" in stripped:
            key, value = stripped.split(":", 1)
            data[key.strip()] = _parse_value(value.strip())
            current_list_key = None

    return data


def load_profile_yaml(path: Path) -> UserProfile:
    data = parse_simple_yaml(path.read_text(encoding="utf-8"))
    return UserProfile(
        hours_per_week=int(data["hours_per_week"]),
        capital_eur=int(data["capital_eur"]),
        language_cefr=str(data["language_cefr"]),
        assets=list(data.get("assets", [])),
        objective_preset=str(data["objective_preset"]),
        startability_window_weeks=int(data.get("startability_window_weeks", 4)),
        include_tags=list(data.get("include_tags", [])),
        exclude_tags=list(data.get("exclude_tags", [])),
        max_legal_friction=str(data.get("max_legal_friction", "ok")),
    )
