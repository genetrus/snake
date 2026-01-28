from __future__ import annotations

from pathlib import Path

from money_map import core
from money_map.render.export import render_plan_md
from tests.utils import load_profile


def test_plan_actionability_demo_fast_start() -> None:
    profile = load_profile(Path("profiles/demo_fast_start.yaml"))
    app_data = core.load_app_data(country="DE", data_dir=None)
    recommendations = core.recommend(profile, app_data).recommendations
    assert recommendations
    top_variant_id = recommendations[0].variant_id
    plan = core.build_plan(profile, app_data, top_variant_id)

    total_actions = len(plan.steps) + sum(len(items) for items in plan.week_view.values())
    assert total_actions >= 10

    if plan.outputs:
        assert len(plan.outputs) >= 3
    else:
        plan_md = render_plan_md(plan, app_data, profile)
        outputs_lines = [line for line in plan_md.splitlines() if line.startswith("- ")]
        assert len(outputs_lines) >= 3
