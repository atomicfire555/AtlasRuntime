# Decision Log

This file is the index for durable architectural decisions. New major decisions should receive a dated ADR under `docs/decisions/` and be linked here.

## D-001 — Zero Stutter is the goal; Atlas is the brain

**Status:** Accepted  
**Decision:** AtlasRuntime is a runtime intelligence and intervention framework, not merely a telemetry plugin. Telemetry and analysis are prerequisite stages toward safe stutter mitigation.

## D-002 — Evidence-first staged development

**Status:** Accepted  
**Decision:** Begin read-only, validate telemetry, build classification, then introduce narrowly scoped and reversible interventions.

## D-003 — Split in-game and external responsibilities

**Status:** Accepted  
**Decision:** AtlasRuntime owns in-process engine/gameplay context. ZeroStutter owns orchestration, system/frametime capture, persistent session analysis, visualization, and user-facing diagnostics.

## D-004 — PresentMon starts before the game

**Status:** Accepted  
**Decision:** Start PresentMon before MO2/NVSE/FalloutNV so it is ready before swap-chain creation and avoids the observed capture race.

## D-005 — Raw telemetry is immutable

**Status:** Accepted  
**Decision:** Preserve measured session facts separately from evolving classifications, scores, and recommendations.

## D-006 — Existing systems retain ownership

**Status:** Accepted  
**Decision:** Atlas must complement rather than casually replace xNVSE/NVTF/NVHR/NVR/DXVK responsibilities. New hooks or patches require an explicit ownership review.

## D-007 — Confidence and uncertainty are part of the output

**Status:** Accepted  
**Decision:** Diagnoses must expose supporting evidence, confidence, and alternatives. Mod categories are correlation candidates, not proof.

## D-008 — Active optimization is isolated and bounded

**Status:** Accepted  
**Decision:** Experimental mitigation belongs behind separate flags/branches with dry-run prediction, budgets, rollback, and measurable A/B validation.

## ADR template

```markdown
# ADR-NNN: Title

- Date:
- Status: Proposed | Accepted | Superseded | Rejected
- Context:
- Decision:
- Consequences:
- Evidence:
- Supersedes:
```
