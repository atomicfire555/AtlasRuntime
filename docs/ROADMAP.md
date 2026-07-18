# Roadmap

This roadmap records direction, not fixed release promises. Advancement is gated by measured stability and evidence quality.

## Phase 1 — Runtime telemetry

- Stable xNVSE plugin lifecycle.
- Session identifiers and schema versions.
- Player, cell, worldspace, movement, and transition events.
- High-resolution hitch markers.
- Low-overhead, failure-tolerant event transport.

**Exit criterion:** telemetry is complete enough to reproduce and align a session timeline without materially affecting frametime.

## Phase 2 — Camera and traversal context

- Camera state and movement classification.
- Exterior/interior and transition context.
- Loading, fast travel, menu, startup, and shutdown boundaries.
- Resource growth and recovery envelopes.

**Exit criterion:** Atlas can distinguish gameplay traversal from unrelated startup, menu, loading, and exit outliers.

## Phase 3 — Hitch classification

- Hitch clustering and impact scoring.
- Streaming/allocation, rendering/shader, CPU/script, loading, and unknown categories.
- Evidence and contradiction accounting.
- Confidence calibration against repeated sessions.
- Mod-aware correlation without claiming proof.

**Exit criterion:** classifications are useful, inspectable, and demonstrably better than generic frametime labels.

## Phase 4 — Session intelligence

- SQLite-backed session history.
- Baselines and before/after comparisons.
- Regression detection after mod or configuration changes.
- Searchable timeline and root-cause reports.
- Repeated-pattern learning per installation/profile.

**Exit criterion:** Atlas reliably explains what changed between sessions and preserves raw evidence.

## Phase 5 — Controlled mitigation experiments

- Dry-run prediction mode.
- Bounded prefetch experiments.
- Work-spreading and deferral experiments where engine ownership is clear.
- Strict memory budgets, timeouts, kill switches, and rollback.
- A/B session validation.

**Exit criterion:** at least one mitigation reduces a repeatable hitch pattern without measurable stability, compatibility, or memory regressions.

## Phase 6 — Compatibility validation

- xNVSE, NVTF, NVHR, NVR, DXVK, ReShade/ENB, and common plugin-stack matrices.
- Hook and responsibility ownership checks.
- Conflict detection and automatic safe disablement.
- Large-modlist stress testing.

## Phase 7 — User configuration and release hardening

- Safe defaults and transparent profiles.
- Diagnostics and share packages.
- Schema migration and backward compatibility.
- Crash-safe shutdown and incomplete-session recovery.
- Documentation, reproducible builds, tests, and release packaging.

## Deferred until justified

- Broad engine patches.
- Replacement timing or heap systems.
- Unbounded asset caching.
- Automatic interventions without dry-run evidence.
- Features that duplicate established plugin ownership.
