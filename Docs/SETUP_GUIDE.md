# Unreal MCP — Complete Setup Guide (from scratch)

This guide takes you from nothing to a working Unreal Engine MCP setup that an AI client
(Claude Desktop, Cursor, Windsurf, Claude Code) can drive with natural language.

> Architecture in one line: **AI client → Python MCP server (`unreal_mcp_server.py`,
> stdio) → TCP socket on `127.0.0.1:55557` → C++ `UnrealMCP` plugin running inside the
> Unreal Editor.** The plugin's TCP server auto-starts when the editor loads.

---

## 0. Prerequisites

| Requirement | Version / notes |
|---|---|
| **Unreal Engine** | 5.5+ (installed via Epic Games Launcher or source) |
| **Visual Studio 2022** | Workloads: *Game development with C++* **and** *Desktop development with C++*. Required to compile the plugin. |
| **Python** | 3.10+ (3.12 recommended) |
| **uv** | Python package/runner used to launch the server |
| **Python Editor Script Plugin** | Ships with the editor; the `UnrealMCP.uplugin` enables it automatically (needed for the `execute_python` tool) |
| **An MCP client** | Claude Desktop, Cursor, Windsurf, or Claude Code |

> Windows is assumed below (matches this repo at `C:\Development\unreal-mcp`). macOS/Linux
> notes are called out where they differ.

---

## 1. Get the code

```powershell
git clone https://github.com/chongdashu/unreal-mcp.git
cd unreal-mcp
```

(If you already have this repo at `C:\Development\unreal-mcp`, skip this.)

Repository layout you care about:
- `MCPGameProject/` — a ready-to-use UE 5.5 project with the plugin pre-installed
- `MCPGameProject/Plugins/UnrealMCP/` — the C++ plugin
- `Python/` — the MCP server and tools

---

## 2. Build the Unreal plugin

You can either use the bundled sample project (fastest) or add the plugin to your own project.

### Option A — Use the bundled sample project (recommended first run)

1. **Generate Visual Studio project files**
   - Right-click `MCPGameProject/MCPGameProject.uproject` → **Generate Visual Studio
     project files**.
   - *(No right-click option? Run:
     `"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project="C:\Development\unreal-mcp\MCPGameProject\MCPGameProject.uproject" -game -engine`)*

2. **Build the editor target**
   - Open `MCPGameProject/MCPGameProject.sln` in Visual Studio 2022.
   - Set Solution Configuration to **Development Editor** and platform **Win64**.
   - **Build → Build Solution** (`Ctrl+Shift+B`). First build takes a while.

3. **Open the project**
   - Double-click `MCPGameProject.uproject` (or press **F5** in VS to launch the editor).
   - If prompted that modules are out of date / missing, click **Yes** to rebuild.

### Option B — Add the plugin to your existing project

1. Copy `MCPGameProject/Plugins/UnrealMCP` into `YourProject/Plugins/UnrealMCP`.
2. Regenerate VS project files for your `.uproject` (as above).
3. Build **Development Editor / Win64**.
4. In the editor: **Edit → Plugins**, search **UnrealMCP** (Editor category), ensure it's
   **Enabled**, restart if asked.

> ⚠️ **This repo's plugin has been extended** with System/Level/Asset/Material command
> classes and now depends on `PythonScriptPlugin`, `AssetTools`, and `MaterialEditor`.
> A clean rebuild is required after pulling these changes. The first compile is the
> moment to catch any C++ errors.

### Confirm the server started
When the editor finishes loading, open **Window → Output Log** and look for:
```
UnrealMCPBridge: Initializing
MCPServerRunnable: Server thread starting...
```
That means the TCP server is listening on `127.0.0.1:55557`. **Leave the editor open** —
the MCP server talks to it.

---

## 3. Set up the Python server

Open a terminal in the repo's `Python/` folder.

1. **Install uv** (if you don't have it)
   - Windows (PowerShell):
     ```powershell
     powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
     ```
   - macOS/Linux:
     ```bash
     curl -LsSf https://astral.sh/uv/install.sh | sh
     ```

2. **Create the virtual environment and install dependencies**
   ```powershell
   cd C:\Development\unreal-mcp\Python
   uv venv
   .venv\Scripts\activate        # macOS/Linux: source .venv/bin/activate
   uv pip install -e .
   ```

This installs `mcp[cli]`, `fastmcp`, `uvicorn`, `fastapi`, `pydantic`, `requests`.

---

## 4. Smoke test (no MCP client needed)

With the **editor open** (server listening on 55557), run a bundled test script that
connects directly over the socket:

```powershell
cd C:\Development\unreal-mcp\Python
uv run scripts/actors/test_cube.py
```

If it spawns a cube / prints a success response, the Python↔Unreal bridge works. If it
can't connect, see Troubleshooting.

You can also start the MCP server standalone to confirm it boots:
```powershell
uv run unreal_mcp_server.py
```
It will block waiting for stdio (that's expected); `Ctrl+C` to stop. Real use is via the
MCP client below.

---

## 5. Configure your MCP client

All clients use the same JSON. **Use an absolute path** to the `Python` folder:

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "C:/Development/unreal-mcp/Python",
        "run",
        "unreal_mcp_server.py"
      ]
    }
  }
}
```

Config file locations:

| Client | Location |
|---|---|
| **Claude Desktop** | `%APPDATA%\Claude\claude_desktop_config.json` (macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`) |
| **Cursor** | `.cursor/mcp.json` in your project root |
| **Windsurf** | `%USERPROFILE%\.codeium\windsurf\mcp_config.json` |
| **Claude Code** | `claude mcp add unrealMCP -- uv --directory C:/Development/unreal-mcp/Python run unreal_mcp_server.py` |

