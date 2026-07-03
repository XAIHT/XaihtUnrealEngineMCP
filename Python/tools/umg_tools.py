"""
UMG Tools for Unreal MCP.

This module provides tools for creating and manipulating UMG Widget Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_umg_tools(mcp: FastMCP):
    """Register UMG tools with the MCP server."""

    @mcp.tool()
    def create_umg_widget_blueprint(
        ctx: Context,
        widget_name: str
    ) -> Dict[str, Any]:
        """
        Create a new UMG Widget Blueprint.

        The plugin always creates the Widget Blueprint under `/Game/Widgets/` with
        `UserWidget` as the parent class and a Canvas Panel as the root widget.
        Custom parent classes / paths are not supported by the C++ handler yet —
        use `execute_python` for those cases.

        Args:
            widget_name: Name of the widget blueprint to create

        Returns:
            Dict containing the created widget's name and path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # The C++ handler reads the blueprint name from the "name" key.
            params = {
                "name": widget_name
            }
            
            logger.info(f"Creating UMG Widget Blueprint with params: {params}")
            response = unreal.send_command("create_umg_widget_blueprint", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Create UMG Widget Blueprint response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating UMG Widget Blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_text_block_to_widget(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = "New Text Block",
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a Text Block widget to a UMG Widget Blueprint.

        The target Widget Blueprint must live under `/Game/Widgets/` (where
        `create_umg_widget_blueprint` creates it). Styling (size, font, color) is
        not supported by the C++ handler yet — set those via `execute_python`.

        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name to give the new Text Block
            text: Initial text content
            position: [X, Y] position in the canvas panel

        Returns:
            Dict containing the new text block's name and text
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # C++ handler keys: blueprint_name = target Widget Blueprint,
            # widget_name = name of the new Text Block.
            params = {
                "blueprint_name": widget_name,
                "widget_name": text_block_name,
                "text": text,
                "position": position
            }
            
            logger.info(f"Adding Text Block to widget with params: {params}")
            response = unreal.send_command("add_text_block_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Text Block response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Text Block to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_button_to_widget(
        ctx: Context,
        widget_name: str,
        button_name: str,
        text: str = "Button",
        position: List[float] = [0.0, 0.0]
    ) -> Dict[str, Any]:
        """
        Add a Button widget (with a text label) to a UMG Widget Blueprint.

        The target Widget Blueprint must live under `/Game/Widgets/`. Styling
        (size, font, colors) is not supported by the C++ handler yet — set those
        via `execute_python`.

        Args:
            widget_name: Name of the target Widget Blueprint
            button_name: Name to give the new Button
            text: Text to display on the button (required by the plugin)
            position: [X, Y] position in the canvas panel

        Returns:
            Dict containing success status and the new button's name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # C++ handler keys: blueprint_name = target Widget Blueprint,
            # widget_name = name of the new Button.
            params = {
                "blueprint_name": widget_name,
                "widget_name": button_name,
                "text": text,
                "position": position
            }
            
            logger.info(f"Adding Button to widget with params: {params}")
            response = unreal.send_command("add_button_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Button response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Button to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def bind_widget_event(
        ctx: Context,
        widget_name: str,
        widget_component_name: str,
        event_name: str
    ) -> Dict[str, Any]:
        """
        Bind an event on a widget component by creating the bound event node
        (e.g. OnClicked) in the Widget Blueprint's event graph.

        The C++ handler creates (or reuses) the standard bound event node for the
        widget class; custom target function names are not supported yet —
        implement the event body with the Blueprint node tools afterwards.

        Args:
            widget_name: Name of the target Widget Blueprint (under /Game/Widgets/)
            widget_component_name: Name of the widget component (button, etc.)
            event_name: Name of the event to bind (OnClicked, etc.)

        Returns:
            Dict containing success status and the bound event name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # C++ handler keys: blueprint_name = target Widget Blueprint,
            # widget_name = the component whose event is bound.
            params = {
                "blueprint_name": widget_name,
                "widget_name": widget_component_name,
                "event_name": event_name
            }
            
            logger.info(f"Binding widget event with params: {params}")
            response = unreal.send_command("bind_widget_event", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Bind widget event response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error binding widget event: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_to_viewport(
        ctx: Context,
        widget_name: str,
        z_order: int = 0
    ) -> Dict[str, Any]:
        """
        Prepare a Widget Blueprint for viewport display.

        Note: the C++ handler validates the widget class and returns its class
        path, but it does NOT add the widget to a running game viewport (that
        requires a game context). Use the returned class with `CreateWidget` +
        `AddToViewport` Blueprint nodes to display it in game.

        Args:
            widget_name: Name of the Widget Blueprint (under /Game/Widgets/)
            z_order: Z-order for the widget (echoed back; higher numbers on top)

        Returns:
            Dict with the widget's `class_path`, `z_order`, and a usage note
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # C++ handler key: blueprint_name = the Widget Blueprint.
            params = {
                "blueprint_name": widget_name,
                "z_order": z_order
            }
            
            logger.info(f"Adding widget to viewport with params: {params}")
            response = unreal.send_command("add_widget_to_viewport", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add widget to viewport response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding widget to viewport: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_text_block_binding(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        binding_property: str
    ) -> Dict[str, Any]:
        """
        Set up a Text property binding for a Text Block widget.

        Creates a Text member variable named `binding_property` plus a
        `Get<binding_property>` binding function in the Widget Blueprint. Only
        Text bindings are supported by the C++ handler.

        Args:
            widget_name: Name of the target Widget Blueprint (under /Game/Widgets/)
            text_block_name: Name of the Text Block to bind
            binding_property: Name of the variable/binding to create

        Returns:
            Dict containing success status and the binding name
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # C++ handler keys: blueprint_name = target Widget Blueprint,
            # widget_name = the Text Block, binding_name = variable to create.
            params = {
                "blueprint_name": widget_name,
                "widget_name": text_block_name,
                "binding_name": binding_property
            }
            
            logger.info(f"Setting text block binding with params: {params}")
            response = unreal.send_command("set_text_block_binding", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set text block binding response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting text block binding: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("UMG tools registered successfully") 