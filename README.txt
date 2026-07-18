AtlasRuntime v0.3.8 — Stage 9 cell FormID test

Replace:
  AtlasRuntime\src\Plugin.cpp

with the included file, then run:
  Build AtlasRuntime.bat

Expected sequence after loading a save:
  game_post_load
  runtime_initialized
  player_pointer_valid
  parent_cell_valid
  player_cell_form_id

The final JSON record includes both:
  "formId": <decimal value>
  "formIdHex": "<8-digit hexadecimal value>"

Scope:
- Preserves the successful Stage 8 player and parentCell checks.
- Reads exactly one new cell member: parentCell->refID.
- Does not read the cell name, flags, worldspace, coordinates, or linked objects.
- Does not write to game memory.
- Plugin version increments from 37 to 38.

Validation:
- Generated directly from the successful Stage 8 source.
- No g_thePlayer reference.
- Player and parentCell guards precede the FormID read.
- The only parentCell member access is refID.
- Balanced braces and parentheses.
- ZIP integrity verified.
- Actual MSVC/xNVSE compilation was not possible in this Linux environment.
