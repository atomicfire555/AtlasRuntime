# Vision

## North star

**Zero Stutter is the goal. Project Atlas is the brain.**

AtlasRuntime is not merely a profiler, telemetry DLL, or collection of isolated fixes. It is intended to become a runtime systems framework for Fallout: New Vegas that can understand when and why traversal and rendering stalls occur, predict them when possible, and safely reduce or eliminate them.

Telemetry is the foundation, not the destination. Every observer, event marker, frame measurement, correlation rule, and diagnostic exists to build enough trustworthy knowledge for narrowly targeted intervention.

## Mission

AtlasRuntime should:

- Observe engine and gameplay state with minimal disturbance.
- Correlate runtime events with frametime, CPU, memory, rendering, loading, and mod-stack context.
- Explain individual hitches using evidence, confidence, and uncertainty rather than unsupported claims.
- Learn recurring patterns across sessions and installations.
- Progress toward prevention through bounded, reversible mitigation.
- Remain compatible with xNVSE, New Vegas Tick Fix, New Vegas Heap Replacer, New Vegas Reloaded, DXVK, and the wider Zero Stutter ecosystem.

## Product relationship

- **AtlasRuntime** runs inside the game and supplies engine/gameplay context.
- **ZeroStutter** runs outside the game and records, correlates, analyzes, visualizes, and manages sessions.
- **Project Atlas** is the intelligence spanning both components.

## Success

The long-term success criterion is not a higher average FPS number. It is the ability to answer and eventually act on questions such as:

> Why did this specific frame take 71 ms, what engine work caused it, could it have been predicted, and can Atlas prevent the same stall without destabilizing the game?

## Non-goals

AtlasRuntime must not become:

- A collection of aggressive patches enabled without evidence.
- A replacement for systems already safely owned by established plugins.
- A tool that hides uncertainty or presents correlation as proof.
- A system that trades stutter for crashes, runaway memory growth, save corruption, or mod incompatibility.
