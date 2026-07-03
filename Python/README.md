# Unreal MCP

Python bridge for interacting with Unreal Engine 5.7+ using the Model Context Protocol (MCP).

This is the MCP **server** side. It runs `unreal_mcp_server.py` (a FastMCP server over
stdio), connects to the C++ `UnrealMCP` plugin over a TCP socket on
`127.0.0.1:55557`, and exposes 63 tools across 10 categories to your AI client.

> For the complete, from-scratch setup (including building the Unreal plugin and
> configuring your MCP client), see [`../Docs/SETUP_GUIDE.md`](../Docs/SETUP_GUIDE.md).

## Setup

1. Make sure Python 3.10+ is installed (3.12 recommended)
2. Install `uv` if you haven't already:
   ```bash
   curl -LsSf https://astral.sh/uv/install.sh | sh
   # Windows (PowerShell):
   # powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
   ```
3. Create and activate a virtual environment:
   ```bash
   uv venv
   source .venv/bin/activate  # On Unix/macOS
   # or
   .venv\Scripts\activate     # On Windows
   ```
4. Install dependencies:
   ```bash
   uv pip install -e .
   ```

At this point, you can configure your MCP Client (Claude Desktop, Cursor, Windsurf) to use the Unreal MCP Server as per the [Configuring your MCP Client](../README.md#configuring-your-mcp-client).

## Tool modules

Tools live in `tools/` and are registered in `unreal_mcp_server.py`:

| Module | Category | Tools |
|--------|----------|-------|
| `editor_tools.py` | Actor + Editor | `get_actors_in_level`, `find_actors_by_name`, `spawn_actor`, `delete_actor`, `set_actor_transform`, `get_actor_properties`, `set_actor_property`, `focus_viewport`, `take_screenshot`, `spawn_blueprint_actor` |
| `blueprint_tools.py` | Blueprint | `create_blueprint`, `add_component_to_blueprint`, `set_static_mesh_properties`, `set_component_property`, `set_physics_properties`, `compile_blueprint`, `set_blueprint_property`, `set_pawn_properties`, `save_blueprint`, `is_blueprint_dirty` |
| `node_tools.py` | Blueprint nodes | `add_blueprint_event_node`, `add_blueprint_input_action_node`, `add_blueprint_function_node`, `connect_blueprint_nodes`, `add_blueprint_variable`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`, `find_blueprint_nodes` |
| `project_tools.py` | Input (legacy) | `create_input_mapping` |
| `umg_tools.py` | UMG widgets | `create_umg_widget_blueprint`, `add_text_block_to_widget`, `add_button_to_widget`, `bind_widget_event`, `add_widget_to_viewport`, `set_text_block_binding` |
| `system_tools.py` | System / introspection | `execute_python`, `execute_python_file`, `execute_console_command`, `get_class_info`, `list_assets`, `get_supported_commands`, `call_unreal` |
| `level_tools.py` | Level / world | `open_level`, `save_current_level`, `save_all`, `new_level`, `get_current_level` |
| `asset_tools.py` | Assets | `import_asset`, `duplicate_asset`, `rename_asset`, `delete_asset`, `save_asset`, `create_folder` |
| `material_tools.py` | Materials | `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material`, `set_material_color`, `get_material_info`, `assign_material_to_all_slots` |
| `automation_tools.py` | Automation (pure Python) | `run_macro`, `build_project`, `run_automation_tests` |

Per-tool reference (parameters, returns, examples) is in [`../Docs/Tools/`](../Docs/Tools/README.md).

## Headless automation env vars (optional)

`build_project` and `run_automation_tests` shell out to the engine and work even when
the editor is closed. Set these once so you don't have to pass paths every call:

```bash
# Windows
setx UNREAL_EDITOR_CMD "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
setx UNREAL_UPROJECT  "C:\Development\unreal-mcp\MCPGameProject\MCPGameProject.uproject"
```

## Testing Scripts

There are several scripts in the [scripts](./scripts) folder. They are useful for testing the tools and the Unreal Bridge via a direct connection. This means that you do not need to have an MCP Server running.

You should make sure you have installed dependencies and/or are running in the `uv` virtual environment in order for the scripts to work.

## Troubleshooting

- Make sure the Unreal Engine editor is loaded and running before running the server
  (the plugin's TCP server starts when the editor loads).
- `execute_python` returning "Python scripting is not available" means the **Python
  Editor Script Plugin** is disabled — enable it (Edit → Plugins) and restart.
- Check logs in `unreal_mcp.log` (DEBUG level) for detailed error information.

## Development

The tool surface spans two layers — add new functionality in both:

1. **C++ (plugin) side.** Add a handler method to the matching
   `UnrealMCP<Category>Commands` class (or create a new one following the existing
   pattern), **and register the command name in
   `UnrealMCPBridge::BuildCommandRegistry`** — dispatch is driven by that registry,
   so an unregistered command is unreachable even if its handler exists.
   Rebuild the plugin in Unreal.
2. **Python (server) side.** Add a `@mcp.tool()` function to the relevant module in
   `tools/` (or a new `tools/<category>_tools.py` with a `register_<category>_tools(mcp)`
   function), then register it in `unreal_mcp_server.py`.

For a long-tail or one-off operation, you often don't need a dedicated tool at all —
use `execute_python` (run arbitrary editor Python) or `call_unreal` (invoke any bridge
command) from `system_tools.py`.

> Note: the older `Development` instructions referenced a `UnrealMCPBridge.py` and an
> "HTTP API". Those do not exist — the bridge is the C++ `UnrealMCPBridge` and the
> transport is stdio (MCP) ⇄ TCP (to the plugin).
