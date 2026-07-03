# Unreal MCP — Post-P0/P1/P2/P3 Comparison & Next-Operations Proposal

> Prepared 2026-05-24, **after** the P0 fixes, the P1 System category, the P2 Tier-1
> content categories (Level, Asset, Material), and the P3 automation tools.
> Supersedes the tables in `IMPROVEMENT_PROPOSAL.md` and `NEW_OPERATIONS_PROPOSAL.md`.
> Competitor data is from the same-day GitHub research.
>
> **⚠️ Snapshot — partially superseded (July 2026).** Since this was written, the
> v2.0 improvements landed (see `IMPROVEMENTS_SUMMARY.md`): the if/else dispatch
> was replaced by a **self-registering command registry** (§3's "TMap command
> registry" item is DONE), `get_supported_commands` provides runtime discovery,
> and 7 new commands were added (`execute_python_file`, `save_blueprint`,
> `is_blueprint_dirty`, `set_material_color`, `get_material_info`,
> `assign_material_to_all_slots`, `get_supported_commands`). Current surface:
> **63 MCP tools / 61 bridge commands**. The tables below reflect the 2026-05-24
> state and are kept for history.

---

## 1. What changed this round

P2 promoted three big **🐍-via-Python** rows to first-class dedicated **✅** tools, and
P3 addressed the **two capability rows this repo previously could not do at all**:

| Row | Was (after P1) | Now (after P2/P3) | Added |
|---|:--:|:--:|---|
| Materials | 🐍 | ✅ | `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material` |
| Level / World mgmt | 🐍 | ✅ | `open_level`, `save_current_level`, `save_all`, `new_level`, `get_current_level` |
| Asset import / mgmt | 🐍 | ✅ | `import_asset`, `duplicate/rename/delete/save_asset`, `create_folder` |
| Headless build/test | ❌ | 🟡 | `build_project`, `run_automation_tests` (cook still TODO) |
| Workflow orchestration | ❌ | 🟡 | `run_macro` (stop-on-error; rollback still TODO) |

Tool count: **~38 → ~56 usable MCP tools** across **10 categories**.

**The strategic result: there are no longer any hard ❌ capability rows.** Every row is
now at least 🟡 (dedicated-but-partial) or 🐍 (reachable via `execute_python`). The
remaining gaps are **ergonomics** (promote the 🐍 rows to ✅) and **architecture**
(dispatch + transport), not raw ability.

---

## 2. Updated feature comparison matrix

Legend: ✅ dedicated tool · 🟡 partial/limited dedicated · 🐍 reachable only via the
`execute_python` escape hatch · ❌ none

| Capability | **This repo (now)** | ChiR24 | db-lyon | remiphilippe | Flux-Point |
|---|:--:|:--:|:--:|:--:|:--:|
| Actor spawn/transform/props | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint create/compile | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint node-graph editing | ✅ | ✅ | ✅ | ✅ | ✅ |
| Screenshot / viewport | ✅ | ✅ | ✅ | ✅ | ✅ |
| Console / CVar exec | ✅ | ✅ | ✅ | ✅ | ✅ |
| Python passthrough | ✅ | ✅ | ✅ | ✅ | ✅ |
| Reflection / introspection | ✅ | ✅ | ✅ | ✅ | ✅ |
| Asset listing/search | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Asset import (FBX/tex/CSV)** | ✅ *(P2)* | ✅ | ✅ | ✅ | 🟡 |
| **Materials** | ✅ *(P2)* | ✅ | ✅ | ✅ | ✅ |
| **Level / World mgmt** | ✅ *(P2)* | ✅ | ✅ | ✅ | ✅ |
| **Workflow orchestration** | 🟡 *(P3, no rollback)* | ❌ | ✅ (YAML) | ❌ | ❌ |
| **Headless build/cook/test** | 🟡 *(P3, no cook)* | 🟡 | 🟡 | ✅ | 🟡 |
| UMG widgets | 🟡 (2 types) | ✅ | ✅ | 🟡 | ✅ |
| Input mappings | 🟡 (legacy only) | ✅ | ✅ | ✅ (Enhanced) | ✅ (Enhanced) |
| **PIE control** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **Niagara / VFX** | 🐍 | ✅ | ✅ | ✅ | ✅ |
| **Audio** | 🐍 | ✅ | ✅ | ❌ | ✅ |
| **Sequencer / cinematics** | 🐍 | ✅ | ✅ | ❌ | ✅ |
| **Landscape / Foliage** | 🐍 | ✅ | ✅ | 🟡 | ✅ |
| **Data tables** | 🐍 | ✅ | ✅ | ✅ | 🟡 |
| Transport robustness | 🟡 (per-cmd TCP, hardened) | ✅ | ✅ | ✅ | ✅ |
| Dispatch architecture | 🟡 (if/else, 10 categories) | ✅ | ✅ | ✅ | ✅ |

| Repo | Lang | ~Usable tools | Stars | Position now |
|------|------|--------------|-------|--------------|
| **This repo (now)** | C++/Python | **~56** | (fork) | Capability parity; trailing only on ergonomics (🐍 rows) + architecture |
| ChiR24/Unreal_mcp | C++/TS | 22 bundled | ~645 | Broad bundled tools + AI gameplay systems |
| db-lyon/ue-mcp | C++/TS | 525+ actions | ~111 | Widest surface + YAML workflows w/ rollback |
| remiphilippe/mcp-unreal | Go/C++ | 49 | ~31 | Full headless CI/CD (build+cook+test) + doc index |
| Flux-Point-Studios/unreal-mcp | C++/TS | 36 | ~5 | Finest-grained dedicated tools |

**Takeaway:** on raw capability the repo is now competitive with the leaders. The
specific places it is still *behind* are: full headless **cook**, orchestration
**rollback/retry**, **Enhanced Input**, and the long tail of content categories that are
🐍-only (PIE, Niagara, Audio, Sequencer, Landscape, DataTables). And it trails *all* of
them on **internal architecture** (a 10-branch if/else dispatch and a per-command TCP
transport).

---

## 3. Proposal — next operations to add (prioritized)

Design rule unchanged: a **dedicated tool earns its place** over `execute_python` only if
it adds reliability, discoverability, structured output, or high frequency.

### Tier 2 — closes the interactive observe→act loop (highest remaining value)
**`FUnrealMCPRuntimeCommands`**
- `start_pie()` · `stop_pie()` · `is_in_pie()` · `pie_step(frames)` — run the game and,
  paired with `take_screenshot`, let the AI *observe the result of its changes*.

**Enhanced Input** (augment the legacy-only `create_input_mapping`)
- `create_input_action(name, value_type)`
- `create_input_mapping_context(name)`
- `add_key_mapping(context, action, key, modifiers)`

**Level-actor depth** (today component ops only work on Blueprints)
- `add_component_to_actor(actor, component_type, name)`
- `list_actor_components(actor)` · `get_component_property(actor, component, property)`

### Tier 3 — content authoring (promote the 🐍 rows)
**`FUnrealMCPVfxAudioCommands`**
- `spawn_niagara_at_location(system, location)` · `attach_niagara_to_actor(...)` · `set_niagara_user_parameter(...)`
- `play_sound_2d(sound)` · `play_sound_at_location(sound, location)` · `spawn_ambient_sound(...)`

**Lighting helpers**
- `set_light_properties(actor, intensity, color, attenuation_radius)` · `build_lighting(quality)`

**`FUnrealMCPSequencerCommands`**
- `create_level_sequence(path, name)` · `add_actor_to_sequence(seq, actor)`
- `add_transform_keyframe(seq, actor, time, transform)` · `set_playback_range(seq, start, end)` · `play_sequence(seq)`

### Tier 4 — completeness
- **Landscape / Foliage:** `create_landscape(size, sections)`, `add_foliage_type(mesh)`, `paint_foliage(area, density)`
- **Data tables:** `create_datatable(row_struct, path)`, `import_csv_to_datatable(csv, datatable)`, `get_datatable_rows(datatable)`
- **Diagnostics:** `get_output_log(filter, max_lines)`, `get_blueprint_compile_errors(blueprint)`
- **`cook_project(platform)`** — completes the headless trio vs. remiphilippe.

### Architecture — now the highest-leverage work (10 categories and growing)
1. **`TMap` command registry** (the deferred refactor). With 10 handler categories the
   bridge if/else is now the main desync risk; a registry makes new commands self-register.
2. **`run_macro` rollback + retry** — record an inverse per step (or snapshot/restore via
   the transaction system) to reach full parity with db-lyon's flow engine.
3. **MCP Streamable HTTP/SSE transport** — progress streaming for long ops (imports,
   builds, lighting bakes) instead of one blocking TCP round-trip.

---

## 4. Suggested sequencing
1. **Architecture first this time** — adopt the command registry *before* adding Tier 3/4.
   At 10 categories the refactor now pays for itself immediately and de-risks the long tail.
2. **Tier 2** (PIE + Enhanced Input) — closes the test loop and fixes the legacy-input gap.
3. **Tier 3 → Tier 4** as demand dictates; keep `execute_python` for the long tail.
4. **`cook_project` + orchestration rollback** to close the last two competitive deltas.

---

## 5. Sources
- ChiR24/Unreal_mcp — https://github.com/ChiR24/Unreal_mcp
- db-lyon/ue-mcp — https://github.com/db-lyon/ue-mcp
- remiphilippe/mcp-unreal — https://github.com/remiphilippe/mcp-unreal
- Flux-Point-Studios/unreal-mcp — https://github.com/Flux-Point-Studios/unreal-mcp
- ayeletstudioindia/unreal-analyzer-mcp — https://github.com/ayeletstudioindia/unreal-analyzer-mcp
- chongdashu/unreal-mcp (origin) — https://github.com/chongdashu/unreal-mcp
