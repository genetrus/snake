from __future__ import annotations

from money_map.core.models import UserProfile
from money_map.render.export import render_profile_yaml
from money_map.ui import state as ui_state


def render() -> None:
    import streamlit as st

    ui_state.ensure_state_defaults(st.session_state)
    current_profile = ui_state.get_profile(st.session_state)

    st.subheader("Profile")
    status_label = "READY" if st.session_state.get("profile_ready") else "DRAFT"
    st.write(f"State: **{status_label}**")

    with st.form("profile_form"):
        st.markdown("### Quick profile")
        hours_per_week = st.number_input(
            "Hours per week",
            min_value=1,
            max_value=80,
            value=int(current_profile.hours_per_week),
            step=1,
        )
        capital_eur = st.number_input(
            "Capital (EUR)",
            min_value=0,
            max_value=100000,
            value=int(current_profile.capital_eur),
            step=50,
        )
        language_cefr = st.selectbox(
            "Language (CEFR)",
            ["Unknown", "A1", "A2", "B1", "B2", "C1", "C2"],
            index=["Unknown", "A1", "A2", "B1", "B2", "C1", "C2"].index(
                current_profile.language_cefr
                if current_profile.language_cefr in ["Unknown", "A1", "A2", "B1", "B2", "C1", "C2"]
                else "Unknown"
            ),
        )
        assets = st.multiselect(
            "Assets",
            ["car", "bike", "laptop", "phone", "camera", "storage"],
            default=current_profile.assets,
        )
        objective_preset = st.selectbox(
            "Objective preset",
            ["fastest_money", "lowest_risk", "max_income", "low_legal_friction"],
            index=["fastest_money", "lowest_risk", "max_income", "low_legal_friction"].index(
                current_profile.objective_preset
                if current_profile.objective_preset in ["fastest_money", "lowest_risk", "max_income", "low_legal_friction"]
                else "fastest_money"
            ),
        )
        startability_window_weeks = st.slider(
            "Startability window (weeks)",
            min_value=1,
            max_value=8,
            value=int(current_profile.startability_window_weeks),
        )

        with st.expander("Advanced", expanded=False):
            include_tags = st.text_input(
                "Include tags (comma-separated)",
                value=", ".join(current_profile.include_tags),
            )
            exclude_tags = st.text_input(
                "Exclude tags (comma-separated)",
                value=", ".join(current_profile.exclude_tags),
            )
            max_legal_friction = st.selectbox(
                "Max legal friction",
                ["ok", "require_check", "registration", "license"],
                index=["ok", "require_check", "registration", "license"].index(
                    current_profile.max_legal_friction
                    if current_profile.max_legal_friction in ["ok", "require_check", "registration", "license"]
                    else "ok"
                ),
            )

        submitted = st.form_submit_button("Save / Apply")

    if submitted:
        profile = UserProfile(
            hours_per_week=int(hours_per_week),
            capital_eur=int(capital_eur),
            language_cefr=language_cefr,
            assets=assets,
            objective_preset=objective_preset,
            startability_window_weeks=int(startability_window_weeks),
            include_tags=_parse_csv(include_tags),
            exclude_tags=_parse_csv(exclude_tags),
            max_legal_friction=max_legal_friction,
        )
        ui_state.update_profile(st.session_state, profile)
        st.session_state["profile_yaml_bytes"] = render_profile_yaml(profile).encode("utf-8")
        st.success("Profile saved.")


def _parse_csv(value: str) -> list[str]:
    return [item.strip() for item in value.split(",") if item.strip()]
