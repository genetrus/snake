from __future__ import annotations

from pathlib import Path

from money_map.render import export as export_render
from money_map.ui import state as ui_state


def render(app_data) -> None:
    import streamlit as st

    ui_state.ensure_state_defaults(st.session_state)
    plan_result = st.session_state.get("plan_result")
    recommendation_result = st.session_state.get("recommendation_result")
    profile = ui_state.get_profile(st.session_state)

    st.subheader("Export")
    if plan_result is None or recommendation_result is None:
        st.info("Plan not ready. Build a plan before exporting.")
        return

    exports_dir = Path("exports")
    exports_dir.mkdir(exist_ok=True)

    plan_md = export_render.render_plan_md(plan_result, app_data, profile)
    result_json = export_render.render_result_json(recommendation_result, plan_result, profile)
    profile_yaml = export_render.render_profile_yaml(profile)

    (exports_dir / "plan.md").write_text(plan_md, encoding="utf-8")
    (exports_dir / "result.json").write_text(result_json, encoding="utf-8")
    (exports_dir / "profile.yaml").write_text(profile_yaml, encoding="utf-8")

    st.download_button("Download plan.md", plan_md, file_name="plan.md")
    st.download_button("Download result.json", result_json, file_name="result.json")
    st.download_button("Download profile.yaml", profile_yaml, file_name="profile.yaml")
