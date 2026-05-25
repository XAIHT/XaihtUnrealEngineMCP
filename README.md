<div align="center">

# Model Context Protocol for Unreal Engine
<span style="color: #555555">unreal-mcp</span>

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.7%2B-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.12%2B-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/chongdashu/unreal-mcp)

</div>

This project enables AI assistant clients like Cursor, Windsurf and Claude Desktop to control Unreal Engine through natural language using the Model Context Protocol (MCP).

## ⚠️ Experimental Status

This project is currently in an **EXPERIMENTAL** state. The API, functionality, and implementation details are subject to significant changes. While we encourage testing and feedback, please be aware that:

- Breaking changes may occur without notice
- Features may be incomplete or unstable
- Documentation may be outdated or missing
- Production use is not recommended at this time

## 🌟 Overview

The Unreal MCP integration provides comprehensive tools for controlling Unreal Engine through natural language — **~56 tools across 10 categories**:

| Category | Capabilities |
|----------|-------------|
| **Actor Management** | • Spawn and delete actors (static meshes, lights, cameras)<br>• Set actor transforms (position, rotation, scale)<br>• Query and set actor properties; find actors by name<br>• List all actors in the current level |
| **Blueprint Development** | • Create new Blueprint classes with custom components<br>• Add and configure components (mesh, camera, light, etc.)<br>• Set component properties, physics, and Pawn settings<br>• Compile Blueprints and spawn Blueprint actors |
| **Blueprint Node Graph** | • Add event nodes (BeginPlay, Tick, etc.)<br>• Create function call nodes and connect them<br>• Add variables with custom types and default values<br>• Create component and self references; find nodes |
| **Editor Control** | • Focus viewport on specific actors or locations<br>• Capture viewport screenshots to disk |
| **UMG (Widgets)** | • Create Widget Blueprints; add Text Block and Button widgets<br>• Bind widget events and text-block property bindings<br>• Add widgets to the viewport |
| **Input** | • Create legacy input action/axis mappings |
| **System / Introspection** | • Run Python inside the editor (`execute_python` — universal escape hatch)<br>• Execute console commands / CVars<br>• Reflect classes (`get_class_info`), list assets, call any bridge command |
| **Level / World** | • Open, create, and save levels<br>• Save all dirty packages; query the current level |
| **Assets** | • Import FBX/textures/audio; duplicate, rename, delete, save assets<br>• Create content-browser folders |
| **Materials** | • Create materials and material instances<br>• Set scalar/vector parameters; assign materials to actors |
| **Automation** | • Compose multi-step workflows (`run_macro`)<br>• Headless build and automation-test runs (editor-independent) |

All these capabilities are accessible through natural language commands via AI assistants, making it easy to automate and control Unreal Engine workflows. See **[Docs/Tools/](Docs/Tools/README.md)** for per-tool reference.

## 🧩 Components

### Sample Project (MCPGameProject) `MCPGameProject`
- Based off the Blank Project, but with the UnrealMCP plugin added.

### Plugin (UnrealMCP) `MCPGameProject/Plugins/UnrealMCP`
- Native TCP server for MCP communication (`127.0.0.1:55557`)
- Integrates with Unreal Editor subsystems
- Command handling is split into modular handlers under `Source/UnrealMCP/.../Commands/`:
  Editor/Actor, Blueprint, Blueprint Node, Project, UMG, **System**, **Level**,
  **Asset**, and **Material** commands
- `UnrealMCPBridge` routes each command to the appropriate handler and serializes responses

### Python MCP Server `Python/unreal_mcp_server.py`
- Implemented in `unreal_mcp_server.py`
- Manages TCP socket connections to the C++ plugin (port 55557)
- Handles command serialization and response parsing
- Provides error handling and connection management
- Loads and registers tool modules from the `tools` directory
- Uses the FastMCP library to implement the Model Context Protocol

## 📂 Directory Structure

