# Demo checklist (10 minutes)

> Goal: validate offline-first demo flow and smoke checks without any network access.

## 0) Pre-flight
- Ensure you are working offline (no external network calls).
- Have the demo profiles available in `profiles/`.

## 1) Validate data
```bash
money-map validate
```
- Expect: `OK` summary.
- If you see `FATAL`, stop and fix before continuing.

## 2) Recommend (demo_fast_start)
```bash
money-map recommend --profile profiles/demo_fast_start.yaml --top 10
```
- Ensure you get a top-10 list (or fewer if the dataset is small).
- Note the **top-1** `variant_id` for the next step.

## 3) Build plan for top-1
```bash
money-map plan --profile profiles/demo_fast_start.yaml --variant-id <top1>
```
- Confirm the plan includes 4-week actions and compliance items.

## 4) Export all
```bash
money-map export all --profile profiles/demo_fast_start.yaml
```
- Verify the export directory contains:
  - Combined JSON summary.
  - Plan markdown.
  - Profile YAML.

## 5) UI flow (local)
- Open the UI and complete the profile.
- Navigate **Recommendations → Plan → Export**.
- Download the files.

## 6) Final demo checks
- **Disclaimer present**: no legal verdicts or income guarantees.
- **Dataset version** is visible (e.g., in sidebar).
- **Warnings** show if the dataset is stale.

---

## Smoke / perf commands (manual)
```bash
pytest -q
money-map validate
money-map recommend --profile profiles/demo_fast_start.yaml --top 10
money-map plan --profile profiles/demo_fast_start.yaml --variant-id <top1>
money-map export all --profile profiles/demo_fast_start.yaml
MM_RUN_PERF=1 pytest -q -m perf
```
