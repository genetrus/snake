from __future__ import annotations

from money_map import core


def test_economics_snapshot_present() -> None:
    app_data = core.load_app_data(country="DE", data_dir=None)
    variant = next(v for v in app_data.variants if v.get("economics"))
    economics = core.evaluate_economics(variant)
    assert economics is not None
    assert economics.net_range_eur is not None
