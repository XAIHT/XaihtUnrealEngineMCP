# Unreal MCP — Post-P0/P1 Comparison & New-Operations Proposal

> Prepared 2026-05-24, **after** the P0 fixes and P1 additions in this branch.
> Supersedes the comparison in `IMPROVEMENT_PROPOSAL.md` (which reflected the
> pre-change state). Competitor data is from the same-day GitHub research.

---

## 1. What changed in this repo's competitive position

P0 repaired the broken surface; **P1 added a `System` command category** —
`execute_python`, `execute_console_command`, `get_class_info`, `list_assets`, and the
`call_unreal` escape hatch. The tool count went from **~26 usable → ~38 usable**, but
the *strategic* change is bigger than the count:

- **`execute_python` is a universal escape hatch.** Anything in the Unreal Editor
  Python API is now reachable — materials, Niagara, Sequencer, landscape, level I/O,
  etc. So the repo flipped from "❌ can't do it" to "🐍 reachable, just not ergonomic"
  on almost every previously-missing row.
- **Three capability rows flipped to first-class ✅:** console/CVar execution, Python
  passthrough, and reflection/introspection.
- The remaining gap vs. the leaders is now about **ergonomics, reliability, and
  discoverability** (dedicated, validated, structured-output tools the model can find),
  **not raw reach**.

---

## 2. Updated feature comparison matrix

Legend: ✅ dedicated tool · 🟡 partial/limited dedicated · 🐍 reachable only via the
`execute_python` escape hatch · ❌ none

| Capability | **This repo (now)** | ChiR24 | db-lyon | remiphilippe | Flux-Point |
|---|:--:|:--:|:--:|:--:|:--:|
| Actor spawn/transform/props | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint create/compile | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint node-graph editing | ✅ | ✅ | ✅ | ✅ | ✅ |
| UMG widgets | 🟡 (2 types) | ✅ | ✅ | 🟡 | ✅ |
| Input mappings | 🟡 (legacy only) | ✅ | ✅ | ✅ (Enhanced) | ✅ (Enhanced) |
| Screenshot / viewport | ✅ *(P0)* | ✅ | ✅ | ✅ | ✅ |
| **Console / CVar exec** | ✅ *(P1)* | ✅ | ✅ | ✅ | ✅ |
| **Python passthrough** | ✅ *(P1)* | ✅ | ✅ | ✅ | ✅ |
| **Reflection / introspection** | ✅ *(P1)* | ✅ | ✅ | ✅ | ✅ |
| Asset listing/search | ✅ *(P1)* | ✅ | ✅ | ✅ | ✅ |
| **Asset import (FBX/tex/CSV)** | 🐍 | ✅ | ✅ | ✅ | 🟡 |
| **Materials** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **Level / World mgmt** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **PIE control** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **Niagara / VFX** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **Audio** | 🐍 | ✅ | ✅ | ❌ | ✅ |
| **Sequencer / cinematics** | 🐍 | ✅ | ✅ | ❌ | ✅ |
| **Landscape / Foliage** | 🐍 | ✅ | ✅ | 🟡 | ✅ |
| **Data tables** | 🐍 | ✅ | ✅ | ✅ | 🟡 |
| **Headless build/cook/test** | ❌ | 🟡 | 🟡 | ✅ | 🟡 |
| **Workflow orchestration** | ❌ | ❌ | ✅ (YAML) | ❌ | ❌ |
| Transport robustness | 🟡 *(P0 hardened)* | ✅ | ✅ | ✅ | ✅ |
| Dispatch architecture | 🟡 (if/else) | ✅ | ✅ | ✅ | ✅ |

| Repo | Lang | ~Usable tools | Stars | Position after our changes |
|------|------|--------------|-------|----------------------------|
| **This repo (now)** | C++/Python | **~38** | (fork) | Reach parity via Python; trailing on *dedicated* content tools |
| ChiR24/Unreal_mcp | C++/TS | 22 (bundled) | ~645 | Broad bundled tools + AI gameplay systems |
| db-lyon/ue-mcp | C++/TS | 525+ actions | ~111 | Widest surface + YAML workflows |
| remiphilippe/mcp-unreal | Go/C++ | 49 | ~31 | Headless CI/CD + doc index |
| Flux-Point-Studios/unreal-mcp | C++/TS | 36 | ~5 | Finest-grained dedicated tools |

**Takeaway:** the only rows where this repo is genuinely *incapable* are now
**headless build/cook/test** and **workflow orchestration**. Everywhere else it can
act through `execute_python`; the work ahead is promoting the highest-traffic 🐍 rows
to ergonomic ✅ tools.

---

## 3. Design principle for new operations (post-`execute_python`)

Because the escape hatch exists, a **dedicated tool now has to earn its place**. Add one
when it provides at least one of:

