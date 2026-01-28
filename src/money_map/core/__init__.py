from __future__ import annotations

from pathlib import Path
from typing import Dict, List, Optional

from .models import (
    AppData,
    AppMeta,
    Economics,
    Feasibility,
    Legal,
    PlanResult,
    PlanStep,
    Recommendation,
    RecommendationResult,
    RulepackMeta,
    UserProfile,
    ValidationIssue,
    ValidationReport,
)


DEFAULT_VARIANTS = [
    {
        "variant_id": "local_deliveries",
        "title": "Local delivery runs",
        "tags": ["offline", "fast", "logistics"],
        "startable_weeks": 1,
        "legal_gate": "ok",
        "compliance_kits": ["basic_safety"],
        "checklist": ["Insurance verified", "Safety gear ready", "Local registration confirmed"],
        "economics": {
            "time_to_first_money_range_weeks": [1, 2],
            "net_range_eur": [150, 450],
            "confidence": "medium",
        },
    },
    {
        "variant_id": "marketplace_resale",
        "title": "Marketplace resale",
        "tags": ["offline", "inventory", "low_risk"],
        "startable_weeks": 2,
        "legal_gate": "require_check",
        "compliance_kits": ["basic_finance"],
        "checklist": ["Track inventory", "Tax category checked", "Receipts organized"],
        "economics": {
            "time_to_first_money_range_weeks": [2, 4],
            "net_range_eur": [200, 700],
            "confidence": "low",
        },
    },
    {
        "variant_id": "weekend_services",
        "title": "Weekend neighborhood services",
        "tags": ["fast", "community", "low_risk"],
        "startable_weeks": 1,
        "legal_gate": "registration",
        "compliance_kits": ["basic_safety", "basic_finance"],
        "checklist": ["Service scope written", "Local permits checked", "Pricing sheet ready"],
        "economics": None,
    },
]


def load_app_data(country: str, data_dir: Optional[Path] = None) -> AppData:
    warnings: List[str] = []
    if data_dir is not None and (data_dir / "stale.flag").exists():
        warnings.append("stale")
    meta = AppMeta(dataset_version="2025-01-01", warnings=warnings)
    rulepack = RulepackMeta(reviewed_at="2025-02-01")
    return AppData(meta=meta, rulepack=rulepack, variants=DEFAULT_VARIANTS)


def validate_app_data(app_data: AppData) -> ValidationReport:
    issues: List[ValidationIssue] = []
    if not app_data.meta.dataset_version:
        issues.append(
            ValidationIssue(
                severity="FATAL",
                code="MISSING_DATASET_VERSION",
                where="meta.dataset_version",
                message="Dataset version is missing.",
            )
        )
    if "stale" in app_data.meta.warnings:
        issues.append(
            ValidationIssue(
                severity="WARN",
                code="STALE",
                where="meta.warnings",
                message="Dataset may be out of date.",
            )
        )
    summary = "INVALID" if any(issue.severity == "FATAL" for issue in issues) else "OK"
    return ValidationReport(summary=summary, issues=issues)


def evaluate_feasibility(profile: UserProfile, variant: Dict[str, object]) -> Feasibility:
    blockers: List[str] = []
    if profile.hours_per_week < 5:
        blockers.append("Low weekly hours")
    if profile.capital_eur < 100:
        blockers.append("Low starting capital")
    if profile.language_cefr in {"A1", "A2"}:
        blockers.append("Low local language")
    assets = set(profile.assets)
    if "laptop" not in assets:
        blockers.append("No laptop available")
    status = "ready" if not blockers else "blocked"
    prep_weeks = max(1, int(variant.get("startable_weeks", 2)))
    return Feasibility(status=status, blockers=blockers[:3], prep_weeks=prep_weeks)


def evaluate_legal(profile: UserProfile, variant: Dict[str, object], app_data: AppData) -> Legal:
    gate = str(variant.get("legal_gate", "ok"))
    checklist = list(variant.get("checklist", []))
    compliance_kits = list(variant.get("compliance_kits", []))
    stale_warning = "" if "stale" not in app_data.meta.warnings else "Dataset may be stale"
    return Legal(
        gate=gate,
        checklist=checklist,
        compliance_kits=compliance_kits,
        stale_warning=stale_warning or None,
    )


