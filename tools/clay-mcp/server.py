#!/usr/bin/env python3
"""Clay Designer MCP server.

Talks MCP over stdio to an AI agent, and JSON-RPC over TCP to a running
Clay instance (Tools → AI Agent Bridge on 127.0.0.1:9877).
"""

from __future__ import annotations

import json
import os
import socket
import sys
from typing import Any

HOST = os.environ.get("CLAY_MCP_HOST", "127.0.0.1")
PORT = int(os.environ.get("CLAY_MCP_PORT", "9877"))


def clay_call(method: str, params: dict[str, Any] | None = None) -> Any:
    payload = json.dumps({
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params or {},
    }) + "\n"
    with socket.create_connection((HOST, PORT), timeout=30) as sock:
        sock.sendall(payload.encode("utf-8"))
        chunks: list[bytes] = []
        while True:
            data = sock.recv(65536)
            if not data:
                break
            chunks.append(data)
            if b"\n" in data:
                break
    raw = b"".join(chunks).decode("utf-8").strip()
    if not raw:
        raise RuntimeError(
            "Clay MCP bridge returned an empty response. "
            "Open Clay and start Tools → AI Agent Bridge (MCP Server)."
        )
    reply = json.loads(raw.splitlines()[-1])
    if "error" in reply:
        raise RuntimeError(reply["error"].get("message", str(reply["error"])))
    return reply.get("result")


TOOLS = [
    {
        "name": "open_image_in_clay",
        "description": "Open an image file in the running Clay Designer app.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "required": ["path"],
        },
    },
    {
        "name": "get_canvas_preview",
        "description": "Export a PNG snapshot of the active Clay canvas and return its path.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "beautify_image",
        "description": "Apply a Clay beautify preset (auto_beautify, skin_smooth_portrait, cinematic_grade, hdr_pop, denoise_and_sharpen).",
        "inputSchema": {
            "type": "object",
            "properties": {"preset": {"type": "string", "default": "auto_beautify"}},
        },
    },
    {
        "name": "content_aware_fill",
        "description": "Texture patch + diffusion heal inside the selection. Use make_random_white_patch for demos; quality high|medium|low.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "rect": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "Only used with replace_selection=true",
                },
                "replace_selection": {"type": "boolean", "default": False},
                "auto_select_white": {"type": "boolean", "default": True},
                "make_random_white_patch": {"type": "boolean", "default": False},
                "quality": {"type": "string", "enum": ["high", "medium", "low"], "default": "high"},
                "sampling_radius": {"type": "integer"},
                "refinement_passes": {"type": "integer"},
            },
        },
    },
    {
        "name": "resize_image",
        "description": "Scale the active image and set resolution (DPI).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "width": {"type": "integer", "default": 150},
                "height": {"type": "integer", "default": 150},
                "dpi": {"type": "number", "default": 88},
            },
        },
    },
    {
        "name": "apply_gegl_filter",
        "description": "Apply a GEGL operation to the active drawable.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "operation": {"type": "string"},
                "properties": {"type": "object"},
            },
            "required": ["operation"],
        },
    },
    {
        "name": "edit_layer_or_selection",
        "description": "Manage selections. action=select_rectangle with rect, or action=none.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "action": {"type": "string"},
                "rect": {"type": "array", "items": {"type": "number"}},
            },
            "required": ["action"],
        },
    },
    {
        "name": "add_text_overlay",
        "description": "Execute Python in Clay to add a text layer.",
        "inputSchema": {
            "type": "object",
            "properties": {"text": {"type": "string"}},
            "required": ["text"],
        },
    },
    {
        "name": "export_image",
        "description": "Save/export the active Clay image to a path.",
        "inputSchema": {
            "type": "object",
            "properties": {"path": {"type": "string"}},
            "required": ["path"],
        },
    },
    {
        "name": "search_clay_app_store",
        "description": "Search the Clay App Store catalog keywords (content-aware, beautify, vfx, mcp).",
        "inputSchema": {
            "type": "object",
            "properties": {"query": {"type": "string"}},
            "required": ["query"],
        },
    },
    {
        "name": "install_clay_plugin",
        "description": "Ask Clay to enable/install a store extension by AppStream id.",
        "inputSchema": {
            "type": "object",
            "properties": {"entry_id": {"type": "string"}},
            "required": ["entry_id"],
        },
    },
    {
        "name": "run_pdb_procedure",
        "description": "Run any Clay PDB procedure by name.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "arguments": {"type": "object"},
            },
            "required": ["name"],
        },
    },
    {
        "name": "execute_clay_python",
        "description": "Execute Python inside Clay with Clay, Gegl, image, drawable in scope.",
        "inputSchema": {
            "type": "object",
            "properties": {"code": {"type": "string"}},
            "required": ["code"],
        },
    },
    {
        "name": "get_session_state",
        "description": "List open images and Clay session info.",
        "inputSchema": {"type": "object", "properties": {}},
    },
]


