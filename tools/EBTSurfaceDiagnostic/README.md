# EBT Surface Diagnostic

Native F4SE/CommonLibF4 diagnostic plugin targeted at Fallout 4 runtime 1.11.221.

On each TESDeathEvent (`dying=true`) it casts a 3x3 grid of downward rays around the corpse and logs:

- actor/base/cell FormIDs and EditorIDs
- ray hit/miss
- hit position and normal
- collision layer
- Havok material ID and body ID
- owning reference/base FormIDs and EditorIDs where recoverable
- hit scene-node name

The purpose is to compare Enhanced Blood Textures blood-pool receiver behavior on dirt/terrain versus concrete/static meshes.
