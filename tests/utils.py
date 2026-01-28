from __future__ import annotations

from pathlib import Path

from money_map.core.models import UserProfile
from money_map.core.profile_io import load_profile_yaml


def load_profile(path: Path) -> UserProfile:
    return load_profile_yaml(path)
