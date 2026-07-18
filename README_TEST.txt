AtlasRuntime v0.3.6 Stage 7

1. Edit the paths at the top of Build AtlasRuntime.bat if needed.
2. Run Build AtlasRuntime.bat. It deletes the old build folder, builds Release Win32, and installs the DLL.
3. Delete the prior atlas_runtime.jsonl log.
4. Start Fallout: New Vegas through xNVSE and load a save.
5. Exit the game and inspect/upload the new log.

Expected after runtime_initialized:
- player_pointer_valid
or
- player_pointer_null

Stage 7 does not dereference the player object.
