#!/usr/bin/env python3
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import time
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

def foggify(procedure, run_mode, image, drawables, config, data):
    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi

        ClayUi.init('python-fu-foggify')

        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        else:
            dialog.destroy()

    color      = config.get_property('color')
    name       = config.get_property('name')
    turbulence = config.get_property('turbulence')
    opacity    = config.get_property('opacity')

    Clay.context_push()
    image.undo_group_start()

    if image.get_base_type() is Clay.ImageBaseType.RGB:
        type = Clay.ImageType.RGBA_IMAGE
    else:
        type = Clay.ImageType.GRAYA_IMAGE
    for drawable in drawables:
        fog = Clay.Layer.new(image, name,
                             drawable.get_width(), drawable.get_height(),
                             type, opacity,
                             Clay.LayerMode.NORMAL)
        fog.fill(Clay.FillType.TRANSPARENT)
        image.insert_layer(fog, drawable.get_parent(),
                           image.get_item_position(drawable))

        Clay.context_set_background(color)
        fog.edit_fill(Clay.FillType.BACKGROUND)

        # create a layer mask for the new layer
        mask = fog.create_mask(0)
        fog.add_mask(mask)

        # add some clouds to the layer
        _, x, y, width, height = mask.mask_intersect()
        if not Clay.Selection.is_empty(image):
          x = 0
          y = 0
        plasma_filter = Clay.DrawableFilter.new(mask, "gegl:plasma", "")
        plasma_filter_config = plasma_filter.get_config()
        plasma_filter_config.set_property("seed", int(time.time()))
        plasma_filter_config.set_property("turbulence", turbulence)
        plasma_filter_config.set_property("x", x)
        plasma_filter_config.set_property("y", y)
        plasma_filter_config.set_property("width", width)
        plasma_filter_config.set_property("height", height)
        mask.merge_filter (plasma_filter)

        # apply the clouds to the layer
        fog.remove_mask(Clay.MaskApplyMode.APPLY)
        fog.set_visible(True)

    Clay.displays_flush()

    image.undo_group_end()
    Clay.context_pop()

    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())

class Foggify (Clay.PlugIn):
    ## ClayPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return [ 'python-fu-foggify' ]

    def do_create_procedure(self, name):
        Gegl.init(None)

        _color = Gegl.Color.new("black")
        _color.set_rgba(0.871, 0.457, 0.061, 1.0)

        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            foggify, None)
        procedure.set_image_types("RGB*, GRAY*");
        procedure.set_sensitivity_mask (Clay.ProcedureSensitivityMask.DRAWABLE |
                                        Clay.ProcedureSensitivityMask.DRAWABLES)
        procedure.set_documentation (_("Add a layer of fog"),
                                     _("Adds a layer of fog to the image."),
                                     name)
        procedure.set_menu_label(_("_Fog..."))
        procedure.set_attribution("James Henstridge",
                                  "James Henstridge",
                                  "1999,2007")
        procedure.add_menu_path ("<Image>/Filters/Decor")

        procedure.add_string_argument ("name", _("Layer _name"), _("Layer name"),
                                       _("Clouds"), GObject.ParamFlags.READWRITE)
        procedure.add_color_argument ("color", _("_Fog color"), _("Fog color"),
                                      True, _color, GObject.ParamFlags.READWRITE)
        procedure.add_double_argument ("turbulence", _("_Turbulence"), _("Turbulence"),
                                       0.0, 7.0, 1.0, GObject.ParamFlags.READWRITE)
        procedure.add_double_argument ("opacity", _("O_pacity"), _("Opacity"),
                                       0.0, 100.0, 100.0, GObject.ParamFlags.READWRITE)

        return procedure

Clay.main(Foggify.__gtype__, sys.argv)
