# Unreal MCP Tools

This document is an index to all the tool categories supported by the Unreal MCP
integration. There are **~56 tools across 10 categories**.

Each category maps to a Python module in `Python/tools/` and (except the pure-Python
automation tools) a C++ command handler in
`MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/.../Commands/`.

## Core tools

| Category | Doc | Python module | Highlights |
|----------|-----|---------------|------------|
| Actor | [actor_tools.md](actor_tools.md) | `editor_tools.py` | `get_actors_in_level`, `spawn_actor`, `set_actor_transform`, `set_actor_property` |
| Editor | [editor_tools.md](editor_tools.md) | `editor_tools.py` | `focus_viewport`, `take_screenshot` |
| Blueprint | [blueprint_tools.md](blueprint_tools.md) | `blueprint_tools.py` | `create_blueprint`, `add_component_to_blueprint`, `compile_blueprint`, `set_pawn_properties` |
| Blueprint Nodes | [node_tools.md](node_tools.md) | `node_tools.py` | `add_blueprint_event_node`, `connect_blueprint_nodes`, `add_blueprint_variable` |
| UMG (Widgets) | [umg_tools.md](umg_tools.md) | `umg_tools.py` | `create_umg_widget_blueprint`, `add_button_to_widget`, `bind_widget_event` |
| Project (Input) | [project_tools.md](project_tools.md) | `project_tools.py` | `create_input_mapping` (legacy input) |

## Extended tools

| Category | Doc | Python module | Highlights |
|----------|-----|---------------|------------|
| System / Introspection | [system_tools.md](system_tools.md) | `system_tools.py` | `execute_python`, `execute_console_command`, `get_class_info`, `list_assets`, `call_unreal` |
| Level / World | [level_tools.md](level_tools.md) | `level_tools.py` | `open_level`, `save_current_level`, `save_all`, `new_level`, `get_current_level` |
| Assets | [asset_tools.md](asset_tools.md) | `asset_tools.py` | `import_asset`, `duplicate_asset`, `rename_asset`, `delete_asset`, `save_asset`, `create_folder` |
| Materials | [material_tools.md](material_tools.md) | `material_tools.py` | `create_material`, `create_material_instance`, `set_material_parameter`, `assign_material` |
| Automation (pure Python) | [automation_tools.md](automation_tools.md) | `automation_tools.py` | `run_macro`, `build_project`, `run_automation_tests` |

## Notes

- **`execute_python`** (System tools) is the universal escape hatch: anything the
  editor's `unreal` Python API can do is reachable through it, even without a
  dedicated tool.
- **Automation tools** are pure Python with no C++ handler. `build_project` and
  `run_automation_tests` shell out to `UnrealEditor-Cmd` and work even when the
  editor is closed.
- For setup and configuration, see the [Complete Setup Guide](../SETUP_GUIDE.md).
- For how the tool surface evolved and what is planned next, see the analysis docs:
  [IMPROVEMENT_PROPOSAL](../IMPROVEMENT_PROPOSAL.md),
  [NEW_OPERATIONS_PROPOSAL](../NEW_OPERATIONS_PROPOSAL.md), and
  [COMPARISON_V3_AND_NEXT_OPERATIONS](../COMPARISON_V3_AND_NEXT_OPERATIONS.md).
