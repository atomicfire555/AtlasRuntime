# Vision

## North star

**Zero Stutter is the goal. Project Atlas is the brain.**

AtlasRuntime is not merely a profiler, telemetry DLL, or collection of isolated fixes. It is intended to become a runtime systems framework for Fallout: New Vegas and Tale of Two Wastelands that can understand when and why traversal and rendering stalls occur, predict them when possible, and safely reduce or eliminate them.

Telemetry is the foundation, not the destination. Every observer, event marker, frame measurement, correlation rule, and diagnostic exists to build enough trustworthy knowledge for narrowly targeted intervention.

## Mission

AtlasRuntime should:

- Observe engine and gameplay state with minimal disturbance.
- Correlate runtime events with frametime, CPU, memory, rendering, loading, and mod-stack context.
- Explain individual hitches using evidence, confidence, and uncertainty rather than unsupported claims.
- Learn recurring patterns across sessions and installations.
- Progress toward prevention through bounded, reversible mitigation.
- Develop safe runtime resource-management capabilities, including adaptive heap and allocation management, when evidence shows that ownership can be assumed without destabilizing the game or conflicting with established plugins.
- Remain compatible with xNVSE, New Vegas Tick Fix, New Vegas Heap Replacer, New Vegas Reloaded, DXVK, and the wider Zero Stutter ecosystem.

## Required platform scope

- Fallout: New Vegas is the base runtime.
- Tale of Two Wastelands is a first-class requirement, not an optional compatibility target.
- Atlas must be designed and tested against large, heavily modified TTW installations rather than assuming a lightweight vanilla environment.

## Product relationship

- **AtlasRuntime** runs inside the game and supplies engine/gameplay context, prediction, and eventually bounded intervention.
- **ZeroStutter** runs outside the game and records, correlates, analyzes, visualizes, configures, and manages sessions.
- **Project Atlas** is the intelligence spanning both components.

## Long-term intervention objective

The final system is intended to be a true stutter remover, not only a diagnostic tool. Its long-term scope includes active management of allocation pressure, heap behavior, asset residency, streaming demand, and work scheduling while the game is running.

This does not mean immediately replacing NVHR or taking ownership of the heap blindly. It means building the evidence, compatibility model, safety controls, and runtime understanding required to determine when Atlas can coordinate with, extend, or eventually provide an alternative memory-management layer. Any such capability must be measurable, opt-in during development, bounded by strict budgets, reversible, and automatically disabled when ownership or compatibility is uncertain.

## Distribution intent

The project will remain private during early research and development. The intended destination is an open-source release once the architecture, safety model, licensing, documentation, and reproducible build process are mature enough for public collaboration.

## Success

The long-term success criterion is not a higher average FPS number. It is the ability to answer and eventually act on questions such as:

> Why did this specific frame take 71 ms, what engine work caused it, could it have been predicted, and can Atlas prevent the same stall without destabilizing the game?

A mature Atlas should also be able to determine whether a hitch was driven by allocation pressure, heap contention or fragmentation, asset streaming, shader work, scripts, I/O, or another subsystem—and then apply only the intervention justified by the evidence.

## Non-goals

AtlasRuntime must not become:

- A collection of aggressive patches enabled without evidence.
- A tool that replaces established systems merely for the sake of ownership.
- A tool that hides uncertainty or presents correlation as proof.
- A system that trades stutter for crashes, runaway memory growth, save corruption, or mod incompatibility.
- A system whose Fallout: New Vegas support works only when TTW is absent.
