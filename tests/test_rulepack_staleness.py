from __future__ import annotations

from money_map import core


def test_rulepack_staleness_warning(tmp_path) -> None:
    stale_flag = tmp_path / "stale.flag"
    stale_flag.write_text("1", encoding="utf-8")
    app_data = core.load_app_data(country="DE", data_dir=tmp_path)
    report = core.validate_app_data(app_data)
    assert report.has_stale
