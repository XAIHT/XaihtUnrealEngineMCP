# Unreal MCP — Capability Analysis & Improvement Proposal

> Prepared 2026-05-24. Compares this repository (a fork of `chongdashu/unreal-mcp`)
> against the leading Unreal Engine MCP servers on GitHub, identifies code-level and
> capability deficiencies, and proposes a prioritized roadmap to "improve a lot".

---

## 1. Executive summary

This project is a clean, minimal **Python (FastMCP) ⇄ C++ TCP plugin** bridge for driving
the Unreal Editor with natural language. It ships ~30 commands across 5 categories
(actors, blueprints, blueprint-nodes, UMG, input). Its architecture is the easiest of
all surveyed projects to read and extend, and it is the most-starred origin in the
ecosystem (~1.9k stars upstream).

However, it is explicitly **EXPERIMENTAL** and now lags the field badly on **breadth**.
Competing servers expose materials, Niagara/VFX, landscape/foliage, Sequencer,
audio, level/World-Partition management, asset import, Enhanced Input, Play-In-Editor
control, reflection/introspection, headless build/cook/test, and a Python passthrough.
This repo has **none** of those. It also carries concrete code defects (a dead command
route, two disabled tools, a screenshot command implemented in C++ but never exposed,
and a fragile single-threaded transport).

The proposal below is in three layers: **(P0) fix what is broken**, **(P1) modernize the
architecture so breadth is cheap to add**, and **(P2) close the biggest capability gaps**
with ~9 new command categories.

---

## 2. Current operations inventory (verified against source)

Commands are dispatched in `UnrealMCPBridge.cpp` (`HandleCommand`) to five handler
classes. The table reflects what is **actually wired**, and whether it is exposed as a
Python MCP tool.

| Category | C++ command | Python tool | Notes |
|----------|-------------|-------------|-------|
| **Editor/Actor** | `get_actors_in_level` | ✅ | |
| | `find_actors_by_name` | ✅ | |
| | `spawn_actor` / `create_actor` | ✅ | basic shapes, lights, camera |
| | `delete_actor` | ✅ | |
| | `set_actor_transform` | ✅ | |
| | `get_actor_properties` | ✅ | |
| | `set_actor_property` | ✅ | |
| | `spawn_blueprint_actor` | ✅ | also routed via Blueprint handler |
| | `focus_viewport` | ⚠️ **disabled** | `@mcp.tool()` commented out — "buggy" |
| | `take_screenshot` | ❌ **not exposed** | implemented in C++, no Python tool |
| **Blueprint** | `create_blueprint` | ✅ | |
| | `add_component_to_blueprint` | ✅ | |
| | `set_component_property` | ✅ | |
| | `set_physics_properties` | ✅ | |
| | `set_static_mesh_properties` | ✅ | |
| | `set_blueprint_property` | ✅ | CDO property |
| | `compile_blueprint` | ✅ | |
| | `set_pawn_properties` | ⚠️ **disabled** | commented out; use `set_component_property` |
| **Blueprint Nodes** | `add_blueprint_event_node` | ✅ | |
| | `add_blueprint_input_action_node` | ✅ | |
| | `add_blueprint_function_node` | ✅ | |
| | `connect_blueprint_nodes` | ✅ | |
| | `add_blueprint_variable` | ✅ | |
| | `add_blueprint_get_self_component_reference` | ✅ | |
| | `add_blueprint_self_reference` | ✅ | |
| | `find_blueprint_nodes` | ✅ | |
| | `add_blueprint_get_component_node` | ❌ **dead route** | listed in bridge dispatch, **no handler** in `UnrealMCPBlueprintNodeCommands` |
| **Project** | `create_input_mapping` | ✅ | **legacy** input only (not Enhanced Input) |
| **UMG** | `create_umg_widget_blueprint` | ✅ | |
| | `add_text_block_to_widget` | ✅ | |
| | `add_button_to_widget` | ✅ | only Text + Button widget types |
| | `bind_widget_event` | ✅ | |
| | `set_text_block_binding` | ✅ | |
| | `add_widget_to_viewport` | ✅ | |

**Net: ~30 commands implemented, ~26 usable from an AI client.**

---

## 3. Deficiencies

### 3.1 Code-level defects (found in source)
1. **Dead command route** — `add_blueprint_get_component_node` is dispatched in
   `UnrealMCPBridge.cpp` but has **no matching branch** in
   `UnrealMCPBlueprintNodeCommands::HandleCommand`. Any call falls through to an
   "unknown command" error.
