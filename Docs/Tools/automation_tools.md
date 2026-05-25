# Unreal MCP Automation Tools

This document describes the **automation / orchestration** tools in the Unreal MCP
integration. Unlike every other tool category, these are **pure Python** — they do
**not** have a dedicated C++ bridge handler.

> Python: `Python/tools/automation_tools.py` (`register_automation_tools`) ·
> No C++ backend.

## Overview

| Tool | Goes through bridge? | Purpose |
|------|:--:|---------|
| `run_macro` | yes (per step) | Execute a sequence of bridge commands as one workflow |
| `build_project` | no (shells out) | Headless compile via `UnrealEditor-Cmd` |
| `run_automation_tests` | no (shells out) | Headless automation test run via `UnrealEditor-Cmd` |

`build_project` and `run_automation_tests` shell out to the engine executable and
are **independent of the live editor TCP bridge**, so they work even when the
editor is closed. `run_macro` runs against the live editor.

---

## run_macro

Execute a sequence of bridge commands as a single workflow. Each step runs in
order against the live editor. Useful for composing a verified multi-step
operation (e.g. create blueprint → add component → compile → spawn) in one call.

**Parameters:**
- `steps` (array) - Ordered list of steps. Each step is an object:
  `{"command": <command_name>, "params": {<params>}}`.
- `stop_on_error` (boolean, default: `true`) - If true, stop at the first failing step.

**Returns:**
- `success` (boolean) - true if all executed steps succeeded
- `completed` (integer) - number of steps executed
- `total` (integer) - number of steps supplied
- `results` (array) - per-step `{step, command, success, response}`

**Example:**
```json
{
  "command": "run_macro",
  "params": {
    "stop_on_error": true,
    "steps": [
      { "command": "create_blueprint", "params": { "name": "BP_Door", "parent_class": "Actor" } },
      { "command": "add_component_to_blueprint", "params": { "blueprint_name": "BP_Door", "component_type": "StaticMeshComponent", "component_name": "Frame" } },
      { "command": "compile_blueprint", "params": { "blueprint_name": "BP_Door" } },
      { "command": "spawn_blueprint_actor", "params": { "blueprint_name": "BP_Door", "actor_name": "Door1", "location": [0, 0, 0] } }
    ]
  }
}
```

> **Note:** rollback is not implemented — `run_macro` is stop-on-error only. A
> failing step does not undo earlier steps.

---

## build_project

Headless-compile the project (no editor UI required).

Paths may be passed explicitly or supplied via environment variables so they do
not have to be passed every call:
- `UNREAL_EDITOR_CMD` - full path to `UnrealEditor-Cmd(.exe)`
- `UNREAL_UPROJECT` - full path to the `.uproject`

**Parameters:**
- `editor_cmd` (string, optional) - Path to `UnrealEditor-Cmd(.exe)`. Defaults to `$UNREAL_EDITOR_CMD`.
- `uproject` (string, optional) - Path to the `.uproject`. Defaults to `$UNREAL_UPROJECT`.
- `target` (string, default: `"Development"`) - Build configuration label (informational).
- `timeout_seconds` (integer, default: `1800`) - Max seconds to wait.

**Returns:**
- `success` (boolean), `return_code` (integer), `stdout_tail`, `stderr_tail`, `target`.

**Example:**
```json
{
  "command": "build_project",
  "params": {
    "editor_cmd": "C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe",
    "uproject": "C:/Development/unreal-mcp/MCPGameProject/MCPGameProject.uproject"
  }
}
```

Under the hood it runs the editor-cmd with
`-run=CompileAllBlueprints -unattended -nopause -nullrhi -stdout -FullStdOutLogOutput`.

---

## run_automation_tests

Run Unreal automation tests headlessly and return the result.

**Parameters:**
- `test_filter` (string) - Test name prefix/filter, e.g. `"Project."` or a full test path.
- `editor_cmd` (string, optional) - Path to `UnrealEditor-Cmd(.exe)`. Defaults to `$UNREAL_EDITOR_CMD`.
- `uproject` (string, optional) - Path to the `.uproject`. Defaults to `$UNREAL_UPROJECT`.
- `timeout_seconds` (integer, default: `1800`) - Max seconds to wait.

**Returns:**
- `success` (boolean), `return_code` (integer), `stdout_tail`, `stderr_tail`, `test_filter`.

**Example:**
```json
{
  "command": "run_automation_tests",
  "params": {
    "test_filter": "Project."
  }
}
```

Under the hood it runs the editor-cmd with
`-ExecCmds="Automation RunTests <filter>;Quit" -unattended -nopause -nullrhi -stdout -FullStdOutLogOutput`.

---

## Error Handling

If the engine paths cannot be resolved, the headless tools return:

```json
{
  "success": false,
  "message": "Provide editor_cmd + uproject (or set UNREAL_EDITOR_CMD / UNREAL_UPROJECT)."
}
```

## Implementation Notes

- `stdout_tail` / `stderr_tail` contain only the **last 100 lines** of output to
  keep responses small.
- The headless tools cannot be running at the same time as an editor that holds
  the same project lock; close the editor first or use a separate copy.
- These tools require an engine install on the host; they are not exercised by the
  TCP bridge.
