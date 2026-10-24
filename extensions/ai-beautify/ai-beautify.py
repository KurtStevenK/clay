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


PRESETS = {
    'auto_beautify': [
        ('gegl:color-temperature', {'intended-temperature': 6500.0, 'original-temperature': 6000.0}),
        ('gegl:shadows-highlights', {'shadows': 0.15, 'highlights': -0.08}),
        ('gegl:saturation', {'scale': 1.08}),
        ('gegl:unsharp-mask', {'std-dev': 0.8, 'scale': 0.35}),
    ],
    'skin_smooth_portrait': [
        ('gegl:noise-reduction', {'iterations': 3}),
        ('gegl:bilateral-filter', {'blur-radius': 4.0, 'edge-preservation': 0.4}),
        ('gegl:unsharp-mask', {'std-dev': 0.5, 'scale': 0.2}),
    ],
    'cinematic_grade': [
        ('gegl:color-temperature', {'intended-temperature': 5400.0, 'original-temperature': 6500.0}),
        ('gegl:saturation', {'scale': 0.92}),
        ('gegl:vignette', {'radius': 1.2, 'softness': 0.8, 'shape': 0}),
    ],
    'hdr_pop': [
        ('gegl:shadows-highlights', {'shadows': 0.25, 'highlights': -0.12}),
        ('gegl:unsharp-mask', {'std-dev': 2.0, 'scale': 0.45}),
        ('gegl:saturation', {'scale': 1.12}),
    ],
    'denoise_and_sharpen': [
        ('gegl:noise-reduction', {'iterations': 4}),
        ('gegl:unsharp-mask', {'std-dev': 1.0, 'scale': 0.5}),
    ],
}


def run_preset(procedure, run_mode, image, drawables, config, run_data):
    if len(drawables) != 1:
        msg = _("Procedure '{}' only works with one drawable.").format(procedure.get_name())
        error = GLib.Error.new_literal(Clay.PlugIn.error_quark(), msg, 0)
        return procedure.new_return_values(Clay.PDBStatusType.CALLING_ERROR, error)

    drawable = drawables[0]
    preset = config.get_property('preset')
    strength = float(config.get_property('strength'))

    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi
        ClayUi.init('ai-beautify')
        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        dialog.destroy()
        preset = config.get_property('preset')
        strength = float(config.get_property('strength'))

    steps = PRESETS.get(preset, PRESETS['auto_beautify'])
    Clay.context_push()
    image.undo_group_start()
    for op_name, props in steps:
        scaled = {}
        for key, value in props.items():
            if isinstance(value, float) and key not in ('intended-temperature', 'original-temperature'):
                scaled[key] = value * strength
            else:
                scaled[key] = value
        try:
            apply_gegl(drawable, op_name, scaled)
        except Exception:
            pass
    Clay.displays_flush()
    image.undo_group_end()
    Clay.context_pop()
    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class AiBeautify (Clay.PlugIn):
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return [
            'plug-in-clay-auto-beautify',
            'plug-in-clay-portrait-retouch',
            'plug-in-clay-cinematic-grade',
            'plug-in-clay-hdr-pop',
        ]

    def do_create_procedure(self, name):
        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            run_preset, None)
        procedure.set_image_types("RGB*")
        procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.DRAWABLE)
        procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
        procedure.add_menu_path('<Image>/Filters/Beautify')
        procedure.add_double_argument("strength", _("_Strength"),
                                      _("How strong the look should be"),
                                      0.1, 2.0, 1.0, GObject.ParamFlags.READWRITE)

        mapping = {
            'plug-in-clay-auto-beautify':
                ('auto_beautify', _("One-Click Auto _Beautify…"),
                 _("Auto white-balance, contrast, saturation and mild sharpen")),
            'plug-in-clay-portrait-retouch':
                ('skin_smooth_portrait', _("Portrait _Skin Retouch…"),
                 _("Edge-preserving smoothing with detail recovery")),
            'plug-in-clay-cinematic-grade':
                ('cinematic_grade', _("_Cinematic Color Grade…"),
                 _("Warmer grade, slightly lower saturation, vignette")),
            'plug-in-clay-hdr-pop':
                ('hdr_pop', _("Smart _HDR & Clarify…"),
                 _("Local contrast and clarity boost")),
        }
        preset, label, docs = mapping[name]
        procedure.add_string_argument("preset", _("Preset"), _("Beautify preset id"),
                                      preset, GObject.ParamFlags.READWRITE)
        procedure.set_menu_label(label)
        procedure.set_documentation(docs, docs, name)
        return procedure


Clay.main(AiBeautify.__gtype__, sys.argv)
