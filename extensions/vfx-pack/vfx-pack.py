#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
from gi.repository import GObject
from gi.repository import GLib
import sys

def _(message):
    return GLib.dgettext(None, message)


def apply_gegl(drawable, op_name, props=None):
    filt = Clay.DrawableFilter.new(drawable, op_name, "")
    cfg = filt.get_config()
    if props:
        for key, value in props.items():
            try:
                cfg.set_property(key, value)
            except Exception:
                pass
    drawable.merge_filter(filt)


LOOKS = {
    'plug-in-clay-chromatic': [
        ('gegl:lens-distortion', {'aberration': 8.0}),
        ('gegl:unsharp-mask', {'std-dev': 0.4, 'scale': 0.2}),
    ],
    'plug-in-clay-vhs': [
        ('gegl:noise-rgb', {'red': 0.04, 'green': 0.04, 'blue': 0.06, 'independent': True}),
        ('gegl:gaussian-blur', {'std-dev-x': 0.6, 'std-dev-y': 0.1}),
        ('gegl:saturation', {'scale': 1.2}),
    ],
    'plug-in-clay-halftone': [
        ('gegl:newsprint', {'period': 6.0}),
    ],
    'plug-in-clay-bloom': [
        ('gegl:bloom', {'strength': 0.45, 'softness': 0.7}),
    ],
    'plug-in-clay-film-grain': [
        ('gegl:noise-cie-lch', {'lightness-distance': 4.0, 'chroma-distance': 2.0}),
        ('gegl:vignette', {'radius': 1.3, 'softness': 0.7}),
    ],
}

LABELS = {
    'plug-in-clay-chromatic': _("Chromatic _Aberration…"),
    'plug-in-clay-vhs': _("_VHS / CRT Scanlines…"),
    'plug-in-clay-halftone': _("_Halftone Pop-Art…"),
    'plug-in-clay-bloom': _("Anamorphic _Bloom…"),
    'plug-in-clay-film-grain': _("Vintage _Film Grain…"),
}


def run_vfx(procedure, run_mode, image, drawables, config, run_data):
    if len(drawables) != 1:
        msg = _("Procedure '{}' only works with one drawable.").format(procedure.get_name())
        error = GLib.Error.new_literal(Clay.PlugIn.error_quark(), msg, 0)
        return procedure.new_return_values(Clay.PDBStatusType.CALLING_ERROR, error)

    drawable = drawables[0]
    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi
        ClayUi.init('vfx-pack')
        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        dialog.destroy()

    Clay.context_push()
    image.undo_group_start()
    for op_name, props in LOOKS.get(procedure.get_name(), []):
        try:
            apply_gegl(drawable, op_name, props)
        except Exception:
            pass
    Clay.displays_flush()
    image.undo_group_end()
    Clay.context_pop()
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class VfxPack (Clay.PlugIn):
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return list(LOOKS.keys())

    def do_create_procedure(self, name):
        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            run_vfx, None)
        procedure.set_image_types("RGB*")
        procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.DRAWABLE)
        procedure.set_menu_label(LABELS[name])
        procedure.add_menu_path('<Image>/Filters/VFX')
        procedure.set_documentation(LABELS[name], LABELS[name], name)
        procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
        return procedure


Clay.main(VfxPack.__gtype__, sys.argv)
