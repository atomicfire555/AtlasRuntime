# Performance Model

Atlas evaluates smoothness as a timeline of work, stalls, and recovery—not as a single average FPS value.

## Primary measurements

- Frame time distribution.
- 1% and 0.1% lows using gameplay-filtered data.
- Hitch counts above configurable thresholds such as 33.3 ms, 50 ms, and 100 ms.
- Worst gameplay frame after startup/shutdown/loading filtering.
- Hitch cluster duration, density, peak, and cumulative impact.
- CPU utilization and saturation context.
- Working-set and private-memory change.
- Resource growth, stabilization, reclamation, and recovery time.
- Player movement, camera, cell/worldspace, and transition context.

## Event model

A hitch is not automatically a cause. It is an observation that anchors a surrounding evidence window.

```text
pre-event baseline
        |
        v
trigger / transition / unknown event
        |
        v
frametime disturbance and resource response
        |
        v
stabilization, recovery, or session termination
```

## Resource envelopes

Atlas may expand an event into a resource envelope when evidence shows sustained growth or delayed recovery. Envelope boundaries must not absorb unrelated startup work, stable samples, or later events without evidence.

Recovery states should distinguish:

- Recovered or partially reclaimed.
- Stabilized at a higher resident-cache level.
- Session ended before recovery could be determined.
- No meaningful reclamation observed.
- Inconclusive recovery.

## Classification inputs

Candidate classes include:

- Asset streaming or allocation spike.
- Rendering, shader compilation, or presentation stall.
- CPU/script/AI pressure.
- Loading or transition behavior.
- Background task contention.
- External capture or instrumentation artifact.
- Unknown.

A classification must include supporting evidence, contradicting evidence, confidence, and the data quality of the event window.

## Filtering policy

Startup, shutdown, menus, loading screens, dropped frames, and extreme capture artifacts must be labeled rather than silently mixed into gameplay metrics. Filters and thresholds belong in the session metadata so results remain reproducible.

## Optimization criterion

A mitigation is successful only when repeated A/B sessions show a meaningful reduction in hitch impact or recovery time without regressions in crashes, memory, compatibility, visual correctness, input behavior, or save integrity.