2. **Two disabled tools** — `focus_viewport` (editor) and `set_pawn_properties`
   (blueprint) are commented out because they were buggy; the C++ handlers still exist,
   so the capability is half-built.
3. **`take_screenshot` orphaned** — fully handled in `UnrealMCPEditorCommands` but never
   registered as a Python `@mcp.tool()`, so no client can call it.
4. **Fragile transport** — `unreal_mcp_server.py` opens a **new TCP connection per
   command**, relies on "parse-as-JSON-to-detect-end-of-message" framing (no length
   prefix), and "pings" with a raw `b'\x00'` byte. This breaks on large payloads and
   concurrent calls.
5. **Brittle dispatch** — command routing is a hand-maintained `if / else if` chain
   duplicated in the bridge *and* each handler. Every new command edits two places and
   risks exactly the kind of desync that produced defect #1.
6. **Inconsistent result contract** — Python normalizes *two* different error shapes
   (`status:error` and `success:false`); handlers don't share a single response schema.
7. **Always-on DEBUG logging** to a file with no rotation or level config.

### 3.2 Capability gaps (vs. the field)
No support for any of: **Materials** (create/instance/parameters), **Niagara / VFX**,
**Landscape & Foliage**, **Sequencer / cinematics**, **Audio** (SoundCue/MetaSound),
**Level & World management** (open/save/create level, World Partition, streaming),
**Asset import** (FBX/textures/CSV→DataTable), **Enhanced Input** (only legacy
`PlayerInput` action mappings), **Play-In-Editor control**, **console/CVar execution**,
**reflection/introspection** of classes & properties, **Python passthrough**, and
**headless build/cook/automated tests**. UMG is limited to Text + Button only.

---

## 4. Competitive landscape (GitHub, May 2026)

