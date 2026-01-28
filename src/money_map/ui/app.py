from __future__ import annotations

from pathlib import Path
from typing import Optional

from money_map import core
from money_map.ui import state as ui_state
from money_map.ui.views import data_status, export, plan, profile, recommendations


PAGES = {
    "Data status": data_status.render,
    "Profile": profile.render,
    "Recommendations": recommendations.render,
    "Plan": plan.render,
    "Export": export.render,
}


def get_app_data(country: str, data_dir: Optional[Path]) -> core.AppData:
    import streamlit as st

    data_dir_str = str(data_dir) if data_dir else ""

    @st.cache_resource
    def _load(country_key: str, data_dir_key: str) -> core.AppData:
        data_path = Path(data_dir_key) if data_dir_key else None
        return core.load_app_data(country_key, data_path)

    return _load(country, data_dir_str)


def main() -> None:
    import streamlit as st

    ui_state.ensure_state_defaults(st.session_state)
    st.set_page_config(page_title="Money Map MVP", layout="wide")

    st.sidebar.title("Navigation")
    page = st.sidebar.radio("Go to", list(PAGES.keys()))
    country = st.sidebar.selectbox("Country", ["DE", "FR", "NL", "ES"], index=0)

    data_dir_input = st.sidebar.text_input("Data dir (optional)", value="")
    data_dir = Path(data_dir_input) if data_dir_input else None

    st.session_state["country"] = country

    app_data = get_app_data(country, data_dir)
    report = core.validate_app_data(app_data)
    status = "invalid" if report.has_fatal else "stale" if report.has_stale else "valid"

    st.sidebar.markdown("---")
    st.sidebar.write(f"Dataset: {app_data.meta.dataset_version}")
    st.sidebar.write(f"Status: {status}")

    if page in {"Recommendations", "Plan", "Export", "Data status"}:
        PAGES[page](app_data)
    else:
        PAGES[page]()


if __name__ == "__main__":
    main()
