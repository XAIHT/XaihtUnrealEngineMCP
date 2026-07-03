# Unreal MCP Node Tools

This document provides detailed information about the Blueprint node tools available in the Unreal MCP integration.

## Overview

Node tools allow you to manipulate Blueprint graph nodes and connections programmatically, including adding event nodes, function nodes, variables, and creating connections between nodes.

## Node Tools

### add_blueprint_event_node

Add an event node to a Blueprint's event graph.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `event_name` (string) - Exact event member name, using the `Receive` prefix for
  standard events: `ReceiveBeginPlay`, `ReceiveTick`, etc.
- `node_position` (array, optional) - [X, Y] position in the graph (default: [0, 0])

**Returns:**
- Response containing the node ID and success status

**Example:**
```json
{
  "command": "add_blueprint_event_node",
  "params": {
    "blueprint_name": "MyActor",
    "event_name": "ReceiveBeginPlay",
    "node_position": [100, 100]
  }
}
```

### add_blueprint_input_action_node

Add an input action event node to a Blueprint's event graph.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `action_name` (string) - Name of the input action to respond to
- `node_position` (array, optional) - [X, Y] position in the graph (default: [0, 0])

**Returns:**
- Response containing the node ID and success status

**Example:**
```json
{
  "command": "add_blueprint_input_action_node",
  "params": {
    "blueprint_name": "MyActor",
    "action_name": "Jump",
    "node_position": [200, 200]
  }
}
```

### add_blueprint_function_node

Add a function call node to a Blueprint's event graph.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `target` (string) - Target object for the function (component name or self)
- `function_name` (string) - Name of the function to call
- `params` (object, optional) - Parameters to set on the function node
- `node_position` (array, optional) - [X, Y] position in the graph (default: [0, 0])

**Returns:**
- Response containing the node ID and success status

**Example:**
```json
{
  "command": "add_blueprint_function_node",
  "params": {
    "blueprint_name": "MyActor",
    "target": "Mesh",
    "function_name": "SetRelativeLocation",
    "params": {
      "NewLocation": [0, 0, 100]
    },
    "node_position": [300, 300]
  }
}
```

### connect_blueprint_nodes

Connect two nodes in a Blueprint's event graph.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `source_node_id` (string) - ID of the source node
- `source_pin` (string) - Name of the output pin on the source node
- `target_node_id` (string) - ID of the target node
- `target_pin` (string) - Name of the input pin on the target node

**Returns:**
- Response indicating success or failure

**Example:**
```json
{
  "command": "connect_blueprint_nodes",
  "params": {
    "blueprint_name": "MyActor",
    "source_node_id": "node_1",
    "source_pin": "exec",
    "target_node_id": "node_2",
    "target_pin": "exec"
  }
}
```

### add_blueprint_variable

Add a variable to a Blueprint.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `variable_type` (string) - Type of the variable. Supported values:
  `Boolean`, `Integer` (or `Int`), `Float`, `String`, `Vector`
- `variable_name` (string) - Name of the variable
- `is_exposed` (boolean, optional) - Whether to expose the variable to the editor (default: false)

> Setting a default value is **not supported** by the C++ handler — set it via
> `set_blueprint_property` or `execute_python` after creating the variable.

**Returns:**
- Response indicating success or failure

**Example:**
```json
{
  "command": "add_blueprint_variable",
  "params": {
    "blueprint_name": "MyActor",
    "variable_name": "Health",
    "variable_type": "Float",
    "is_exposed": true
  }
}
```

### create_input_mapping

Create an input mapping for the project.

**Parameters:**
- `action_name` (string) - Name of the input action
- `key` (string) - Key to bind (SpaceBar, LeftMouseButton, etc.)
- `input_type` (string, optional) - Type of input mapping (Action or Axis, default: "Action")

**Returns:**
- Response indicating success or failure

**Example:**
```json
{
  "command": "create_input_mapping",
  "params": {
    "action_name": "Jump",
    "key": "SpaceBar",
    "input_type": "Action"
  }
}
```

### add_blueprint_get_self_component_reference

Add a node that gets a reference to a component owned by the current Blueprint.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `component_name` (string) - Name of the component to get a reference to
- `node_position` (array, optional) - [X, Y] position in the graph (default: [0, 0])

**Returns:**
- Response containing the node ID and success status

**Example:**
```json
{
  "command": "add_blueprint_get_self_component_reference",
  "params": {
    "blueprint_name": "MyActor",
    "component_name": "Mesh",
    "node_position": [400, 400]
  }
}
```

### add_blueprint_self_reference

Add a 'Get Self' node to a Blueprint's event graph.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `node_position` (array, optional) - [X, Y] position in the graph (default: [0, 0])

**Returns:**
- Response containing the node ID and success status

**Example:**
```json
{
  "command": "add_blueprint_self_reference",
  "params": {
    "blueprint_name": "MyActor",
    "node_position": [500, 500]
  }
}
```

### find_blueprint_nodes

Find nodes in a Blueprint's event graph.

> The C++ handler currently implements **Event-node search only**; other node
> types return an empty list.

**Parameters:**
- `blueprint_name` (string) - Name of the target Blueprint
- `node_type` (string) - Type of node to find. Only `"Event"` is implemented.
- `event_name` (string) - Exact event member name to find, e.g.
  `"ReceiveBeginPlay"` (required when `node_type` is `"Event"`)

**Returns:**
- `node_guids` (array) - GUIDs of the matching nodes

**Example:**
```json
{
  "command": "find_blueprint_nodes",
  "params": {
    "blueprint_name": "MyActor",
    "node_type": "Event",
    "event_name": "ReceiveBeginPlay"
  }
}
```

## Error Handling

Bridge-level errors come back as a `status`/`error` pair; the Python tools may
also return `success`/`message` for connection problems.

```json
{
  "status": "error",
  "error": "Blueprint not found: MyActor"
}
```

All node tools resolve the target Blueprint at the fixed path
`/Game/Blueprints/<blueprint_name>`.

## Type Reference

### Node Types

Node types for the `find_blueprint_nodes` command:

- `Event` - Event nodes, matched by exact event member name (e.g.
  `ReceiveBeginPlay`, `ReceiveTick`) — **the only type implemented today**

### Variable Types

Variable types supported by the `add_blueprint_variable` command
(anything else returns an "Unsupported variable type" error):

- `Boolean` - True/false values
- `Integer` / `Int` - Whole numbers
- `Float` - Decimal numbers
- `String` - Text values
- `Vector` - 3D vector values (FVector)

For object/actor/component reference variables and other types, use
[`execute_python`](system_tools.md#execute_python).
