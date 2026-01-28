from __future__ import annotations

from money_map import core
from money_map.core.models import AppData, PlanResult
from money_map.ui import state as ui_state


def render(app_data: AppData) -> None:
    import streamlit as st

    ui_state.ensure_state_defaults(st.session_state)
    profile = ui_state.get_profile(st.session_state)
    selected_variant_id = st.session_state.get("selected_variant_id")

    st.subheader("Plan")
    if not selected_variant_id:
        st.info("Select a recommendation to build a plan.")
        return

    with st.spinner("Building plan..."):
        plan_result = _get_plan_cached(profile, app_data, selected_variant_id)

    st.session_state["plan_result"] = plan_result

    st.markdown(f"### {plan_result.title}")
    st.markdown("**Steps**")
    for step in plan_result.steps:
        st.write(f"- {step.title}: {step.details}")

    st.markdown("**4-week view**")
    st.table({week: ", ".join(items) for week, items in plan_result.week_view.items()})

    st.markdown("**Outputs**")
    st.write(", ".join(plan_result.outputs))

    st.markdown("**Checkpoints**")
    st.write(", ".join(plan_result.checkpoints))

    st.markdown("**Compliance kits**")
    for kit, outputs in plan_result.compliance_kits.items():
        st.write(f"- {kit}: {', '.join(outputs)}")

    st.markdown("**Aggregated checklist**")
    for item in plan_result.legal_checklist:
        st.write(f"- {item}")


def _get_plan_cached(profile, app_data: AppData, variant_id: str) -> PlanResult:
    import streamlit as st

    profile_key = ui_state.profile_hash(profile)
    dataset_version = app_data.meta.dataset_version

    @st.cache_data
    def _cached(profile_key: str, variant_id: str, dataset_version: str) -> PlanResult:
        return core.build_plan(profile, app_data, variant_id)

    return _cached(profile_key, variant_id, dataset_version)
