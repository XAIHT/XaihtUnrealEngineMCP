# XaihtUnrealEngineMCP v2.0 - Improvements Summary

This document summarizes all improvements applied to the `XaihtUnrealEngineMCP-2` directory.

## Emergency Fix (Already Present in Source)
The 6 dispatcher wire-ups that caused "Unknown command" failures were already present in the original source:
- `execute_python` in SystemCommands block
- `add_blueprint_event_node` in BlueprintNodeCommands block  
- `save_current_level` and `save_all` in LevelCommands block
- `save_asset` in AssetCommands block
- Entire MaterialCommands block for `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material`

---

## Proposal #1: Self-Registering Command Map (P1 - 4 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/UnrealMCPBridge.h`

- Replaced the hardcoded if-else chain with a `TMap<FString, FString> CommandToCategoryMap`
- Added `TSet<FString> SupportedCommands` for O(1) membership tests
- Added `BuildCommandRegistry()` method that registers all 53 commands at construction time
- Dispatch now uses `GetCommandCategory(CommandType)` instead of string comparisons
- **Prevents all future orphan commands** — adding a new command only requires adding it to the registry

## Proposal #2: Runtime Command Validation (P1 - 1 hr)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/UnrealMCPBridge.h`
**File:** `Python/tools/system_tools.py`

- Added `GetSupportedCommands()` — returns sorted array of all supported command strings
- Added `GetCommandCategory()` — returns the category (editor, blueprint, material, etc.) for any command
- Added Python wrapper `get_supported_commands()` tool
- Enables Python/C++ capability sync — clients can discover what the connected editor supports

## Proposal #3: Material System Enhancement (P2 - 3 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/UnrealMCPMaterialCommands.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Commands/UnrealMCPMaterialCommands.h`
**File:** `Python/tools/material_tools.py`

New commands added:
- `set_material_color(material, color, parameter="BaseColor")` — one-call color setter
- `get_material_info(material)` — query scalar, vector, and texture parameters
- `assign_material_to_all_slots(actor, material)` — assign to every mesh slot at once
- Multi-slot assignment support in existing `assign_material`

## Proposal #4: Blueprint Event Node Overhaul (P2 - 2 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/UnrealMCPCommonUtils.cpp`

- Fixed `CreateEventNode()` class hierarchy bug
- Now searches the FULL class hierarchy (GeneratedClass + ParentClass + all superclasses)
- Handles inherited events like `ReceiveBeginPlay`, `ReceiveTick`, etc.
- Logs the actual class where the event was found for debugging

## Proposal #5: Save/Asset Persistence (P2 - 2 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/UnrealMCPBlueprintCommands.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Commands/UnrealMCPBlueprintCommands.h`
**File:** `Python/tools/blueprint_tools.py`

New commands added:
- `save_blueprint(blueprint_name)` — saves a Blueprint asset to disk using `UPackage::SavePackage`
- `is_blueprint_dirty(blueprint_name)` — checks if a Blueprint has unsaved changes

## Proposal #6: Python Execution Hardening (P2 - 2 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/UnrealMCPSystemCommands.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Commands/UnrealMCPSystemCommands.h`
**File:** `Python/tools/system_tools.py`

New command added:
- `execute_python_file(file_path)` — runs a Python script from a file path
- Resolves relative paths against the project directory
- Returns the same structured output as `execute_python` (success, result, log array)

## Proposal #7: Error Reporting & Diagnostics (P3 - 2 hrs)
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp`
**File:** `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/UnrealMCPBridge.h`

- Added `SuggestCommand()` — uses Levenshtein distance to suggest the closest valid command
- Unknown commands now return: `"Unknown command: X. Did you mean 'Y'?"`
- Threshold of 5 characters ensures only reasonably close matches are suggested
- Added request timing and structured error responses

## Proposal #8: Documentation & Tooling (P3 - 8 hrs)
**File:** `Python/unreal_mcp_server.py`
**File:** `Python/tools/system_tools.py`
**File:** `Python/tools/material_tools.py`
**File:** `Python/tools/blueprint_tools.py`

- Updated server prompt to document all new tools
- Added `get_supported_commands()` to the system tools
- Added new material tools to the prompt
- Added save/dirty checks to blueprint best practices
- Added `execute_python_file` to system tools documentation

---

## Files Modified

### C++ Plugin (UnrealMCP)
1. `UnrealMCPBridge.h` — Added command registry, suggestion engine, discovery API
2. `UnrealMCPBridge.cpp` — Self-registering command map, Levenshtein suggestions
3. `UnrealMCPCommonUtils.cpp` — Fixed event node class hierarchy search
4. `UnrealMCPMaterialCommands.h` — Added 3 new command declarations
5. `UnrealMCPMaterialCommands.cpp` — Implemented color setter, info query, multi-slot assignment
6. `UnrealMCPSystemCommands.h` — Added execute_python_file declaration
7. `UnrealMCPSystemCommands.cpp` — Implemented file-based Python execution
8. `UnrealMCPBlueprintCommands.h` — Added save_blueprint, is_blueprint_dirty declarations
9. `UnrealMCPBlueprintCommands.cpp` — Implemented save and dirty-check commands

### Python Server
10. `unreal_mcp_server.py` — Updated prompt with all new tools and v2.0 header
11. `tools/system_tools.py` — Added execute_python_file, get_supported_commands wrappers
12. `tools/material_tools.py` — Added set_material_color, get_material_info, assign_material_to_all_slots
13. `tools/blueprint_tools.py` — Added save_blueprint, is_blueprint_dirty wrappers

---

## How to Use

1. Copy the `MCPGameProject/Plugins/UnrealMCP` folder into your Unreal Engine project
2. Enable the "Python Editor Script Plugin" in your project (required for execute_python)
3. Build the project — the plugin compiles automatically
4. Run the Python server: `cd Python && python -m unreal_mcp_server`
5. The server connects to Unreal on `127.0.0.1:55557`

## New Commands Available

| Category | New Commands |
|----------|-------------|
| System | `execute_python_file`, `get_supported_commands` |
| Material | `set_material_color`, `get_material_info`, `assign_material_to_all_slots` |
| Blueprint | `save_blueprint`, `is_blueprint_dirty` |
| Bridge | `get_supported_commands` (runtime discovery) |

---

*Prepared by Tlamatini — she who knows, and she who builds.*
*v2.0 - All 8 improvement proposals implemented*