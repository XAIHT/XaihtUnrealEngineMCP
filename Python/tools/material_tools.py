"""
Material Tools for Unreal MCP.

Tools for creating materials and material instances, setting instance parameters,
and assigning materials to level actors.
"""

import logging
from typing import Dict, List, Any, Union
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_material_tools(mcp: FastMCP):
    """Register material tools with the MCP server."""

    def _send(command: str, params: Dict[str, Any]) -> Dict[str, Any]:
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command(command, params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            logger.info(f"{command} response: {response}")
            return response
        except Exception as e:
            error_msg = f"Error in {command}: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_material(ctx: Context, name: str, path: str = "/Game/Materials") -> Dict[str, Any]:
        """
        Create a new Material asset.

        Args:
            name: Name of the material asset.
            path: Content path to create it in.
        """
        return _send("create_material", {"name": name, "path": path})

    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        name: str,
        parent_material: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a Material Instance Constant from a parent material.

        Args:
            name: Name of the material instance asset.
            parent_material: Path of the parent material, e.g. "/Game/Materials/M_Base".
            path: Content path to create the instance in.
        """
        return _send("create_material_instance", {
            "name": name,
            "parent_material": parent_material,
            "path": path
        })

    @mcp.tool()
    def set_material_parameter(
        ctx: Context,
        material: str,
        parameter: str,
        value: Union[float, List[float]]
    ) -> Dict[str, Any]:
        """
        Set a parameter on a Material Instance.

        Args:
            material: Path of the material instance, e.g. "/Game/Materials/MI_Base".
            parameter: Parameter name as defined in the parent material.
            value: A number for a scalar parameter, or [r, g, b] / [r, g, b, a] for a
                   vector/color parameter.
        """
        return _send("set_material_parameter", {
            "material": material,
            "parameter": parameter,
            "value": value
        })

    @mcp.tool()
    def assign_material(
        ctx: Context,
        actor: str,
        material: str,
        slot: int = 0
    ) -> Dict[str, Any]:
        """
        Assign a material to a level actor's mesh component slot.

        Args:
            actor: Name of the level actor.
            material: Path of the material/instance to assign.
            slot: Material slot index (default 0).
        """
        return _send("assign_material", {
            "actor": actor,
            "material": material,
            "slot": slot
        })

    logger.info("Material tools registered successfully")
