from __future__ import annotations

import hashlib
import json
from dataclasses import asdict
from typing import Dict, Optional

from money_map.core.models import UserProfile


DEFAULT_PROFILE = UserProfile(
    hours_per_week=10,
    capital_eur=300,
    language_cefr="B1",
    assets=["laptop"],
    objective_preset="fastest_money",
    startability_window_weeks=4,
    include_tags=[],
    exclude_tags=[],
    max_legal_friction="ok",
)


def ensure_state_defaults(state: Dict[str, object]) -> None:
    state.setdefault("country", "DE")
    state.setdefault("profile", DEFAULT_PROFILE)
    state.setdefault("profile_ready", False)
    state.setdefault("profile_yaml_bytes", b"")
    state.setdefault("selected_variant_id", "")
    state.setdefault("recommendation_result", None)
    state.setdefault("plan_result", None)
    state.setdefault("objective_preset", DEFAULT_PROFILE.objective_preset)
    state.setdefault("filters", {})


def profile_hash(profile: UserProfile) -> str:
    payload = json.dumps(asdict(profile), sort_keys=True, ensure_ascii=False)
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()


def update_profile(state: Dict[str, object], profile: UserProfile) -> None:
    state["profile"] = profile
    state["profile_ready"] = True
    state["profile_yaml_bytes"] = "".encode("utf-8")
    state["selected_variant_id"] = ""
    state["plan_result"] = None
    state["recommendation_result"] = None


def get_profile(state: Dict[str, object]) -> UserProfile:
    profile = state.get("profile")
    if isinstance(profile, UserProfile):
        return profile
    return DEFAULT_PROFILE


def update_filters(state: Dict[str, object], filters: Dict[str, object]) -> None:
    state["filters"] = filters
