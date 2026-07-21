# Stage 23 — Pre-hitch context window

Stage 23 adds a fixed-capacity, allocation-free rolling frame history to AtlasRuntime.

When a new frame hitch begins, AtlasRuntime emits a `hitch_context` JSONL record immediately before the existing `frame_hitch` record. The context summary covers up to 120 preceding frames and includes:

- sample count and observed window duration
- average and maximum frame time
- average and maximum player speed
- maximum acceleration magnitude
- camera sample count and net camera-turn angle
- cell changes observed inside the window

The ring buffer resets with the existing frame-timer reset path during save loading, new games, exit to the main menu, and invalid performance-counter state.

This stage remains diagnostic-only and does not modify engine behavior, memory allocation, streaming, or rendering.
