#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import os
import sys

def _(message):
    return GLib.dgettext(None, message)


def run_export(procedure, run_mode, image, drawables, config, run_data):
    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi
        ClayUi.init('batch-export')
        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        dialog.destroy()

    dest = config.get_property('destination')
    width = int(config.get_property('max-width'))
    watermark = config.get_property('watermark')

    if not dest:
        dest = os.path.join(GLib.get_tmp_dir(), "clay-export.png")

    dup = image.duplicate()
    try:
        if width > 0 and dup.get_width() > width:
            new_h = int(dup.get_height() * (width / float(dup.get_width())))
            dup.scale(width, max(1, new_h))
        if watermark:
            try:
                layer = Clay.TextLayer.new(dup, watermark, Clay.context_get_font(),
                                           32.0, 72.0)
                dup.insert_layer(layer, None, 0)
                layer.set_offsets(24, dup.get_height() - 64)
            except Exception:
                pass
        file = Gio.File.new_for_path(dest)
        Clay.file_save(Clay.RunMode.NONINTERACTIVE, dup, file, None)
    finally:
        dup.delete()

    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class BatchExport (Clay.PlugIn):
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return ['plug-in-clay-batch-export']

    def do_create_procedure(self, name):
        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            run_export, None)
        procedure.set_image_types("*")
        procedure.set_sensitivity_mask(Clay.ProcedureSensitivityMask.ALWAYS)
        procedure.set_menu_label(_("Social / Web Batch E_xport…"))
        procedure.add_menu_path('<Image>/File/Export')
        procedure.set_documentation(_("Export a web-ready copy"),
                                    _("Scale and optionally watermark, then save."),
                                    name)
        procedure.set_attribution("ClayDesign", "ClayDesign", "2026")
        procedure.add_string_argument("destination", _("_Destination"),
                                      _("Output file path"),
                                      "", GObject.ParamFlags.READWRITE)
        procedure.add_int_argument("max-width", _("Max _width"),
                                   _("Maximum export width in pixels (0 keeps original)"),
                                   0, 16384, 1920, GObject.ParamFlags.READWRITE)
        procedure.add_string_argument("watermark", _("_Watermark"),
                                      _("Optional watermark text"),
                                      "", GObject.ParamFlags.READWRITE)
        return procedure


Clay.main(BatchExport.__gtype__, sys.argv)