def dispatch_tool(name: str, arguments: dict[str, Any]) -> Any:
    mapping = {
        "open_image_in_clay": ("open_image", arguments),
        "get_canvas_preview": ("get_canvas_snapshot", {}),
        "beautify_image": ("beautify_image", arguments),
        "content_aware_fill": ("content_aware_fill", arguments),
        "resize_image": ("resize_image", arguments),
        "apply_gegl_filter": (
            "apply_gegl_operation",
            {
                "operation": arguments.get("operation"),
                "properties": arguments.get("properties") or {},
            },
        ),
        "edit_layer_or_selection": ("manage_layers_and_selections", arguments),
        "add_text_overlay": (
            "execute_python",
            {
                "code": (
                    "text = %r\n"
                    "if image is not None:\n"
                    "    layer = Clay.TextLayer.new(image, text, Clay.context_get_font(), 48.0, 72.0)\n"
                    "    image.insert_layer(layer, None, 0)\n"
                ) % arguments.get("text", "Clay")
            },
        ),
        "export_image": ("export_image", arguments),
        "search_clay_app_store": ("store_search", arguments),
        "install_clay_plugin": ("store_search", {"query": arguments.get("entry_id", "")}),
        "run_pdb_procedure": ("call_pdb", arguments),
        "execute_clay_python": ("execute_python", arguments),
        "get_session_state": ("get_session_state", {}),
    }
    if name not in mapping:
        raise RuntimeError("Unknown tool: %s" % name)
    method, params = mapping[name]
    result = clay_call(method, params)
    if name == "open_image_in_clay":
        try:
            import ctypes, time as _t
            user32 = ctypes.windll.user32
            hwnd = user32.FindWindowW(None, "Clay Image Beautification Program")
            if hwnd:
                user32.ShowWindow(hwnd, 9)
                user32.SetForegroundWindow(hwnd)
                _t.sleep(0.25)
                user32.keybd_event(0x31, 0, 0, 0)
                user32.keybd_event(0x31, 0, 2, 0)
        except Exception:
            pass
    return result


def send(msg: dict[str, Any]) -> None:
    body = json.dumps(msg).encode("utf-8")
    sys.stdout.buffer.write(
        b"Content-Length: %d\r\n\r\n" % len(body) + body
    )
    sys.stdout.buffer.flush()


def read_msg() -> dict[str, Any] | None:
    headers: dict[str, str] = {}
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        key, value = line.decode("utf-8").split(":", 1)
        headers[key.strip().lower()] = value.strip()
    length = int(headers.get("content-length", "0"))
    body = sys.stdin.buffer.read(length)
    return json.loads(body.decode("utf-8"))


def handle(msg: dict[str, Any]) -> None:
    method = msg.get("method")
    req_id = msg.get("id")
    params = msg.get("params") or {}

    if method == "initialize":
        send({
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": params.get("protocolVersion", "2024-11-05"),
                "capabilities": {"tools": {}, "resources": {}},
                "serverInfo": {"name": "ClayDesigner", "version": "1.0.0"},
            },
        })
        return

    if method == "notifications/initialized":
        return

    if method == "tools/list":
        send({"jsonrpc": "2.0", "id": req_id, "result": {"tools": TOOLS}})
        return

    if method == "resources/list":
        send({
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "resources": [
                    {"uri": "clay://session/images", "name": "Open images"},
                    {"uri": "clay://store/catalog", "name": "App Store catalog"},
                    {"uri": "clay://pdb/procedures", "name": "Hint: use run_pdb_procedure"},
                ]
            },
        })
        return

    if method == "resources/read":
        uri = params.get("uri")
        if uri == "clay://session/images":
            text = json.dumps(clay_call("get_session_state"), indent=2)
        elif uri == "clay://store/catalog":
            text = json.dumps(clay_call("store_search", {"query": ""}), indent=2)
        else:
            text = uri
        send({
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "contents": [{"uri": uri, "mimeType": "application/json", "text": text}]
            },
        })
        return

    if method == "tools/call":
        name = params.get("name")
        arguments = params.get("arguments") or {}
        try:
            result = dispatch_tool(name, arguments)
            send({
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": json.dumps(result, indent=2)}]
                },
            })
        except Exception as exc:
            send({
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": str(exc)}],
                    "isError": True,
                },
            })
        return

    if req_id is not None:
        send({
            "jsonrpc": "2.0",
            "id": req_id,
            "error": {"code": -32601, "message": "Method not found: %s" % method},
        })


def main() -> None:
    while True:
        msg = read_msg()
        if msg is None:
            break
        handle(msg)


if __name__ == "__main__":
    main()
