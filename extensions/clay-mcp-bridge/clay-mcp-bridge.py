#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import json
import os
import random
import sys
import traceback
import tempfile

def _(message):
    return GLib.dgettext(None, message)

DEFAULT_PORT = 9877
_service = None
_log_lines = []
_CONFIG_BASENAME = "mcp-bridge.json"


def _mcp_config_path():
    if sys.platform == "win32":
        base = os.path.join(os.environ.get("APPDATA", ""), "Clay", "3.36")
    else:
        base = os.path.join(os.environ.get("XDG_CONFIG_HOME") or os.path.expanduser("~/.config"), "clay")
    return os.path.join(base, _CONFIG_BASENAME)


def _read_mcp_config():
    try:
        with open(_mcp_config_path(), encoding="utf-8-sig") as fh:
            return json.load(fh)
    except Exception:
        return {}


def _write_mcp_config(data):
    path = _mcp_config_path()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2)


def _mcp_autostart_enabled():
    env = os.environ.get("CLAY_MCP_AUTOSTART", "").strip().lower()
    if env in ("1", "true", "yes", "on"):
        return True
    if env in ("0", "false", "no", "off"):
        return False
    return bool(_read_mcp_config().get("autostart", False))


def _set_mcp_autostart(enabled):
    data = _read_mcp_config()
    data["autostart"] = bool(enabled)
    _write_mcp_config(data)


def _autostart_menu_label(enabled):
    if enabled:
        return _("✓ Autostart MCP Server on Launch")
    return _("Autostart MCP Server on Launch")


def log(message):
    _log_lines.append(str(message))
    if len(_log_lines) > 200:
        del _log_lines[0]
    print("[clay-mcp]", message, file=sys.stderr)


_gegl_initialized = False


def _ensure_gegl():
    global _gegl_initialized
    if _gegl_initialized:
        return
    try:
        Gegl.init(None)
    except (TypeError, AttributeError):
        pass
    _gegl_initialized = True


def gegl_op_available(op_name):
    _ensure_gegl()
    try:
        return Gegl.has_operation(op_name)
    except (AttributeError, TypeError, GLib.GError):
        return False
    except Exception:
        return False


def apply_gegl(drawable, op_name, props=None):
    if not gegl_op_available(op_name):
        raise RuntimeError('GEGL operation "%s" is not installed' % op_name)
    filt = Clay.DrawableFilter.new(drawable, op_name, "")
    cfg = filt.get_config()
    if props:
        for key, value in props.items():
            try:
                cfg.set_property(key, value)
            except Exception:
                pass
    drawable.merge_filter(filt)


def _pixel_rgba(drawable, x, y):
    r, g, b, a = drawable.get_pixel(x, y).get_rgba()
    return r, g, b, a


def _near_white_rgba(r, g, b, a, threshold):
    return (r >= threshold and g >= threshold and b >= threshold and a > 0.5)


def _find_white_box_bbox(drawable, w, h, threshold=0.92):
    """Quick scan for the white sticker on the table — no flood fill."""
    if w >= 400:
        x0, y0, x1, y1 = int(w * 0.26), int(h * 0.40), int(w * 0.55), int(h * 0.70)
        step, min_hits = 3, 12
    else:
        x0, y0, x1, y1 = 0, 0, w, h
        step, min_hits = 2, 6
    minx, miny, maxx, maxy = x1, y1, x0, y0
    hits = 0
    for y in range(y0, y1, step):
        for x in range(x0, x1, step):
            r, g, b, a = _pixel_rgba(drawable, x, y)
            if not _near_white_rgba(r, g, b, a, threshold):
                continue
            hits += 1
            minx, miny = min(minx, x), min(miny, y)
            maxx, maxy = max(maxx, x), max(maxy, y)
    if hits < min_hits:
        return None
    pad = step * 2
    rx = max(0, minx - pad)
    ry = max(0, miny - pad)
    rw = min(w - rx, maxx - minx + 1 + 2 * pad)
    rh = min(h - ry, maxy - miny + 1 + 2 * pad)
    if rw < 6 or rh < 6 or rw > 300 or rh > 300:
        return None
    return rx, ry, rw, rh


