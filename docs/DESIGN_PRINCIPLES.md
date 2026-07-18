# Design Principles

## 1. Observe before acting

Atlas must establish trustworthy measurements and engine context before it changes runtime behavior.

## 2. Evidence over certainty theater

Every diagnosis should expose evidence, confidence, assumptions, and plausible alternatives. Correlation candidates must never be presented as proven causes.

## 3. Compatibility through ownership

Before adding a hook, patch, allocator, timing change, renderer intervention, or scheduler behavior, determine which existing component already owns that responsibility.

## 4. Safety is a feature

Every intervention needs strict bounds, validation, rollback, kill switches, and failure behavior that returns the game to its unmodified path.

## 5. Separate observation from intervention

Read-only telemetry and experimental mitigation must be independently buildable, testable, and disableable.

## 6. Preserve raw facts

Raw telemetry should remain immutable. Derived clusters, labels, confidence scores, and recommendations may evolve without rewriting the original session record.

## 7. Optimize frametime consistency

Average FPS is secondary. Atlas prioritizes hitch frequency, severity, duration, recovery, 1%/0.1% lows, and the shape of the frametime distribution.

## 8. Validate on hostile real-world stacks

A large TTW/NVR/NVTF/MO2 installation is a primary proving ground. A feature is not considered robust merely because it works on a minimal test profile.

## 9. Prefer staged progress

Each milestone should add one understandable capability with a verified baseline. Broad speculative rewrites are avoided.

## 10. Keep the project explainable

Architecture, decisions, data contracts, experiments, and failures belong in the repository so future development does not depend on chat memory.