- **MCPGameProject/** - Example Unreal project
  - **Plugins/UnrealMCP/** - C++ plugin source
    - **Source/UnrealMCP/Private/Commands/** & **Public/Commands/** - per-category
      command handlers (`UnrealMCP{Editor,Blueprint,BlueprintNode,Project,UMG,System,Level,Asset,Material}Commands`)
    - **UnrealMCP.uplugin** - Plugin definition (enables the Python Editor Script Plugin)

- **Python/** - Python server and tools
  - **unreal_mcp_server.py** - FastMCP server entry point; registers all tool modules
  - **tools/** - one module per tool category: `editor_tools.py`, `blueprint_tools.py`,
    `node_tools.py`, `project_tools.py`, `umg_tools.py`, `system_tools.py`,
    `level_tools.py`, `asset_tools.py`, `material_tools.py`, `automation_tools.py`
  - **scripts/** - Example scripts and demos

- **Docs/** - Comprehensive documentation
  - [SETUP_GUIDE.md](Docs/SETUP_GUIDE.md) - complete from-scratch setup
  - [Tools/](Docs/Tools/README.md) - per-tool reference
  - See [Docs/README.md](Docs/README.md) for the full documentation index

## 🚀 Quick Start Guide

> 📖 For a complete, step-by-step walkthrough (build flags, env setup, client config,
> troubleshooting), see the **[Complete Setup Guide](Docs/SETUP_GUIDE.md)**.

### Prerequisites
- Unreal Engine 5.7+
- Visual Studio 2022 (C++ game development workloads) to build the plugin
- Python 3.12+ (3.10+ works)
- MCP Client (e.g., Claude Desktop, Cursor, Windsurf, Claude Code)
- **Python Editor Script Plugin** enabled — required for the `execute_python` tool;
  the bundled `UnrealMCP.uplugin` enables it automatically

> ⚠️ This plugin now depends on the `PythonScriptPlugin`, `AssetTools`, and
> `MaterialEditor` modules (declared in `UnrealMCP.Build.cs`). Do a clean rebuild
> after pulling these changes.

### Sample project

For getting started quickly, feel free to use the starter project in `MCPGameProject`. This is a UE 5.7 Blank Starter Project with the `UnrealMCP.uplugin` already configured. 

1. **Prepare the project**
   - Right-click your .uproject file
   - Generate Visual Studio project files
2. **Build the project (including the plugin)**
   - Open solution (`.sln`)
   - Choose `Development Editor` as your target.
   - Build

### Plugin
Otherwise, if you want to use the plugin in your existing project:

1. **Copy the plugin to your project**
   - Copy `MCPGameProject/Plugins/UnrealMCP` to your project's Plugins folder

2. **Enable the plugin**
   - Edit > Plugins
   - Find "UnrealMCP" in Editor category
   - Enable the plugin
   - Restart editor when prompted

3. **Build the plugin**
   - Right-click your .uproject file
   - Generate Visual Studio project files
   - Open solution (`.sln)
   - Build with your target platform and output settings

### Python Server Setup

See [Python/README.md](Python/README.md) for detailed Python setup instructions, including:
- Setting up your Python environment
- Running the MCP server
- Using direct or server-based connections

### Configuring your MCP Client

Use the following JSON for your mcp configuration based on your MCP client.

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "<path/to/the/folder/PYTHON>",
        "run",
        "unreal_mcp_server.py"
      ]
    }
  }
}
```

An example is found in `mcp.json`

### MCP Configuration Locations

Depending on which MCP client you're using, the configuration file location will differ:

| MCP Client | Configuration File Location | Notes |
|------------|------------------------------|-------|
| Claude Desktop | `~/.config/claude-desktop/mcp.json` | On Windows: `%USERPROFILE%\.config\claude-desktop\mcp.json` |
| Cursor | `.cursor/mcp.json` | Located in your project root directory |
| Windsurf | `~/.config/windsurf/mcp.json` | On Windows: `%USERPROFILE%\.config\windsurf\mcp.json` |

Each client uses the same JSON format as shown in the example above. 
Simply place the configuration in the appropriate location for your MCP client.


## License
MIT

## Questions

For questions, you can reach me on X/Twitter: [@chongdashu](https://www.x.com/chongdashu)