def evaluate_economics(variant: Dict[str, object]) -> Optional[Economics]:
    economics = variant.get("economics")
    if not economics:
        return None
    return Economics(
        time_to_first_money_range_weeks=economics.get("time_to_first_money_range_weeks"),
        net_range_eur=economics.get("net_range_eur"),
        confidence=economics.get("confidence"),
    )


def recommend(profile: UserProfile, app_data: AppData) -> RecommendationResult:
    filters = {
        "include_tags": profile.include_tags,
        "exclude_tags": profile.exclude_tags,
        "startability_window_weeks": profile.startability_window_weeks,
        "max_legal_friction": profile.max_legal_friction,
    }
    recommendations: List[Recommendation] = []
    for variant in app_data.variants:
        if profile.startability_window_weeks < int(variant.get("startable_weeks", 2)):
            continue
        tags = set(variant.get("tags", []))
        if profile.include_tags and not tags.intersection(profile.include_tags):
            continue
        if profile.exclude_tags and tags.intersection(profile.exclude_tags):
            continue
        legal_gate = str(variant.get("legal_gate", "ok"))
        if not _legal_gate_allowed(profile.max_legal_friction, legal_gate):
            continue
        feasibility = evaluate_feasibility(profile, variant)
        legal = evaluate_legal(profile, variant, app_data)
        economics = evaluate_economics(variant)
        reasons_for = [
            "Clear local demand",
            "Quick setup",
            "Fits offline-first constraints",
        ]
        reasons_against = ["Requires consistent weekly effort"]
        recommendations.append(
            Recommendation(
                variant_id=str(variant["variant_id"]),
                title=str(variant["title"]),
                feasibility=feasibility,
                legal=legal,
                economics=economics,
                reasons_for=reasons_for,
                reasons_against=reasons_against,
            )
        )
    recommendations = _sort_recommendations(profile.objective_preset, recommendations)
    return RecommendationResult(
        objective=profile.objective_preset,
        filters=filters,
        recommendations=recommendations,
        applied_rules=["objective_sort"],
    )


def build_plan(profile: UserProfile, app_data: AppData, selected_variant_id: str) -> PlanResult:
    variant = next(
        (variant for variant in app_data.variants if variant["variant_id"] == selected_variant_id),
        None,
    )
    if variant is None:
        raise ValueError("Selected variant not found")
    title = str(variant["title"])
    steps = [
        PlanStep("Prepare", "Confirm assets, time slots, and quick checklist."),
        PlanStep("Launch", "Run the first small batch of tasks."),
        PlanStep("Optimize", "Review metrics and adjust workflow."),
    ]
    week_view = {
        "Week 1": ["Collect materials", "Confirm legal checklist"],
        "Week 2": ["First execution", "Gather feedback"],
        "Week 3": ["Refine pricing", "Increase outreach"],
        "Week 4": ["Stabilize routine", "Document outcomes"],
    }
    outputs = ["Checklist completed", "First revenue logged", "Weekly summary"]
    checkpoints = ["Week 1 review", "Week 2 review", "Week 4 retrospective"]
    compliance_kits = {
        "basic_safety": ["Safety checklist signed", "Emergency contacts saved"],
        "basic_finance": ["Receipt tracker updated", "Tax category verified"],
    }
    legal_checklist = list(variant.get("checklist", []))
    return PlanResult(
        variant_id=selected_variant_id,
        title=title,
        steps=steps,
        week_view=week_view,
        outputs=outputs,
        checkpoints=checkpoints,
        compliance_kits=compliance_kits,
        legal_checklist=legal_checklist,
    )


def _legal_gate_allowed(max_gate: str, gate: str) -> bool:
    order = {"ok": 0, "require_check": 1, "registration": 2, "license": 3}
    return order.get(gate, 0) <= order.get(max_gate, 0)


def _sort_recommendations(objective: str, recommendations: List[Recommendation]) -> List[Recommendation]:
    if objective == "fastest_money":
        return sorted(recommendations, key=lambda rec: rec.feasibility.prep_weeks)
    if objective == "lowest_risk":
        return sorted(recommendations, key=lambda rec: rec.legal.gate)
    if objective == "max_income":
        return sorted(recommendations, key=lambda rec: _net_value(rec), reverse=True)
    return recommendations


def _net_value(recommendation: Recommendation) -> int:
    if recommendation.economics and recommendation.economics.net_range_eur:
        return int(recommendation.economics.net_range_eur[-1])
    return 0
