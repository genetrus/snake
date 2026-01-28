from __future__ import annotations

from money_map import core
from money_map.core.models import UserProfile


def test_recommend_objective_fastest_money() -> None:
    profile = UserProfile(
        hours_per_week=15,
        capital_eur=200,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="fastest_money",
        startability_window_weeks=4,
    )
    app_data = core.load_app_data(country="DE", data_dir=None)
    result = core.recommend(profile, app_data)
    assert result.recommendations[0].variant_id == "local_deliveries"


def test_recommend_objective_max_income() -> None:
    profile = UserProfile(
        hours_per_week=15,
        capital_eur=200,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="max_income",
        startability_window_weeks=4,
        max_legal_friction="registration",
    )
    app_data = core.load_app_data(country="DE", data_dir=None)
    result = core.recommend(profile, app_data)
    assert result.recommendations[0].variant_id == "marketplace_resale"
