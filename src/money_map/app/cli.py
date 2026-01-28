from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable, List

from money_map import core
from money_map.core.models import AppData, RecommendationResult
from money_map.core.profile_io import load_profile_yaml
from money_map.render.export import render_plan_md, render_profile_yaml, render_result_json


def _load_app_data(country: str, data_dir: str | None) -> AppData:
    path = Path(data_dir) if data_dir else None
    return core.load_app_data(country=country, data_dir=path)


def _print_recommendations(result: RecommendationResult, top: int) -> None:
    recommendations = result.recommendations[:top]
    for index, rec in enumerate(recommendations, start=1):
        print(
            f"{index}. {rec.variant_id}: {rec.title} "
            f"(legal={rec.legal.gate}, feasibility={rec.feasibility.status})"
        )


def cmd_validate(args: argparse.Namespace) -> int:
    app_data = _load_app_data(args.country, args.data_dir)
    report = core.validate_app_data(app_data)
    print(f"Validation summary: {report.summary}")
    for issue in report.issues:
        print(f"- {issue.severity} {issue.code} @ {issue.where}: {issue.message}")
    return 1 if report.has_fatal else 0


def cmd_recommend(args: argparse.Namespace) -> int:
    profile = load_profile_yaml(Path(args.profile))
    app_data = _load_app_data(args.country, args.data_dir)
    result = core.recommend(profile, app_data)
    if not result.recommendations:
        print("No recommendations found.")
        return 2
    _print_recommendations(result, args.top)
    return 0


def cmd_plan(args: argparse.Namespace) -> int:
    profile = load_profile_yaml(Path(args.profile))
    app_data = _load_app_data(args.country, args.data_dir)
    plan = core.build_plan(profile, app_data, args.variant_id)
    print(render_plan_md(plan, app_data, profile))
    return 0


def _write_exports(output_dir: Path, outputs: Iterable[tuple[str, str]]) -> List[Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    written: List[Path] = []
    for filename, content in outputs:
        file_path = output_dir / filename
        file_path.write_text(content, encoding="utf-8")
        written.append(file_path)
    return written


def cmd_export_all(args: argparse.Namespace) -> int:
    profile = load_profile_yaml(Path(args.profile))
    app_data = _load_app_data(args.country, args.data_dir)
    result = core.recommend(profile, app_data)
    if not result.recommendations:
        print("No recommendations found.")
        return 2
    variant_id = args.variant_id or result.recommendations[0].variant_id
    plan = core.build_plan(profile, app_data, variant_id)
    output_dir = Path(args.output_dir)
    written = _write_exports(
        output_dir,
        [
            ("profile.yaml", render_profile_yaml(profile)),
            ("plan.md", render_plan_md(plan, app_data, profile)),
            ("result.json", render_result_json(result, plan, profile)),
        ],
    )
    print(f"Exported {len(written)} files to {output_dir}")
    for path in written:
        print(f"- {path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="money-map")
    parser.add_argument("--country", default="DE", help="Country code")
    parser.add_argument("--data-dir", default=None, help="Optional data directory")

    subparsers = parser.add_subparsers(dest="command", required=True)

    validate_parser = subparsers.add_parser("validate", help="Validate app data")
    validate_parser.set_defaults(func=cmd_validate)

    recommend_parser = subparsers.add_parser("recommend", help="Get recommendations")
    recommend_parser.add_argument("--profile", required=True, help="Path to profile YAML")
    recommend_parser.add_argument("--top", type=int, default=10, help="Top N to show")
    recommend_parser.set_defaults(func=cmd_recommend)

    plan_parser = subparsers.add_parser("plan", help="Build a plan")
    plan_parser.add_argument("--profile", required=True, help="Path to profile YAML")
    plan_parser.add_argument("--variant-id", required=True, help="Variant id to plan")
    plan_parser.set_defaults(func=cmd_plan)

    export_parser = subparsers.add_parser("export", help="Export results")
    export_subparsers = export_parser.add_subparsers(dest="export_command", required=True)

    export_all_parser = export_subparsers.add_parser("all", help="Export all outputs")
    export_all_parser.add_argument("--profile", required=True, help="Path to profile YAML")
    export_all_parser.add_argument("--variant-id", default=None, help="Variant id override")
    export_all_parser.add_argument("--output-dir", default="exports", help="Output directory")
    export_all_parser.set_defaults(func=cmd_export_all)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    exit_code = args.func(args)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
