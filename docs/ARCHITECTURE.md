# Architecture

## System overview

```text
Fallout: New Vegas
        |
        v
AtlasRuntime (xNVSE runtime layer)
        |
        | structured events / telemetry
        v
ZeroStutter desktop pipeline
        |
        +-- PresentMon frametime capture
        +-- System telemetry recorder
        +-- MO2 and mod-stack inventory
        +-- Session database
        +-- Timeline and hitch clustering
        +-- Atlas reasoning engine
        +-- Reports, comparisons, and recommendations
```

## Runtime layer

AtlasRuntime operates inside the game process. Its initial responsibility is observation, not intervention.

Expected telemetry domains include:

- Session and game lifecycle.
- Player position, speed, direction, cell, and worldspace.
- Cell and worldspace transitions.
- Frame hitch events and high-resolution timing.
- Camera and traversal state.
- Loading and recovery markers.
- Carefully selected engine events whose ownership and safety are understood.

## Desktop layer

ZeroStutter operates outside the game process and owns orchestration and analysis:

- Starts telemetry capture before launching MO2/NVSE to avoid attachment races.
- Records PresentMon, CPU, memory, and runtime events.
- Captures MO2 profile, plugin, mod, and DLL inventories.
- Stores sessions for comparison.
- Builds hitch clusters and resource envelopes.
- Produces evidence-based classifications and confidence scores.
- Presents timelines, graphs, summaries, and regression analysis.

## Intelligence layer

Project Atlas correlates the runtime and desktop data. Its reasoning should remain inspectable:

1. Identify the event window.
2. Collect supporting and contradicting evidence.
3. Compare against known patterns.
4. Produce a classification and calibrated confidence.
5. Preserve uncertainty and alternative explanations.
6. Learn from repeated sessions without silently rewriting ground truth.

## Intervention layer

Active mitigation is a later-stage capability and must remain separate from observation. Candidate experiments include bounded prefetching, work spreading, and postponement of noncritical background work. Every experiment requires:

- Dry-run prediction before activation.
- Explicit memory and time budgets.
- A narrow activation condition.
- Runtime kill switches and rollback.
- Compatibility ownership checks.
- Before/after validation across multiple sessions.

## Data contract

Runtime events should be versioned, append-friendly, timestamped from a stable clock, and resilient to partial sessions. The contract should distinguish measured facts, inferred labels, and user/configuration metadata.
