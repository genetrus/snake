from __future__ import annotations

from typing import Optional

from money_map import core
from money_map.core.models import AppData


def render(app_data: AppData) -> None:
    import streamlit as st

    report = core.validate_app_data(app_data)
    state = "valid"
    if report.has_fatal:
        state = "invalid"
    elif report.has_stale:
        state = "stale"

    st.subheader("Data status")
    st.write(f"State: **{state.upper()}**")
    st.write(f"Dataset version: `{app_data.meta.dataset_version}`")
    st.write(f"Reviewed at: `{app_data.rulepack.reviewed_at}`")
    if app_data.meta.warnings:
        st.warning("Warnings: " + ", ".join(app_data.meta.warnings))

    st.markdown("### Validation report")
    st.write(f"Summary: **{report.summary}**")
    if not report.issues:
        st.success("No issues detected.")
    else:
        for issue in report.issues:
            level = st.error if issue.severity == "FATAL" else st.warning
            level(f"{issue.severity} {issue.code} ({issue.where}): {issue.message}")
