from __future__ import annotations

from money_map import core
from money_map.core.models import UserProfile


def test_rules_legal_gate_respects_max() -> None:
    profile = UserProfile(
        hours_per_week=15,
        capital_eur=200,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="fastest_money",
        startability_window_weeks=4,
        max_legal_friction="ok",
    )
    app_data = core.load_app_data(country="DE", data_dir=None)
    result = core.recommend(profile, app_data)
    assert result.recommendations
    assert all(rec.legal.gate == "ok" for rec in result.recommendations)
