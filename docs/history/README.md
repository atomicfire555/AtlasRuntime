# Project History

This directory preserves the engineering history behind AtlasRuntime and Zero Stutter.

## Structure

```text
docs/history/
├── raw/          # Unedited conversation exports and source artifacts
├── extracted/    # Curated summaries derived from raw history
├── milestones/   # Verified project checkpoints and test results
└── journal/      # Ongoing engineering notes
```

## Preservation rules

- Files under `raw/` are historical records and should not be silently rewritten.
- Curated documents must distinguish direct historical facts from later interpretation.
- Major architectural decisions should be promoted into `docs/DECISIONS.md` or a dedicated ADR.
- Current architecture documents take precedence over obsolete implementation details, but the history remains available to explain how and why the project evolved.

## Source import status

The first curated extraction is based on the original **NVR-Compatible Stutter Fix** conversation export. That transcript records the evolution from installation scanning and external telemetry into the AtlasRuntime observer/intervention vision.

The full raw export is approximately 1 MB and is intended for `docs/history/raw/ChatGPT-NVR-Compatible-Stutter-Fix.md`. It should remain unedited once imported.