def _looks_like_wood(r, g, b):
    return r > 0.42 and g > 0.32 and b < 0.52 and r > b * 1.04


def _looks_like_wall(r, g, b):
    return b > 0.62 and g > 0.58 and (b - r) > -0.05


def _rect_on_wall(drawable, rx, ry, rw, rh):
    wall, wood = 0, 0
    step_x = max(2, rw // 6)
    step_y = max(2, rh // 6)
    for y in range(ry, ry + rh, step_y):
        for x in range(rx, rx + rw, step_x):
            r, g, b, a = _pixel_rgba(drawable, x, y)
            if _looks_like_wood(r, g, b):
                wood += 1
            elif _looks_like_wall(r, g, b):
                wall += 1
    return wood == 0 and wall >= 6


def _is_clear_table_pixel(r, g, b, a):
    if _near_white_rgba(r, g, b, a, 0.92):
        return False
    if _looks_like_wall(r, g, b):
        return False
    if r > 0.58 and g < 0.42 and b < 0.45:
        return False
    return _looks_like_wood(r, g, b) or (r > 0.32 and g > 0.26 and b < 0.55)


def _rect_on_table(drawable, rx, ry, rw, rh):
    ok_px = 0
    step_x = max(2, rw // 5)
    step_y = max(2, rh // 5)
    for y in range(ry, ry + rh, step_y):
        for x in range(rx, rx + rw, step_x):
            r, g, b, a = _pixel_rgba(drawable, x, y)
            if _is_clear_table_pixel(r, g, b, a):
                ok_px += 1
    return ok_px >= 4


def _random_table_rect(drawable, w, h):
    """Small white demo box on bare wood (not on berries)."""
    for _ in range(50):
        rw = random.randint(40, 78)
        rh = random.randint(52, 98)
        rx = random.randint(int(w * 0.36), max(int(w * 0.36), int(w * 0.50) - rw))
        ry = random.randint(int(h * 0.43), max(int(h * 0.43), int(h * 0.53) - rh))
        if _rect_on_table(drawable, rx, ry, rw, rh):
            return rx, ry, rw, rh
    fallback = (502, 405, 54, 86)
    log("heal: table white-box fallback %s" % list(fallback))
    return fallback


def _random_wall_rect(drawable, w, h):
    """Random rectangle strictly on the blue wall (never the table/berries)."""
    y_max = int(h * 0.36)
    for _ in range(40):
        rw = random.randint(72, 118)
        rh = random.randint(52, 86)
        rx = random.randint(int(w * 0.12), max(int(w * 0.12), int(w * 0.72) - rw))
        ry = random.randint(int(h * 0.10), max(int(h * 0.10), y_max - rh))
        if _rect_on_wall(drawable, rx, ry, rw, rh):
            return rx, ry, rw, rh
    raise RuntimeError("Could not find a clean wall area for demo patch.")


def _fill_selection_white(image, drawable):
    white = Gegl.Color()
    white.set_rgba(1.0, 1.0, 1.0, 1.0)
    Clay.context_set_foreground(white)
    drawable.edit_fill(Clay.FillType.FOREGROUND)


def _selection_on_table(drawable, x1, y1, x2, y2):
    """White sticker / wood table — not the blue wall."""
    cy = (y1 + y2) // 2
    h = drawable.get_height()
    if cy > int(h * 0.40):
        return True
    wood = wall = white = 0
    step = max(2, (x2 - x1) // 6)
    for x in range(x1, x2, step):
        r, g, b, a = _pixel_rgba(drawable, x, cy)
        if _near_white_rgba(r, g, b, a, 0.88):
            white += 1
        elif _looks_like_wood(r, g, b):
            wood += 1
        elif _looks_like_wall(r, g, b):
            wall += 1
    return white + wood >= wall


def _patch_source_score(drawable, sx, sy, rw, rh, want_wood):
    score = 0
    step = max(2, min(rw, rh) // 8)
    for y in range(sy, sy + rh, step):
        for x in range(sx, sx + rw, step):
            r, g, b, a = _pixel_rgba(drawable, x, y)
            if _near_white_rgba(r, g, b, a, 0.9):
                score += 80
            elif want_wood:
                if _looks_like_wall(r, g, b):
                    score += 100
                elif _near_white_rgba(r, g, b, a, 0.95):
                    score += 50
            else:
                if _looks_like_wood(r, g, b):
                    score += 50
                elif not _looks_like_wall(r, g, b):
                    score += 15
    return score


def _patch_fill_selection(drawable, image):
    """Copy texture from beside the hole (wood on table, wall on wall)."""
    ok, has_sel, x1, y1, x2, y2 = Clay.Selection.bounds(image)
    if not ok or not has_sel:
        return False
    w, h = drawable.get_width(), drawable.get_height()
    x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
    rw, rh = max(1, x2 - x1), max(1, y2 - y1)
    on_table = _selection_on_table(drawable, x1, y1, x2, y2)
    gap = 8
    candidates = []
    for mult in (1, 2, 3):
        ox = x1 - mult * (rw + gap)
        if ox >= 0:
            candidates.append((ox, y1, "left%d" % mult))
    if x2 + gap + rw <= w:
        candidates.append((x2 + gap, y1, "right"))
    if not on_table and y1 - rh - gap >= 0:
        candidates.append((x1, y1 - rh - gap, "above"))
    if not candidates:
        return False
    scored = lambda c: _patch_source_score(drawable, c[0], c[1], rw, rh, on_table)
    ranked = sorted(candidates, key=scored)
    primary = ranked[0]
    if not on_table and scored(primary) > 12:
        log("heal: no clean wall patch (score %d)" % scored(primary))
        return False
    sx, sy, side = primary
    dest = (x1, y1, rw, rh)
    image.select_rectangle(Clay.ChannelOps.REPLACE, sx, sy, rw, rh)
    if not Clay.edit_copy([drawable]):
        log("heal: edit-copy failed")
        return False
    image.select_rectangle(Clay.ChannelOps.REPLACE, *dest)
    Clay.edit_paste(drawable, True)
    floating = image.get_floating_sel()
    if floating is not None:
        Clay.floating_sel_anchor(floating)
    kind = "wood" if on_table else "wall"
    log("heal: undoable %s patch from %s (%dx%d)" % (kind, side, rw, rh))
    return True


_RESYNTH_NAMES = (
    "python-fu-heal-selection",
    "plug-in-heal-selection",
    "python-fu-resynth-heal-selection",
    "plug-in-resynthesizer",
    "plug-in-resynth",
)


def _find_resynthesizer_procedure():
    pdb = Clay.get_pdb()
    for name in _RESYNTH_NAMES:
        if pdb.lookup_procedure(name) is not None:
            return name
    return None


def _try_resynthesizer_fill(image, drawable):
    name = _find_resynthesizer_procedure()
    if not name:
        return False
    proc = Clay.get_pdb().lookup_procedure(name)
    try:
        run_image_procedure(proc, image, drawable)
        log("heal: %s" % name)
        return True
    except Exception as exc:
        log("heal: %s failed: %s" % (name, exc))
        return False


def _try_inpaint_heal_fill(image, drawable, **kwargs):
    """Inhaltssensitives Füllen (GEGL blur/inpaint), not bootchk Resynthesizer."""
    proc = Clay.get_pdb().lookup_procedure("plug-in-inpaint-heal")
    if proc is None:
        return False
    try:
        run_image_procedure(proc, image, drawable, **kwargs)
        log("heal: plug-in-inpaint-heal")
        return True
    except Exception as exc:
        log("heal: plug-in-inpaint-heal failed: %s" % exc)
        return False


def _ensure_white_box_selection(image, drawable):
    """Use magic-wand-sized selection, not the whole image."""
    w, h = image.get_width(), image.get_height()
    area = _selection_pixel_count(image)
    if not Clay.Selection.is_empty(image) and area <= int(0.15 * w * h):
        log("heal: keeping your selection (%d px)" % area)
        return True
    if not Clay.Selection.is_empty(image) and area > int(0.15 * w * h):
        log("heal: ignoring full-image selection")
    bbox = _find_white_box_bbox(drawable, w, h)
    if not bbox:
        return False
    image.select_rectangle(Clay.ChannelOps.REPLACE, *bbox)
    log("heal: selected white box %dx%d at %d,%d" % (bbox[2], bbox[3], bbox[0], bbox[1]))
    return True


BEAUTIFY = {
    'auto_beautify': [
        ('gegl:shadows-highlights', {'shadows': 0.15, 'highlights': -0.08}),
        ('gegl:saturation', {'scale': 1.08}),
        ('gegl:unsharp-mask', {'std-dev': 0.8, 'scale': 0.35}),
    ],
    'skin_smooth_portrait': [
        ('gegl:noise-reduction', {'iterations': 3}),
        ('gegl:unsharp-mask', {'std-dev': 0.5, 'scale': 0.2}),
    ],
    'cinematic_grade': [
        ('gegl:saturation', {'scale': 0.92}),
        ('gegl:vignette', {'radius': 1.2, 'softness': 0.8}),
    ],
    'hdr_pop': [
        ('gegl:shadows-highlights', {'shadows': 0.25, 'highlights': -0.12}),
        ('gegl:unsharp-mask', {'std-dev': 2.0, 'scale': 0.45}),
    ],
    'denoise_and_sharpen': [
        ('gegl:noise-reduction', {'iterations': 4}),
        ('gegl:unsharp-mask', {'std-dev': 1.0, 'scale': 0.5}),
    ],
}


def active_image():
    images = Clay.get_images()
    if not images:
        return None
    return images[0]


def active_drawable(image=None):
    image = image or active_image()
    if image is None:
        return None, None
    drawables = image.get_selected_drawables()
    if not drawables:
        layers = image.get_layers()
        drawables = layers[:1] if layers else []
    return image, (drawables[0] if drawables else None)


def _config_props(cfg):
    return {pspec.name for pspec in cfg.list_properties()}


def run_image_procedure(proc, image, drawable, **kwargs):
    """Run a plug-in ImageProcedure with image + drawable (required for PDB)."""
    cfg = proc.create_config()
    props = _config_props(cfg)
    if "run-mode" in props:
        cfg.set_property("run-mode", Clay.RunMode.NONINTERACTIVE)
    if "image" in props:
        cfg.set_property("image", image)
    if "drawables" in props:
        if hasattr(cfg, "set_core_object_array"):
            cfg.set_core_object_array("drawables", [drawable])
        else:
            cfg.set_property("drawables", _edit_drawables_arg(drawable))
    elif "num-drawables" in props and "drawable" in props:
        cfg.set_property("num-drawables", 1)
        cfg.set_property("drawable", drawable)
    elif "drawable" in props:
        cfg.set_property("drawable", drawable)
    for key, value in kwargs.items():
        pname = key if key in props else key.replace("_", "-")
        if pname in props:
            cfg.set_property(pname, value)
    return proc.run(cfg)


def run_pdb_procedure(proc, **kwargs):
    """Run an internal PDB procedure (no run-mode; not a plug-in ImageProcedure)."""
    cfg = proc.create_config()
    props = _config_props(cfg)
    for key, value in kwargs.items():
        pname = key if key in props else key.replace("_", "-")
        if pname in props:
            cfg.set_property(pname, value)
    return proc.run(cfg)


def _pdb(name, **kwargs):
    pdb = Clay.get_pdb()
    proc = pdb.lookup_procedure(name)
    if proc is None:
        raise RuntimeError("PDB missing: %s" % name)
    return run_pdb_procedure(proc, **kwargs)


def _edit_drawables_arg(drawable):
    try:
        return Clay.ObjectArray.new(Clay.Drawable, drawable, False)
    except AttributeError:
        return [drawable]


def _selection_pixel_count(image):
    ok, has_sel, x1, y1, x2, y2 = Clay.Selection.bounds(image)
    if not ok or not has_sel:
        return 0
    return max(0, int(x2 - x1)) * max(0, int(y2 - y1))


def image_info(image):
    layers = image.get_layers() if image else []
    return {
        "id": image.get_id() if hasattr(image, "get_id") else None,
        "width": image.get_width(),
        "height": image.get_height(),
        "name": image.get_name() if hasattr(image, "get_name") else "",
        "layers": len(layers),
    }


def handle_command(cmd, params):
    params = params or {}
    if cmd == "ping":
        version = getattr(Clay, "VERSION", None) or "3.0"
        return {"ok": True, "app": "Clay", "version": str(version)}

    if cmd == "get_session_state":
        images = Clay.get_images() or []
        return {
            "image_count": len(images),
            "images": [image_info(im) for im in images],
            "log": _log_lines[-20:],
        }

    if cmd == "open_image":
        path = params.get("path")
        file = Gio.File.new_for_path(path)
        image = Clay.file_load(Clay.RunMode.NONINTERACTIVE, file)
        Clay.Display.new(image)
        Clay.displays_flush()
        return {"opened": path, "image": image_info(image)}

    if cmd == "create_image":
        w = int(params.get("width", 1920))
        h = int(params.get("height", 1080))
        image = Clay.Image.new(w, h, Clay.ImageBaseType.RGB)
        layer = Clay.Layer.new(image, "Background", w, h,
                               Clay.ImageType.RGB_IMAGE, 100.0,
                               Clay.LayerMode.NORMAL)
        image.insert_layer(layer, None, 0)
        Clay.Display.new(image)
        Clay.displays_flush()
        return {"image": image_info(image)}

    if cmd in ("save_image", "export_image"):
        image, _ = active_drawable()
        path = params.get("path")
        if image is None or not path:
            raise RuntimeError("Need an open image and a path")
        Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(path), None)
        return {"saved": path}

    if cmd == "get_canvas_snapshot":
        image, _ = active_drawable()
        if image is None:
            raise RuntimeError("No image open")
        fd, path = tempfile.mkstemp(suffix=".png", prefix="clay-snap-")
        os.close(fd)
        # Save a duplicate so the working image keeps its undo stack and name.
        dup = image.duplicate()
        Clay.file_save(Clay.RunMode.NONINTERACTIVE, dup, Gio.File.new_for_path(path), None)
        dup.delete()
        return {"path": path}

    if cmd == "beautify_image":
        image, drawable = active_drawable()
        if drawable is None:
            raise RuntimeError("No drawable")
        preset = params.get("preset", "auto_beautify")
        image.undo_group_start()
        for op_name, props in BEAUTIFY.get(preset, BEAUTIFY["auto_beautify"]):
            try:
                apply_gegl(drawable, op_name, props)
            except Exception:
                pass
        image.undo_group_end()
        Clay.displays_flush()
        return {"preset": preset, "image": image_info(image)}

    if cmd == "apply_gegl_operation":
        image, drawable = active_drawable()
        if drawable is None:
            raise RuntimeError("No drawable")
        op_name = params.get("operation")
        props = params.get("properties") or {}
        image.undo_group_start()
        apply_gegl(drawable, op_name, props)
        image.undo_group_end()
        Clay.displays_flush()
        return {"operation": op_name}

    if cmd == "content_aware_fill":
        image, drawable = active_drawable()
        if drawable is None:
            raise RuntimeError("No drawable")
        w, h = image.get_width(), image.get_height()
        image.undo_group_start()
        patch_rect = None
        if params.get("make_random_white_patch"):
            Clay.Selection.none(image)
            surface = (params.get("white_box_surface") or "table").lower()
            if surface == "wall":
                patch_rect = list(_random_wall_rect(drawable, w, h))
            else:
                patch_rect = list(_random_table_rect(drawable, w, h))
            image.select_rectangle(Clay.ChannelOps.REPLACE, *patch_rect)
            _fill_selection_white(image, drawable)
            log("heal: white box on %s %s" % (surface, patch_rect))
        rect = params.get("rect")
        if rect and len(rect) == 4 and params.get("replace_selection"):
            image.select_rectangle(Clay.ChannelOps.REPLACE,
                                   int(rect[0]), int(rect[1]),
                                   int(rect[2]), int(rect[3]))
        elif not params.get("make_random_white_patch"):
            if params.get("auto_select_white", True):
                if not _ensure_white_box_selection(image, drawable):
                    raise RuntimeError(
                        "Could not find white area. Use make_random_white_patch, "
                        "magic-wand, or pass rect."
                    )
            elif Clay.Selection.is_empty(image):
                raise RuntimeError("No selection.")

        quality = (params.get("quality") or "high").lower()
        qmap = {"high": (32, 4), "medium": (24, 2), "low": (12, 1)}
        radius, passes = qmap.get(quality, qmap["high"])
        if params.get("sampling_radius") is not None:
            radius = int(params.get("sampling_radius"))
        if params.get("refinement_passes") is not None:
            passes = int(params.get("refinement_passes"))

        method = None
        if _try_resynthesizer_fill(image, drawable):
            method = "resynthesizer"
        elif _try_inpaint_heal_fill(
            image, drawable,
            sampling_radius=radius,
            refinement_passes=passes,
            heal_transparency=False,
        ):
            method = "inpaint-heal"
        elif _patch_fill_selection(drawable, image):
            method = "texture_patch"
        else:
            image.undo_group_end()
            try:
                image.undo()
            except Exception:
                pass
            raise RuntimeError(
                "Fill failed. Install Resynthesizer "
                "(https://github.com/bootchk/resynthesizer) or move the "
                "selection to open wood/wall with texture beside it."
            )
        image.undo_group_end()
        Clay.displays_flush()
        out = {"filled": True, "method": method, "undo_group": True}
        if patch_rect:
            out["patch_rect"] = patch_rect
        out["resynthesizer_installed"] = _find_resynthesizer_procedure() is not None
        return out

    if cmd == "resize_image":
        image, _ = active_drawable()
        if image is None:
            raise RuntimeError("No image open")
        ow, oh = image.get_width(), image.get_height()
        scale = params.get("scale_percent")
        if scale is not None:
            factor = float(scale) / 100.0
            new_w = max(1, int(round(ow * factor)))
            new_h = max(1, int(round(oh * factor)))
        else:
            new_w = int(params.get("width", 150))
            new_h = int(params.get("height", 150))
        dpi = params.get("dpi")
        pdb = Clay.get_pdb()
        image.undo_group_start()
        scale = pdb.lookup_procedure("clay-image-scale")
        if scale is None:
            raise RuntimeError("clay-image-scale not available")
        run_pdb_procedure(scale, image=image, new_width=new_w, new_height=new_h)
        if dpi is not None:
            res = pdb.lookup_procedure("clay-image-set-resolution")
            if res is None:
                raise RuntimeError("clay-image-set-resolution not available")
            run_pdb_procedure(res, image=image, xresolution=float(dpi), yresolution=float(dpi))
        image.undo_group_end()
        Clay.displays_flush()
        out = {
            "width": image.get_width(),
            "height": image.get_height(),
            "previous_width": ow,
            "previous_height": oh,
        }
        if dpi is not None:
            out["dpi"] = dpi
        return out

    if cmd == "manage_layers_and_selections":
        image, drawable = active_drawable()
        action = params.get("action")
        if action == "select_rectangle" and image:
            r = params.get("rect") or [0, 0, 64, 64]
            image.select_rectangle(Clay.ChannelOps.REPLACE, *r)
        elif action == "none" and image:
            Clay.Selection.none(image)
        Clay.displays_flush()
        return {"action": action}

    if cmd == "call_pdb":
        name = params.get("name")
        pdb = Clay.get_pdb()
        proc = pdb.lookup_procedure(name)
        if proc is None:
            raise RuntimeError("Unknown procedure %s" % name)
        image, drawable = active_drawable()
        if image is None or drawable is None:
            raise RuntimeError("No image/drawable for PDB call")
        kwargs = dict(params.get("arguments") or {})
        run_image_procedure(proc, image, drawable, **kwargs)
        Clay.displays_flush()
        return {"ran": name}

    if cmd == "execute_python":
        image, drawable = active_drawable()
        code = params.get("code") or ""
        exec(code, {"Clay": Clay, "Gegl": Gegl, "image": image, "drawable": drawable})
        Clay.displays_flush()
        return {"executed": True}

    if cmd == "store_search":
        return {"note": "Use the Clay App Store UI or catalog JSON",
                "query": params.get("query")}

    raise RuntimeError("Unknown command: %s" % cmd)


def read_line(connection):
    buf = b""
    istream = connection.get_input_stream()
    while True:
        chunk = istream.read_bytes(1, None)
        if chunk.get_size() == 0:
            return None
        byte = chunk.get_data()
        if byte == b"\n":
            return buf.decode("utf-8")
        buf += byte
        if len(buf) > 1024 * 1024:
            return buf.decode("utf-8")


def on_incoming(service, connection, source):
    try:
        line = read_line(connection)
        if not line:
            connection.close(None)
            return False
        request = json.loads(line)
        cmd = request.get("method") or request.get("command")
        params = request.get("params") or {}
        req_id = request.get("id")
        try:
            Clay.context_push()
            try:
                result = handle_command(cmd, params)
            finally:
                Clay.context_pop()
            reply = {"jsonrpc": "2.0", "id": req_id, "result": result}
        except Exception as exc:
            log(traceback.format_exc())
            reply = {"jsonrpc": "2.0", "id": req_id,
                     "error": {"code": -32000, "message": str(exc)}}
        payload = (json.dumps(reply) + "\n").encode("utf-8")
        connection.get_output_stream().write_all(payload, None)
    except Exception:
        log(traceback.format_exc())
    finally:
        try:
            connection.close(None)
        except Exception:
            pass
    return False


def start_service(port):
    global _service
    if _service is not None:
        return port
    service = Gio.SocketService.new()
    service.add_inet_port(port, None)
    service.connect("incoming", on_incoming)
    service.start()
    _service = service
    log("Listening on 127.0.0.1:%s" % port)
    return port


def run_mcp_daemon(procedure, config, data):
    """Zero-arg persistent extension: Clay starts this on launch.

    Stay alive with a GLib loop only when autostart is enabled, otherwise
    the plug-in process would exit and close the socket.
    """
    try:
        procedure.persistent_ready()
    except Exception:
        pass
    if not _mcp_autostart_enabled():
        return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())
    try:
        start_service(DEFAULT_PORT)
        log("MCP autostart: listening on 127.0.0.1:%s" % DEFAULT_PORT)
    except Exception as exc:
        log("MCP autostart failed: %s" % exc)
        return procedure.new_return_values(Clay.PDBStatusType.EXECUTION_ERROR, GLib.Error())
    try:
        Clay.get_plug_in().persistent_enable()
    except Exception:
        pass
    loop = GLib.MainLoop()
    loop.run()
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


def run_bridge(procedure, run_mode, image, drawables, config, run_data):
    try:
        port = int(config.get_property("port"))
    except Exception:
        port = DEFAULT_PORT

    start_service(port)

    if run_mode == Clay.RunMode.NONINTERACTIVE:
        return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())

    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('Gtk', '3.0')
        gi.require_version('ClayUi', '3.0')
        from gi.repository import Gtk
        from gi.repository import ClayUi
        ClayUi.init("clay-mcp-bridge")
        dialog = Gtk.Dialog(title=_("Clay AI Agent Bridge (MCP)"),
                            flags=0)
        dialog.add_button(_("_Stop"), Gtk.ResponseType.CLOSE)
        box = dialog.get_content_area()
        label = Gtk.Label(label=_("Listening on 127.0.0.1:%d\n"
                                  "Connect Cursor via tools/clay-mcp/server.py") % port)
        label.set_margin_top(12)
        label.set_margin_bottom(12)
        label.set_margin_start(12)
        label.set_margin_end(12)
        box.add(label)
        log_view = Gtk.TextView()
        log_view.set_editable(False)
        log_view.get_buffer().set_text("\n".join(_log_lines) or "Ready.")
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_min_content_height(180)
        scrolled.add(log_view)
        box.add(scrolled)
        box.show_all()
        dialog.run()
        dialog.destroy()
        return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())

    loop = GLib.MainLoop()
    loop.run()
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


def run_toggle_autostart(procedure, run_mode, image, drawables, config, run_data):
    enabled = not _mcp_autostart_enabled()
    _set_mcp_autostart(enabled)
    procedure.set_menu_label(_autostart_menu_label(enabled))
    if run_mode == Clay.RunMode.INTERACTIVE:
        if enabled:
            Clay.message(_("MCP autostart is on. The server starts with Clay; "
                           "the checkmark is at the start of this menu item."))
        else:
            Clay.message(_("MCP autostart is off. Use Tools → AI Agent Bridge to start it."))
    Clay.displays_flush()
    if enabled:
        try:
            start_service(DEFAULT_PORT)
            log("MCP bridge started from autostart toggle")
            try:
                Clay.get_plug_in().persistent_enable()
            except Exception:
                pass
            GLib.MainLoop().run()
        except Exception as exc:
            log("MCP start failed: %s" % exc)
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class ClayMcpBridge (Clay.PlugIn):
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return [
            "plug-in-clay-mcp-bridge-daemon",
            "plug-in-clay-mcp-bridge-start",
            "plug-in-clay-mcp-bridge-toggle-autostart",
        ]

    def do_create_procedure(self, name):
        if name == "plug-in-clay-mcp-bridge-daemon":
            procedure = Clay.Procedure.new(self, name,
                                           Clay.PDBProcType.PERSISTENT,
                                           run_mcp_daemon, None)
            procedure.set_documentation(
                _("Background MCP listener (started with Clay when autostart is on)"),
                _("JSON-RPC on 127.0.0.1:9877"),
                name)
            procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
            return procedure

        if name == "plug-in-clay-mcp-bridge-toggle-autostart":
            procedure = Clay.ImageProcedure.new(self, name,
                                                Clay.PDBProcType.PLUGIN,
                                                run_toggle_autostart, None)
            procedure.set_image_types("*")
            procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.ALWAYS)
            procedure.set_menu_label(_autostart_menu_label(_mcp_autostart_enabled()))
            procedure.add_menu_path("<Image>/Tools")
            procedure.set_documentation(
                _("Toggle MCP bridge autostart when Clay launches"),
                _("Saved in %s") % _mcp_config_path(),
                name)
            procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
            return procedure

        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            run_bridge, None)
        procedure.set_image_types("*")
        procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.ALWAYS)
        procedure.set_menu_label(_("AI Agent Bridge (_MCP Server)…"))
        procedure.add_menu_path("<Image>/Tools")
        procedure.set_documentation(
            _("Start the Clay MCP socket so AI agents can control Clay"),
            _("JSON-RPC bridge on 127.0.0.1:9877"),
            name)
        procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
        procedure.add_int_argument("port", _("_Port"),
                                   _("TCP port for the local AI bridge"),
                                   1024, 65535, DEFAULT_PORT,
                                   GObject.ParamFlags.READWRITE)
        return procedure


Clay.main(ClayMcpBridge.__gtype__, sys.argv)
