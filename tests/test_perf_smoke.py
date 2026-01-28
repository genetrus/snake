from __future__ import annotations

import copy
import os
import time

import pytest

from money_map import core
from money_map.core.models import AppData, AppMeta, RulepackMeta, UserProfile


@pytest.mark.perf
def test_perf_smoke_validate_and_recommend() -> None:
    if os.getenv("MM_RUN_PERF") != "1":
        pytest.skip("Perf smoke disabled. Set MM_RUN_PERF=1 to run.")

    base_app_data = core.load_app_data(country="DE", data_dir=None)
    base_variants = base_app_data.variants
    synthetic_variants = []
    for index in range(1000):
        variant = copy.deepcopy(base_variants[index % len(base_variants)])
        variant["variant_id"] = f"synthetic_{index}"
        variant["title"] = f"Synthetic {index}"
        synthetic_variants.append(variant)

    app_data = AppData(
        meta=AppMeta(
            dataset_version=base_app_data.meta.dataset_version,
            warnings=list(base_app_data.meta.warnings),
        ),
        rulepack=RulepackMeta(reviewed_at=base_app_data.rulepack.reviewed_at),
        variants=synthetic_variants,
    )

    profile = UserProfile(
        hours_per_week=15,
        capital_eur=200,
        language_cefr="B2",
        assets=["laptop"],
        objective_preset="fastest_money",
        startability_window_weeks=4,
    )

    start = time.perf_counter()
    report = core.validate_app_data(app_data)
    validate_time = time.perf_counter() - start
    assert report.summary == "OK"
    assert validate_time <= 2.0

    start = time.perf_counter()
    result = core.recommend(profile, app_data)
    recommend_time = time.perf_counter() - start
    assert result.recommendations
    assert recommend_time <= 1.5
