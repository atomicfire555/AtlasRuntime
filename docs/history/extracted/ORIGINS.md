# Origins of Zero Stutter and Project Atlas

## Initial problem

The project began from a heavily modded Fallout: New Vegas / Tale of Two Wastelands installation using NVR, NVTF, NVHR, large worldspace expansions, extensive LOD and textures, and hundreds of mods. Existing configuration tuning was already strong, so the remaining traversal and rendering hitches could not be solved honestly by another generic INI preset.

## Original four-part concept

The earliest Zero Stutter concept combined four systems:

1. **Smart compatibility manager** — detect the installed runtime stack and decide which component owns each responsibility.
2. **Hitch analyzer** — explain why a specific frame stalled instead of reporting only its duration.
3. **Runtime optimizer** — eventually predict and spread or pre-stage costly work across frames.
4. **Automatic profiles** — derive installation-aware guidance from MO2, mods, plugins, DLLs, rendering, and hardware context.

## Trust before optimization

The first deliverable deliberately avoided modifying the game. It focused on installation detection, MO2 profile discovery, DLL and plugin inventory, compatibility reporting, exportable diagnostics, and a health score. The engineering principle was simple: users must trust the scanner and evidence before they can trust an optimizer.

## External telemetry becomes Atlas

The desktop pipeline evolved to launch PresentMon before MO2/NVSE, record CPU and memory, store session snapshots, calculate gameplay-filtered metrics, cluster hitches, and produce mod-aware correlation candidates. The desired result changed from a generic performance summary into an explanation of a specific frame and its surrounding event window.

## AtlasRuntime emerges

The missing information was in-game context: player movement, camera state, cells, worldspaces, transitions, and engine lifecycle events. AtlasRuntime became the xNVSE runtime layer supplying that context to the external ZeroStutter analyzer.

The intended relationship is:

```text
AtlasRuntime: in-game observer and future bounded intervention layer
ZeroStutter: external capture, persistence, analysis, visualization, and control
Project Atlas: the reasoning system joining both sides
```

## Foundational identity

The project constitution crystallized as:

> **Zero Stutter is the goal. Project Atlas is the brain.**

AtlasRuntime was never intended to stop at telemetry. Observation is the safe first stage of a runtime observer/intervention system whose ultimate purpose is to understand, predict, reduce, and eventually eliminate traversal and rendering stutter without destabilizing the game or conflicting with the established mod stack.

## Development pattern established by the history

- Establish a verified baseline.
- Add one observable capability at a time.
- Validate against real sessions and a demanding mod stack.
- Correct misleading boundaries and conclusions before adding features.
- Preserve uncertainty and avoid claiming causation from correlation.
- Keep active mitigation deferred until the external analyzer and runtime evidence are reliable.
