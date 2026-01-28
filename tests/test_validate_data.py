from __future__ import annotations

from money_map import core


def test_validate_data_has_no_fatal() -> None:
    app_data = core.load_app_data(country="DE", data_dir=None)
    report = core.validate_app_data(app_data)
    assert report.summary == "OK"
    assert not report.has_fatal