1. **Reliability** — validated inputs, fewer ways for the model to write broken Python.
2. **Discoverability** — shows up in the tool list so the model reaches for it instead of
   improvising a script.
3. **Structured output** — returns typed JSON (ids, paths, counts) the model can chain,
   rather than parsing log text.
4. **High frequency** — used in almost every session (level I/O, materials, PIE).

Long-tail, one-off operations should stay on `execute_python`.

---

## 4. Proposed new operations (prioritized)

Each is a new handler method following the `FUnrealMCPSystemCommands` pattern, plus a
Python wrapper. Grouped into a few new handler classes.

### Tier 1 — Foundational, used in nearly every session
**`FUnrealMCPLevelCommands`**
- `open_level(path)` — open/load a map (the AI currently can't change which level it edits)
- `save_current_level()` / `save_all()`
- `new_level(path, template)` 
- `get_current_level()` — returns level name/path + actor count

**`FUnrealMCPAssetCommands`** (extends the P1 `list_assets`)
- `import_asset(source_file, destination_path, options)` — FBX / textures / audio
- `duplicate_asset(source, destination)` · `rename_asset(path, new_name)` · `delete_asset(path)`
- `save_asset(path)` · `create_folder(path)`

**`FUnrealMCPMaterialCommands`**
- `create_material(path, name)`
- `create_material_instance(parent_material, path, name)`
- `set_material_parameter(material, name, value, type=scalar|vector|texture)`
- `assign_material(actor_or_component, material, slot=0)`

### Tier 2 — Closes the iteration / gameplay loop
**`FUnrealMCPRuntimeCommands`**
- `start_pie()` · `stop_pie()` · `is_in_pie()` — run the game and observe (pairs with `take_screenshot`)
- `set_simulate_physics_in_level(actor, bool)`

**Enhanced Input** (replace/augment legacy `create_input_mapping`)
- `create_input_action(name, value_type)`
- `create_input_mapping_context(name)`
- `add_key_mapping(context, action, key, modifiers)`

**Actor/component depth**
- `add_component_to_actor(actor, component_type, name)` — for level actors, not just BPs
- `list_actor_components(actor)` · `get_component_property(actor, component, property)`

### Tier 3 — Rich content authoring
**`FUnrealMCPVfxAudioCommands`**
- `spawn_niagara_at_location(system_path, location)` · `attach_niagara_to_actor(...)` · `set_niagara_user_parameter(...)`
- `play_sound_2d(sound_path)` · `play_sound_at_location(sound_path, location)` · `spawn_ambient_sound(...)`

**Lighting helpers**
- `set_light_properties(actor, intensity, color, attenuation_radius)`
- `build_lighting(quality)`

**`FUnrealMCPSequencerCommands`**
- `create_level_sequence(path, name)`
- `add_actor_to_sequence(sequence, actor)`
- `add_transform_keyframe(sequence, actor, time, transform)`
- `set_playback_range(sequence, start, end)` · `play_sequence(sequence)`

### Tier 4 — Differentiators / longer horizon
- **Landscape / Foliage:** `create_landscape(size, sections)`, `add_foliage_type(mesh)`, `paint_foliage(area, density)`
- **Data tables:** `create_datatable(row_struct, path)`, `import_csv_to_datatable(csv, datatable)`, `get_datatable_rows(datatable)`
- **Diagnostics:** `get_output_log(filter, max_lines)`, `get_blueprint_compile_errors(blueprint)`
- **Headless mode** (separate process, *not* the editor bridge): `build_project`, `cook_project`, `run_automation_tests` via `UnrealEditor-Cmd`/RunUAT — the one true capability gap vs. remiphilippe.
- **Workflow orchestration:** a `run_macro(steps[])` that executes a validated multi-step
  sequence with rollback — matches db-lyon's YAML flow engine, the other true gap.

---

## 5. Suggested sequencing
1. **Tier 1** (Level + Asset + Material) — biggest day-to-day ergonomics win; unblocks
   scene authoring end-to-end.
2. **Adopt the `TMap` command registry** (deferred P1 item) *before* Tier 3, so adding
   the long tail of content tools stops requiring bridge edits.
3. **Tier 2** (PIE + Enhanced Input) — closes the test loop.
4. **Tiers 3–4** as demand dictates; keep using `execute_python` for the long tail.

---

## 6. Sources
- ChiR24/Unreal_mcp — https://github.com/ChiR24/Unreal_mcp
- db-lyon/ue-mcp — https://github.com/db-lyon/ue-mcp
- remiphilippe/mcp-unreal — https://github.com/remiphilippe/mcp-unreal
- Flux-Point-Studios/unreal-mcp — https://github.com/Flux-Point-Studios/unreal-mcp
- ayeletstudioindia/unreal-analyzer-mcp — https://github.com/ayeletstudioindia/unreal-analyzer-mcp
- chongdashu/unreal-mcp (origin) — https://github.com/chongdashu/unreal-mcp
