from __future__ import annotations

from money_map import core
from money_map.core.models import UserProfile


def test_feasibility_engine_basic() -> None:
    profile = UserProfile(
        hours_per_week=10,
        capital_eur=200,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="fastest_money",
        startability_window_weeks=4,
    )
    app_data = core.load_app_data(country="DE", data_dir=None)
    variant = app_data.variants[0]
    feasibility = core.evaluate_feasibility(profile, variant)
    assert feasibility.status in {"ready", "blocked"}
    assert feasibility.prep_weeks >= 1
