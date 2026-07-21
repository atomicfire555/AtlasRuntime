# Stage 23 test plan

1. Build AtlasRuntime in Release mode with the xNVSE SDK.
2. Install `AtlasRuntime.dll` under `Data/NVSE/Plugins`.
3. Remove or archive the previous `atlas_runtime.jsonl` log.
4. Load an exterior save and remain still for 30 seconds.
5. Traverse across multiple exterior cell boundaries for at least five minutes.
6. Repeat while making several rapid camera turns.
7. Verify each newly entered hitch burst produces a `hitch_context` record immediately followed by a `frame_hitch` record.
8. Confirm `sampleCount` never exceeds 120 and `capacity` is always 120.
9. Confirm loading another save resets the history rather than mixing samples across saves.
10. Compare frame behavior with Stage 22 to ensure the diagnostic observer introduces no obvious regression.
