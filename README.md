# AtlasRuntime

AtlasRuntime is the in-game runtime intelligence layer for **Zero Stutter**, a long-term effort to understand, predict, reduce, and ultimately eliminate traversal and rendering stutter in Fallout: New Vegas.

> **Zero Stutter is the goal. Project Atlas is the brain.**

The project follows an evidence-first development model:

1. Observe the engine without modifying behavior.
2. Correlate gameplay, engine, system, and frametime events.
3. Classify root causes with explicit evidence and confidence.
4. Introduce narrowly bounded mitigation experiments.
5. Validate stability, compatibility, memory limits, and rollback behavior.

## Documentation

- [Vision](docs/VISION.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Design principles](docs/DESIGN_PRINCIPLES.md)
- [Roadmap](docs/ROADMAP.md)
- [Decision log](docs/DECISIONS.md)
- [Hook ownership](docs/HOOK_OWNERSHIP.md)
- [Performance model](docs/PERFORMANCE_MODEL.md)
- [Project history](docs/history/README.md)

## Project status

The repository is being initialized from the original engineering conversation and project history. Early work emphasizes trustworthy telemetry and compatibility before active intervention.