| Repo | Lang | Transport | ~Tools | Stars | Defining strength |
|------|------|-----------|--------|-------|-------------------|
| **chongdashu/unreal-mcp** (this fork's origin) | C++/Python | TCP 55557 + FastMCP | ~30 | ~1.9k | Original/reference; simplest to extend; *EXPERIMENTAL* |
| **ChiR24/Unreal_mcp** | C++/TS | Native HTTP+SSE, WS, stdio | 22 (bundled) | ~645 | Bridge-free streaming HTTP; inline **Python exec**; AI gameplay systems (GAS/AI/combat) |
| **db-lyon/ue-mcp** | C++/TS | stdio + WS/JSON-RPC | 21 cats / **525+ actions** | ~111 | **YAML flow engine** (multi-step, rollback, retry); npm plugin system; auto-feedback |
| **remiphilippe/mcp-unreal** | Go/C++ | stdio + Remote Control + plugin HTTP | 49 | ~31 | **Headless CI/CD** build/cook/test; **local UE doc index**; Fab import; UE 5.7 only |
| **Flux-Point-Studios/unreal-mcp** | C++/TS | socket 8091 | 36 | ~5 | Finest-grained tools; **perf profiling**; optional GraphQL |
| **ayeletstudioindia/unreal-analyzer-mcp** | TS | stdio | 9 | ~152 | Static C++ **source analysis** (Tree-sitter); complementary, not editor control |

---

## 5. Feature comparison matrix

Legend: ✅ full · 🟡 partial/limited · ❌ none

| Capability | **This repo** | ChiR24 | db-lyon | remiphilippe | Flux-Point |
|---|:--:|:--:|:--:|:--:|:--:|
| Actor spawn/transform/props | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint create/compile | ✅ | ✅ | ✅ | ✅ | ✅ |
| Blueprint node-graph editing | ✅ | ✅ | ✅ | ✅ | ✅ |
| UMG widgets | 🟡 (2 types) | ✅ | ✅ | 🟡 | ✅ |
| Input mappings | 🟡 (legacy only) | ✅ | ✅ | ✅ (Enhanced) | ✅ (Enhanced) |
| Screenshot/viewport | 🟡 (orphaned) | ✅ | ✅ | ✅ | ✅ |
| **Materials** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Niagara / VFX** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Landscape / Foliage** | ❌ | ✅ | ✅ | 🟡 | ✅ |
| **Sequencer / cinematics** | ❌ | ✅ | ✅ | ❌ | ✅ |
| **Audio** | ❌ | ✅ | ✅ | ❌ | ✅ |
| **Level / World mgmt** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Asset import** | ❌ | ✅ | ✅ | ✅ | 🟡 |
| **PIE control** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Console / CVar exec** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Python passthrough** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Reflection / introspection** | ❌ | ✅ | ✅ | ✅ | ✅ |
| **Headless build/cook/test** | ❌ | 🟡 | 🟡 | ✅ | 🟡 |
| **Workflow orchestration** | ❌ | ❌ | ✅ (YAML) | ❌ | ❌ |
| Transport robustness | 🟡 (per-cmd TCP) | ✅ (HTTP/SSE) | ✅ (WS) | ✅ | ✅ |

**Takeaway:** this repo is competitive on the *first six rows* and absent on
essentially everything below them.

---

## 6. Proposal

### P0 — Fix what is broken (days, no new surface)  — ✅ IMPLEMENTED 2026-05-24
1. ✅ **Removed the dead `add_blueprint_get_component_node` route** from the bridge
   dispatch (it had no handler *and* no Python caller). The codebase is now consistent.
2. ✅ **Exposed `take_screenshot`** as a Python tool (`filepath` param → saved `.png`).
3. ✅ **Re-enabled `focus_viewport`** (added a null-guard on `GEditor->GetActiveViewport()`
   in the C++ handler to prevent the editor crash that got it disabled) **and
   `set_pawn_properties`** (pure-Python wrapper over the working `set_blueprint_property`).
4. ✅ **Hardened the TCP receive loop** in `MCPServerRunnable::Run()`: fixed an
   out-of-bounds null-terminator write (`Buffer[BytesRead]` on a full 8192-byte buffer)
   and made it **accumulate-until-valid-JSON** so payloads larger than one read buffer
   no longer truncate. The wire format is unchanged, so the Python client needs no
   protocol change. Also removed the stray `b'\x00'` liveness ping on the Python side.
5. ⏸️ **Deferred to P1** — a single response schema across *all* C++ handlers is a
   sweeping refactor that cannot be safely verified without compiling the plugin, so it
   is bundled with the P1 registry work rather than done blind. The Python layer already
   normalizes the two existing error shapes, so nothing is broken in the meantime.

> ⚠️ The C++ changes (items 1, 3, 4) require **rebuilding the UnrealMCP plugin** in
> Unreal Engine to take effect; they were not compiled in this environment.

### P1 — Modernize the architecture so breadth is cheap  — ◧ PARTIALLY IMPLEMENTED 2026-05-24
8. ✅ **Python passthrough** — added a new self-contained `FUnrealMCPSystemCommands`
   handler with **`execute_python`** (`IPythonScriptPlugin::ExecPythonCommandEx`,
   captures stdout/stderr/result). This is the single highest-leverage addition: it
   reaches *any* editor API not yet wrapped. Requires the **Python Editor Script
   Plugin** (now declared in `UnrealMCP.uplugin` + `UnrealMCP.Build.cs`).
9. ✅ **Reflection/introspection** — `get_class_info` (parent, declared properties +
   functions via `TFieldIterator`) and `list_assets` (Asset Registry query by path).
   Also added **`execute_console_command`** (console / CVar execution).
7. ✅ **Generic `call_unreal(command, params)` Python tool** — an escape hatch that can
   invoke *any* bridge command without a dedicated wrapper, so the C++ and Python
   surfaces can never fully desync from the client side (the lighter half of item 7).
6. ⏸️ **Deferred — command registry (`TMap` dispatch).** This refactors the hot path
   that *every* command flows through; doing it blind without a compiler is high-blast-
   radius. It is the recommended next step for someone who can build the plugin. Note
   the new System handler was wired **additively** (one new `else if` block + a new
   handler class), so a defect in it cannot affect existing commands — exactly the
   isolation the registry would later formalize.

> ⚠️ All P1 C++ additions require **rebuilding the UnrealMCP plugin** and having the
> **Python Editor Script Plugin** available; they were not compiled in this environment.

**New commands added (8 + 9 + 7):** `execute_python`, `execute_console_command`,
`get_class_info`, `list_assets`, `call_unreal` (Python-only).

#### Remaining P1 backlog
6. **Command registry, not if/else.** Replace the dispatch chains with a
   `TMap<FString, TFunction<...>>` so new commands self-register and the bridge no
   longer needs editing (kills defect-class #1/#5).
7b. **Auto-generate the Python tool stubs** from a shared command manifest (the heavier
   half of item 7), so dedicated wrappers stay in sync with the C++ automatically.

### P2 — Close the biggest capability gaps (new command categories)  — ◧ TIER-1 IMPLEMENTED 2026-05-24
✅ **Level/World** (`FUnrealMCPLevelCommands`): `open_level`, `save_current_level`,
`save_all`, `new_level`, `get_current_level`.
✅ **Materials** (`FUnrealMCPMaterialCommands`): `create_material`,
`create_material_instance`, `set_material_parameter` (scalar/vector), `assign_material`.
✅ **Asset import/mgmt** (`FUnrealMCPAssetCommands`): `import_asset`, `duplicate_asset`,
`rename_asset`, `delete_asset`, `save_asset`, `create_folder`.
⏳ **Remaining** (next additive handlers): Enhanced Input, PIE & runtime, Niagara/VFX,
Audio, Sequencer, Landscape/Foliage — see `NEW_OPERATIONS_PROPOSAL.md` for signatures.

> ⚠️ New C++ requires rebuilding the plugin; build deps `AssetTools` + `MaterialEditor`
> were added to `UnrealMCP.Build.cs`. Not compiled in this environment.

Add the remaining handler classes mirroring the existing pattern. Priority by user value:

| New category | Key commands |
|---|---|
| **Level / World** | `open_level`, `save_level`, `create_level`, `stream_level`, `set_world_settings` |
| **Materials** | `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material` |
| **Asset import** | `import_fbx`, `import_texture`, `import_csv_datatable`, `duplicate_asset`, `delete_asset` |
| **Enhanced Input** | `create_input_action`, `create_input_mapping_context`, `add_input_mapping` |
| **PIE & runtime** | `start_pie`, `stop_pie`, `possess_pawn`, `run_console_command`, `set_cvar` |
| **Niagara / VFX** | `spawn_niagara_system`, `set_niagara_parameter` |
| **Audio** | `play_sound_at_location`, `create_sound_cue` |
| **Sequencer** | `create_level_sequence`, `add_track`, `add_keyframe`, `play_sequence` |
| **Landscape / Foliage** | `create_landscape`, `paint_foliage`, `add_foliage_type` |

### P3 — Differentiators (optional, longer horizon)  — ◧ PARTIALLY IMPLEMENTED 2026-05-24
10. ✅ **Headless mode** (pure Python, `automation_tools.py`): `build_project` and
    `run_automation_tests` shell out to `UnrealEditor-Cmd` (paths via args or the
    `UNREAL_EDITOR_CMD` / `UNREAL_UPROJECT` env vars). Independent of the live editor.
    *(`cook_project` still TODO.)*
11. ✅ **Workflow orchestration** (pure Python): `run_macro(steps, stop_on_error)` runs a
    sequence of bridge commands as one workflow with structured per-step results.
    *(Full rollback is not implemented — stop-on-error only; noted honestly.)*
12. ⏸️ **Transport upgrade** to MCP Streamable HTTP/SSE — still TODO.

> The P3 items above are **pure Python and were syntax-verified** here. The headless
> tools can't be runtime-tested without an engine install, but require no plugin rebuild.

---

## 7. How to extend (matches the existing pattern)

For each new category, follow the established structure:
1. Create `UnrealMCP<Category>Commands.{h,cpp}` exposing
   `HandleCommand(const FString& Type, TSharedPtr<FJsonObject> Params)`.
2. Construct it in the `UUnrealMCPBridge` ctor and route its command names in
   `HandleCommand` (**or** adopt the P1 registry so this step disappears).
3. Add a `Python/tools/<category>_tools.py` with `register_<category>_tools(mcp)` and
   register it in `unreal_mcp_server.py`.
4. Reuse `UnrealMCPCommonUtils` for JSON parse/serialize and the standard response shape.

---

## 8. Sources
- chongdashu/unreal-mcp — https://github.com/chongdashu/unreal-mcp
- ChiR24/Unreal_mcp — https://github.com/ChiR24/Unreal_mcp
- db-lyon/ue-mcp — https://github.com/db-lyon/ue-mcp
- remiphilippe/mcp-unreal — https://github.com/remiphilippe/mcp-unreal
- Flux-Point-Studios/unreal-mcp — https://github.com/Flux-Point-Studios/unreal-mcp
- ayeletstudioindia/unreal-analyzer-mcp — https://github.com/ayeletstudioindia/unreal-analyzer-mcp
