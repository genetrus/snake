from __future__ import annotations

from dataclasses import dataclass, field, asdict
from typing import Dict, List, Optional, Sequence


@dataclass
class AppMeta:
    dataset_version: str
    warnings: List[str] = field(default_factory=list)


@dataclass
class RulepackMeta:
    reviewed_at: str


@dataclass
class AppData:
    meta: AppMeta
    rulepack: RulepackMeta
    variants: List[Dict[str, object]]


@dataclass
class ValidationIssue:
    severity: str
    code: str
    where: str
    message: str


@dataclass
class ValidationReport:
    summary: str
    issues: List[ValidationIssue]

    @property
    def has_fatal(self) -> bool:
        return any(issue.severity == "FATAL" for issue in self.issues)

    @property
    def has_stale(self) -> bool:
        return any(issue.severity == "WARN" and issue.code == "STALE" for issue in self.issues)


@dataclass
class UserProfile:
    hours_per_week: int
    capital_eur: int
    language_cefr: str
    assets: List[str]
    objective_preset: str
    startability_window_weeks: int = 4
    include_tags: List[str] = field(default_factory=list)
    exclude_tags: List[str] = field(default_factory=list)
    max_legal_friction: str = "ok"

    def to_dict(self) -> Dict[str, object]:
        return asdict(self)


@dataclass
class Feasibility:
    status: str
    blockers: List[str]
    prep_weeks: int


@dataclass
class Legal:
    gate: str
    checklist: List[str]
    compliance_kits: List[str]
    stale_warning: Optional[str] = None


@dataclass
class Economics:
    time_to_first_money_range_weeks: Optional[Sequence[int]] = None
    net_range_eur: Optional[Sequence[int]] = None
    confidence: Optional[str] = None


@dataclass
class Recommendation:
    variant_id: str
    title: str
    feasibility: Feasibility
    legal: Legal
    economics: Optional[Economics]
    reasons_for: List[str]
    reasons_against: List[str]


@dataclass
class RecommendationResult:
    objective: str
    filters: Dict[str, object]
    recommendations: List[Recommendation]
    applied_rules: List[str] = field(default_factory=list)


@dataclass
class PlanStep:
    title: str
    details: str


@dataclass
class PlanResult:
    variant_id: str
    title: str
    steps: List[PlanStep]
    week_view: Dict[str, List[str]]
    outputs: List[str]
    checkpoints: List[str]
    compliance_kits: Dict[str, List[str]]
    legal_checklist: List[str]
