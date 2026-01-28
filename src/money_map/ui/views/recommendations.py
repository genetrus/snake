from __future__ import annotations

from collections import Counter

from money_map import core
from money_map.core.models import AppData, RecommendationResult
from money_map.ui import state as ui_state


def render(app_data: AppData) -> None:
    import streamlit as st

    ui_state.ensure_state_defaults(st.session_state)
    profile = ui_state.get_profile(st.session_state)

    st.subheader("Recommendations")
    _render_reality_check(app_data, profile)

    if not st.session_state.get("profile_ready"):
        st.info("Complete the profile and click Save / Apply to continue.")
        return

    st.write(f"Objective preset: **{profile.objective_preset}**")
    st.write(f"Filters: startability ≤ {profile.startability_window_weeks} weeks")

    if st.button("Run recommendations"):
        with st.spinner("Running recommendations..."):
            result = _get_recommendations_cached(profile, app_data)
        st.session_state["recommendation_result"] = result

    result = st.session_state.get("recommendation_result")
    if result is None:
        st.info("No results yet. Run recommendations.")
        return

    if not result.recommendations:
        st.info("No results")
        return

    for rec in result.recommendations:
        with st.container():
            st.markdown(f"### {rec.title}")
            st.caption(rec.variant_id)
            st.markdown("**Feasibility**")
            st.write(f"Status: {rec.feasibility.status}")
            if rec.feasibility.blockers:
                st.write("Blockers: " + ", ".join(rec.feasibility.blockers))
            st.write(f"Prep weeks: {rec.feasibility.prep_weeks}")

            st.markdown("**Legal**")
            st.write(f"Gate: {rec.legal.gate}")
            if rec.legal.stale_warning:
                st.warning(rec.legal.stale_warning)
            st.write("Checklist: " + ", ".join(rec.legal.checklist[:3]))
            st.write(f"Compliance kits: {len(rec.legal.compliance_kits)}")

            st.markdown("**Economics**")
            if rec.economics:
                st.write(
                    f"Time to first money: {rec.economics.time_to_first_money_range_weeks} weeks"
                )
                st.write(f"Net range (EUR): {rec.economics.net_range_eur}")
                st.write(f"Confidence: {rec.economics.confidence}")
            else:
                st.warning("Economics missing")

            st.markdown("**Reasons**")
            st.write("Pros: " + "; ".join(rec.reasons_for[:3]))
            if rec.reasons_against:
                st.write("Cons: " + "; ".join(rec.reasons_against[:2]))

            if st.button("Select", key=f"select_{rec.variant_id}"):
                st.session_state["selected_variant_id"] = rec.variant_id
                st.success(f"Selected {rec.variant_id}")


def _render_reality_check(app_data: AppData, profile) -> None:
    import streamlit as st

    st.markdown("## Reality Check")
    blockers = []
    for variant in app_data.variants:
        feasibility = core.evaluate_feasibility(profile, variant)
        blockers.extend(feasibility.blockers)
    counter = Counter(blockers)
    top_blockers = [item for item, _count in counter.most_common(3)]

    if top_blockers:
        st.write("Top blockers: " + ", ".join(top_blockers))
    else:
        st.write("Top blockers: None")

    st.markdown("**Quick fixes**")
    col1, col2, col3 = st.columns(3)
    rerun = False
    if col1.button("Снизить legal friction"):
        profile.max_legal_friction = "require_check"
        rerun = True
    if col2.button("Цель: fastest money"):
        profile.objective_preset = "fastest_money"
        rerun = True
    if col3.button("Только startable in 2 weeks"):
        profile.startability_window_weeks = 2
        rerun = True

    if rerun:
        ui_state.update_profile(st.session_state, profile)
        st.session_state["recommendation_result"] = _get_recommendations_cached(profile, app_data)
        st.rerun()


def _get_recommendations_cached(profile, app_data: AppData) -> RecommendationResult:
    import streamlit as st

    profile_key = ui_state.profile_hash(profile)
    dataset_version = app_data.meta.dataset_version

    @st.cache_data
    def _cached(profile_key: str, objective: str, filters: dict, dataset_version: str) -> RecommendationResult:
        return core.recommend(profile, app_data)

    return _cached(profile_key, profile.objective_preset, profile.to_dict(), dataset_version)
