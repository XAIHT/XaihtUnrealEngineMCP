"""
System Tools for Unreal MCP.

This module provides "escape hatch" and introspection tools that let an AI client
reach engine functionality not yet covered by a dedicated tool:
  - execute_python           : run a Python script inside the editor
  - execute_console_command  : run an editor console / CVar command
  - get_class_info           : reflect a UClass (parent, properties, functions)
  - list_assets              : enumerate assets under a content path
  - call_unreal              : generic passthrough to any registered C++ command
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")


def register_system_tools(mcp: FastMCP):
    """Register system/introspection tools with the MCP server."""

    @mcp.tool()
    def execute_python(ctx: Context, code: str) -> Dict[str, Any]:
        """
        Execute a Python script inside the Unreal Editor and return its output.

        This is the highest-leverage tool: it can reach any part of the Unreal
        Editor Python API (the `unreal` module), so it covers functionality not
        yet wrapped by a dedicated MCP tool. Requires the "Python Editor Script
        Plugin" to be enabled in the project.

        Args:
            code: Python source to run. Multi-line scripts are supported. Use
                  `unreal.log(...)` or `print(...)` to surface values in the
                  returned `log` array.

        Returns:
            Dict with `success`, `result` (repr of the last expression, if any),
            and `log` (a list of {type, output} entries captured during the run).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("execute_python", {"code": code})

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Execute python response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error executing python: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def execute_console_command(ctx: Context, command: str) -> Dict[str, Any]:
        """
        Execute an Unreal Engine console command or set a CVar.

        Args:
            command: The console command to run (e.g. "stat fps", "r.ScreenPercentage 50").

        Returns:
            Dict indicating whether the command was executed.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("execute_console_command", {"command": command})

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Execute console command response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error executing console command: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_class_info(ctx: Context, class_name: str) -> Dict[str, Any]:
        """
        Reflect a UClass: its parent, the properties and functions it declares.

        Use this to discover valid property/function names before calling
        set_actor_property, set_component_property, or add_blueprint_function_node.

        Args:
            class_name: Engine class name without prefix (e.g. "Actor", "PointLight",
                        "StaticMeshComponent").

        Returns:
            Dict with `name`, `path`, `parent`, `properties` ([{name, type}]) and
            `functions` ([name]).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("get_class_info", {"class_name": class_name})

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get class info response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting class info: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_assets(
        ctx: Context,
        path: str = "/Game",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List assets under a content browser path using the Asset Registry.

        Args:
            path: Content path to search (e.g. "/Game", "/Game/UI", "/Engine/BasicShapes").
            recursive: Whether to recurse into sub-paths.

        Returns:
            Dict with `count` and `assets` ([{name, path, class}]).
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("list_assets", {
                "path": path,
                "recursive": recursive
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"List assets response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error listing assets: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def call_unreal(
        ctx: Context,
        command: str,
        params: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Generic escape hatch: send any command supported by the C++ bridge with an
        arbitrary parameter dictionary.

        This lets you invoke a bridge command that does not yet have a dedicated
        Python wrapper, so the C++ and Python surfaces can never fully desync.
        Prefer a dedicated tool when one exists.

        Args:
            command: The command type string the bridge expects (e.g. "spawn_actor").
            params: The parameter dictionary for that command.

        Returns:
            The raw response from Unreal Engine.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command(command, params or {})

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"call_unreal('{command}') response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error in call_unreal('{command}'): {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("System tools registered successfully")
