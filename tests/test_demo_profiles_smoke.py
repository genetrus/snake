from __future__ import annotations

from pathlib import Path

import pytest

from money_map import core
from tests.utils import load_profile

DEMO_PROFILES = [
    Path("profiles/demo_fast_start.yaml"),
    Path("profiles/demo_low_risk.yaml"),
    Path("profiles/demo_max_income.yaml"),
]


def _top_variant_ids(profile_path: Path) -> list[str]:
    profile = load_profile(profile_path)
    app_data = core.load_app_data(country="DE", data_dir=None)
    result = core.recommend(profile, app_data)
    return [rec.variant_id for rec in result.recommendations[:10]]


def test_demo_profiles_are_deterministic() -> None:
    for profile_path in DEMO_PROFILES:
        first = _top_variant_ids(profile_path)
        second = _top_variant_ids(profile_path)
        assert first == second


def test_demo_profiles_feasible_rate() -> None:
    app_data = core.load_app_data(country="DE", data_dir=None)
    for profile_path in DEMO_PROFILES:
        profile = load_profile(profile_path)
        result = core.recommend(profile, app_data)
        top = result.recommendations[:10]
        assert top, f"Expected recommendations for {profile_path}"
        eligible = [rec for rec in top if getattr(rec.legal, "gate", "") != "blocked"]
        if not eligible:
            pytest.skip("All candidates blocked by legal gate")
        feasible = [
            rec
            for rec in eligible
            if getattr(rec.feasibility, "status", "") != "not_feasible"
        ]
        rate = len(feasible) / len(eligible)
        assert rate >= 0.70


def test_demo_fast_start_has_results() -> None:
    profile = load_profile(Path("profiles/demo_fast_start.yaml"))
    app_data = core.load_app_data(country="DE", data_dir=None)
    result = core.recommend(profile, app_data)
    assert len(result.recommendations) >= 1
