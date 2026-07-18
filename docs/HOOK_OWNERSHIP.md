# Hook and Responsibility Ownership

AtlasRuntime must not assume that an engine subsystem is unowned simply because it can be hooked. This document defines the compatibility review required before adding runtime behavior.

## Current ownership map

| Responsibility | Primary owner | Atlas policy |
|---|---|---|
| Script extender and plugin lifecycle | xNVSE | Use supported APIs and lifecycle contracts. Do not replace the loader. |
| Frame timing and high-FPS fixes | New Vegas Tick Fix (NVTF) | Observe timing outcomes; avoid competing timing hooks or scheduler changes without an explicit compatibility design. |
| Heap allocation replacement | New Vegas Heap Replacer (NVHR) | Measure allocation/memory behavior; do not introduce a competing allocator. |
| Rendering and shader pipeline | New Vegas Reloaded (NVR), DXVK, wrappers | Correlate render/shader events where observable; do not own Present, device, or shader hooks casually. |
| External frametime capture | PresentMon / ZeroStutter | AtlasRuntime supplies game context; ZeroStutter aligns it with external frame data. |
| MO2/profile/mod inventory | ZeroStutter desktop layer | Runtime consumes a session/config identity rather than rescanning the full installation in-process. |
| Root-cause inference | Project Atlas | Maintain evidence, confidence, and alternatives separately from raw telemetry. |
| Experimental mitigation | Atlas intervention layer | Disabled by default; isolated, bounded, reversible, and ownership-reviewed. |

## Hook review checklist

Before introducing a hook, patch, detour, or memory write, document:

1. The exact function/address/API and supported game versions.
2. Why observation through an existing API is insufficient.
3. Which known plugins may touch the same path.
4. Hook ordering and chaining behavior.
5. Thread and reentrancy assumptions.
6. Performance cost in normal and worst-case paths.
7. Failure behavior when signatures or dependencies do not match.
8. Runtime disable/rollback behavior.
9. A compatibility test matrix.
10. Evidence that the change advances the Zero Stutter mission.

## Default rule

When ownership is uncertain, Atlas remains read-only and records the missing evidence needed for a later decision.
