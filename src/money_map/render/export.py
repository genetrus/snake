from __future__ import annotations

import json
from dataclasses import asdict
from typing import Any, Dict

from money_map.core.models import PlanResult, RecommendationResult, UserProfile


def render_plan_md(plan_result: PlanResult, app_data: Any, profile: UserProfile) -> str:
    lines = [f"# Plan: {plan_result.title}", "", "## Steps"]
    for step in plan_result.steps:
        lines.append(f"- **{step.title}**: {step.details}")
    lines.append("")
    lines.append("## 4-Week View")
    for week, items in plan_result.week_view.items():
        lines.append(f"### {week}")
        for item in items:
            lines.append(f"- {item}")
    lines.append("")
    lines.append("## Outputs")
    for output in plan_result.outputs:
        lines.append(f"- {output}")
    lines.append("")
    lines.append("## Checkpoints")
    for checkpoint in plan_result.checkpoints:
        lines.append(f"- {checkpoint}")
    lines.append("")
    lines.append("## Compliance")
    lines.append(f"- Legal gate: {plan_result.legal_checklist and 'requires review' or 'ok'}")
    if plan_result.legal_checklist:
        lines.append("- Legal checklist:")
        for item in plan_result.legal_checklist:
            lines.append(f"  - {item}")
    lines.append("- Compliance kits:")
    for kit, outputs in plan_result.compliance_kits.items():
        lines.append(f"  - {kit}:")
        for output in outputs:
            lines.append(f"    - {output}")
    return "\n".join(lines)


def render_result_json(
    recommendation_result: RecommendationResult,
    plan_result: PlanResult,
    profile: UserProfile,
) -> str:
    payload: Dict[str, Any] = {
        "profile": asdict(profile),
        "recommendations": {
            "objective": recommendation_result.objective,
            "filters": recommendation_result.filters,
            "applied_rules": recommendation_result.applied_rules,
            "selected_variant_id": plan_result.variant_id,
        },
        "plan": {
            "variant_id": plan_result.variant_id,
            "title": plan_result.title,
            "steps": [asdict(step) for step in plan_result.steps],
            "week_view": plan_result.week_view,
            "outputs": plan_result.outputs,
            "checkpoints": plan_result.checkpoints,
            "compliance_kits": plan_result.compliance_kits,
            "legal_checklist": plan_result.legal_checklist,
        },
    }
    return json.dumps(payload, ensure_ascii=False, indent=2)


def render_profile_yaml(profile: UserProfile) -> str:
    data = asdict(profile)
    lines = []
    for key, value in data.items():
        if isinstance(value, list):
            lines.append(f"{key}:")
            for item in value:
                lines.append(f"  - {item}")
        else:
            lines.append(f"{key}: {value}")
    return "\n".join(lines) + "\n"
