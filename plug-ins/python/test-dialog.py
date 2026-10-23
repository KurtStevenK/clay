#!/usr/bin/env python3
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

'''
A Python plugin.
Tests ClayProcedureDialog.

Temporarily, just testing widgets for subclasses of ClayResource.
Temporarily, just testing Brush subclass of Resource.
FUTURE: For all the parameter types.
Formerly PF_ constants, now all parameters for which ClayParamSpecs exist.

Not localized, no i18n
'''


def process_args(brush, font, gradient, palette, pattern):
    '''
    Test the args are sane.
    '''
    assert brush is not None
    assert isinstance(brush, Clay.Brush)
    id = brush.get_id()
    name = brush.get_name()
    assert id is not None
    msg = "Brush: {} (ID: {})".format(name, id)
    print(msg)
    Clay.message(msg)

    assert font is not None
    assert isinstance(font, Clay.Font)
    id = font.get_id()
    name = font.get_name()
    assert id is not None
    msg = "Font: {} (ID: {})".format(name, id)
    print(msg)
    Clay.message(msg)

    assert gradient is not None
    assert isinstance(gradient, Clay.Gradient)
    id = gradient.get_id()
    name = gradient.get_name()
    assert id is not None
    msg = "Gradient: {} (ID: {})".format(name, id)
    print(msg)
    Clay.message(msg)

    assert palette is not None
    assert isinstance(palette, Clay.Palette)
    id = palette.get_id()
    name = palette.get_name()
    assert id is not None
    msg = "Palette: {} (ID: {})".format(name, id)
    print(msg)
    Clay.message(msg)

    assert pattern is not None
    assert isinstance(pattern, Clay.Pattern)
    id = pattern.get_id()
    name = pattern.get_name()
    assert id is not None
    msg = "Pattern: {} (ID: {})".format(name, id)
    print(msg)
    Clay.message(msg)

    return


def test_dialog(procedure, run_mode, image, drawables, config, data):
    '''
    Just a standard shell for a plugin.
    '''
    if run_mode == Clay.RunMode.INTERACTIVE:
        gi.require_version('ClayUi', '3.0')
        from gi.repository import ClayUi

        ClayUi.init('python-fu-test-dialog')
        Gegl.init(None)
        dialog = ClayUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            return procedure.new_return_values(Clay.PDBStatusType.CANCEL, GLib.Error())
        else:
            dialog.destroy()

    brush = config.get_property('brush')
    font = config.get_property('font')
    gradient = config.get_property('gradient')
    palette = config.get_property('palette')
    pattern = config.get_property('pattern')

    Clay.context_push()

    process_args(brush, font, gradient, palette, pattern)

    Clay.displays_flush()

    Clay.context_pop()

    return procedure.new_return_values(Clay.PDBStatusType.SUCCESS, GLib.Error())


class TestDialogPlugin (Clay.PlugIn):
    # FUTURE all other Clay classes that have ClayParamSpecs

    ## ClayPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return [ 'python-fu-test-dialog' ]

    def do_create_procedure(self, name):
        procedure = Clay.ImageProcedure.new(self, name,
                                            Clay.PDBProcType.PLUGIN,
                                            test_dialog, None)
        procedure.set_sensitivity_mask (Clay.ProcedureSensitivityMask.NO_IMAGE)
        procedure.set_documentation ("Test dialog",
                                     "Test dialog",
                                     name)
        procedure.set_menu_label("Test dialog...")
        procedure.set_attribution("Lloyd Konneker",
                                  "Lloyd Konneker",
                                  "2022")
        # Top level menu "Test"
        procedure.add_menu_path ("<Image>/Filters/Development/Demos")

        procedure.add_brush_argument ("brush", "_Brush", "Brush", True,
                                      Clay.Brush.get_by_name ("2. Hardness 025"),
                                      False,
                                      GObject.ParamFlags.READWRITE)
        procedure.add_font_argument ("font", "_Font", "Font", True,
                                     Clay.Font.get_by_name ("Serif"),
                                     False,
                                     GObject.ParamFlags.READWRITE)
        procedure.add_gradient_argument ("gradient", "_Gradient",
                                         "Gradient", True,
                                         Clay.Gradient.get_by_name ("Incandescent"),
                                         False,
                                         GObject.ParamFlags.READWRITE)
        procedure.add_palette_argument ("palette", "_Palette",
                                        "Palette", True,
                                        Clay.Palette.get_by_name ("Default"),
                                        False,
                                        GObject.ParamFlags.READWRITE)
        procedure.add_pattern_argument ("pattern", "Pa_ttern",
                                        "Pattern", True,
                                        Clay.Pattern.get_by_name ("Paper"),
                                        False,
                                        GObject.ParamFlags.READWRITE)

        return procedure

Clay.main(TestDialogPlugin.__gtype__, sys.argv)
