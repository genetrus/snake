from __future__ import annotations

from money_map import core
from money_map.core.models import UserProfile


def test_plan_includes_compliance_details() -> None:
    profile = UserProfile(
        hours_per_week=12,
        capital_eur=250,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="fastest_money",
        startability_window_weeks=4,
    )
    app_data = core.load_app_data(country="DE", data_dir=None)
    variant_id = app_data.variants[0]["variant_id"]
    plan = core.build_plan(profile, app_data, variant_id)
    assert plan.compliance_kits
    assert plan.legal_checklist
