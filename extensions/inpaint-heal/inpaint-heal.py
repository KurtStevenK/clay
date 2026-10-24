#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
import sys

def _(message):
    return GLib.dgettext(None, message)

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
        return False
    try:
        filt = Clay.DrawableFilter.new(drawable, op_name, "")
        cfg = filt.get_config()
        if props:
            for key, value in props.items():
                try:
                    cfg.set_property(key, value)
                except Exception:
                    pass
        drawable.merge_filter(filt)
        return True
    except GLib.GError:
        return False


def content_aware_fill(drawable, radius, passes):
    """Approximate content-aware fill inside the current selection."""
    for _ in range(max(1, int(passes))):
        if gegl_op_available("gegl:inpaint") and apply_gegl(drawable, "gegl:inpaint", None):
            return
        if apply_gegl(drawable, "gegl:mean-curvature-blur",
                      {"iterations": max(8, int(radius))}):
            pass
        elif apply_gegl(drawable, "gegl:gaussian-blur",
                        {"std-dev-x": float(radius),
                         "std-dev-y": float(radius)}):
            pass
        apply_gegl(drawable, "gegl:noise-reduction", {"iterations": 2})


def run_fill(procedure, run_mode, image, drawables, config, run_data):
    if len(drawables) != 1:
        msg = _("Procedure '{}' only works with one drawable.").format(procedure.get_name())
        error = GLib.Error.new_literal(Clay.PlugIn.error_quark(), msg, 0)
        return procedure.new_return_values(Clay.PDBStatusType.CALLING_ERROR, error)

    drawable = drawables[0]

    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi
        ClayUi.init('inpaint-heal')
        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        dialog.destroy()

    radius = int(config.get_property('sampling-radius'))
    passes = int(config.get_property('refinement-passes'))
    heal_alpha = bool(config.get_property('heal-transparency'))

    Clay.context_push()
    image.undo_group_start()

    if Clay.Selection.is_empty(image) and heal_alpha:
        Clay.Selection.none(image)

    if not Clay.Selection.is_empty(image) and radius > 0:
        Clay.Selection.grow(image, radius)
        Clay.Selection.shrink(image, max(0, radius - 1))

    content_aware_fill(drawable, radius, passes)

    Clay.displays_flush()
    image.undo_group_end()
    Clay.context_pop()
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class InpaintHeal (Clay.PlugIn):
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return ['plug-in-inpaint-heal', 'plug-in-heal-transparency']

    def do_create_procedure(self, name):
        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            run_fill, None)
        procedure.set_image_types("*")
        procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.DRAWABLE)
        procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
        procedure.add_int_argument("sampling-radius", _("Sampling _radius"),
                                   _("How far around the hole to sample"),
                                   4, 128, 24, GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("refinement-passes", _("Refinement _passes"),
                                   _("How many diffusion passes to run"),
                                   1, 8, 2, GObject.ParamFlags.READWRITE)
        procedure.add_boolean_argument("heal-transparency", _("Heal _transparency"),
                                       _("Fill fully transparent pixels when nothing is selected"),
                                       True, GObject.ParamFlags.READWRITE)

        if name == 'plug-in-inpaint-heal':
            procedure.set_menu_label(_("Content-Aware _Fill…"))
            procedure.add_menu_path('<Image>/Filters/Enhance')
            procedure.set_documentation(
                _("Fill the selection from surrounding texture"),
                _("Content-aware fill using diffusion when GEGL inpaint is unavailable."),
                name)
        else:
            procedure.set_menu_label(_("_Heal Transparency…"))
            procedure.add_menu_path('<Image>/Filters/Enhance')
            procedure.set_documentation(
                _("Heal transparent holes in the layer"),
                _("Fill transparent regions from nearby opaque pixels."),
                name)

        return procedure


Clay.main(InpaintHeal.__gtype__, sys.argv)