A template is in the repo at `mcp.json`. After editing, **fully restart the client**.

---

## 6. Run order & first commands

**Every session, in this order:**
1. **Open the Unreal Editor** (the project with the UnrealMCP plugin). Wait for the
   Output Log "Server thread starting..." line.
2. **Open your MCP client** — it launches the Python server automatically via the config.
3. Ask the AI something like:
   - "List all actors in the current level."
   - "Spawn a PointLight named KeyLight at 0,0,300."
   - "Create a material `M_Red` in /Game/Materials and assign it to KeyLight."

The model picks the matching tool (e.g. `get_actors_in_level`, `spawn_actor`,
`create_material`).

---

## 7. (Optional) Enable the headless automation tools

`build_project` and `run_automation_tests` shell out to the engine and work even with the
editor closed. Point them at your engine + project once via environment variables:

```powershell
setx UNREAL_EDITOR_CMD "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
setx UNREAL_UPROJECT  "C:\Development\unreal-mcp\MCPGameProject\MCPGameProject.uproject"
```
(Restart the terminal/client after `setx`.) Or pass `editor_cmd` / `uproject` as tool
arguments per call.

---

## 8. Available tool categories (~56 tools)

| Category | Module | Examples |
|---|---|---|
| Actor / Editor | `editor_tools.py` | `spawn_actor`, `delete_actor`, `set_actor_transform`, `take_screenshot` |
| Blueprint | `blueprint_tools.py` | `create_blueprint`, `add_component_to_blueprint`, `compile_blueprint` |
| Blueprint nodes | `node_tools.py` | `add_blueprint_event_node`, `connect_blueprint_nodes`, `add_blueprint_variable` |
| Input (legacy) | `project_tools.py` | `create_input_mapping` |
| UMG | `umg_tools.py` | `create_umg_widget_blueprint`, `add_button_to_widget` |
| **System** | `system_tools.py` | `execute_python`, `execute_console_command`, `get_class_info`, `list_assets`, `call_unreal` |
| **Level / World** | `level_tools.py` | `open_level`, `save_current_level`, `new_level`, `get_current_level` |
| **Assets** | `asset_tools.py` | `import_asset`, `duplicate_asset`, `delete_asset`, `create_folder` |
| **Materials** | `material_tools.py` | `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material` |
| **Automation** | `automation_tools.py` | `run_macro`, `build_project`, `run_automation_tests` |

`execute_python` is the universal escape hatch — anything the editor's `unreal` Python
API can do is reachable through it.

---

## 9. Troubleshooting

| Symptom | Fix |
|---|---|
| Client shows the server but tools fail / "Failed to connect to Unreal Engine" | The **editor must be open** first. Confirm the Output Log shows the server thread started. |
| No "Server thread starting..." in Output Log | Plugin not enabled or not built. Rebuild **Development Editor**; check **Edit → Plugins → UnrealMCP** is enabled. |
| `execute_python` returns "Python scripting is not available" | Enable **Python Editor Script Plugin** (Edit → Plugins), restart the editor. |
| Compile errors mentioning `PythonScriptPlugin` / `AssetTools` / `MaterialEditor` | Those modules are now build deps; make sure the engine has them (they ship with the editor) and regenerate project files + rebuild. |
| `uv` not found | Reopen the terminal after install, or add uv to PATH. |
| Port 55557 already in use | Another editor instance/server is running; close it. (Host/port are defined in `MCPServerRunnable.cpp` / `unreal_mcp_server.py`.) |
| Want detailed logs | See `Python/unreal_mcp.log` (DEBUG level) and the editor **Output Log**. |

---

## 10. Quick reference — minimal happy path

```text
1. Build plugin:   open MCPGameProject.sln → Development Editor / Win64 → Build
2. Open editor:    double-click MCPGameProject.uproject  (server auto-starts :55557)
3. Python env:     cd Python && uv venv && .venv\Scripts\activate && uv pip install -e .
4. Client config:  add mcp.json pointing "uv --directory <abs>/Python run unreal_mcp_server.py"
5. Restart client, keep editor open, start prompting.
```
