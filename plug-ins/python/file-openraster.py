#!/usr/bin/env python3

# Clay Plug-in for the OpenRaster file format
# http://create.freedesktop.org/wiki/OpenRaster

# Copyright (C) 2009 by Jon Nordby <jononor@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Based on MyPaint source code by Martin Renold
# http://gitorious.org/mypaint/mypaint/blobs/edd84bcc1e091d0d56aa6d26637aa8a925987b6a/lib/document.py

import gi
gi.require_version('Clay', '3.0')
from gi.repository import Clay
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import os, sys, tempfile, zipfile
import xml.etree.ElementTree as ET

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

NESTED_STACK_END = object()

layermodes_map = {
    "svg:src-over":     Clay.LayerMode.NORMAL,
    "svg:multiply":     Clay.LayerMode.MULTIPLY,
    "svg:screen":       Clay.LayerMode.SCREEN,
    "svg:overlay":      Clay.LayerMode.OVERLAY,
    "svg:darken":       Clay.LayerMode.DARKEN_ONLY,
    "svg:lighten":      Clay.LayerMode.LIGHTEN_ONLY,
    "svg:color-dodge":  Clay.LayerMode.DODGE,
    "svg:color-burn":   Clay.LayerMode.BURN,
    "svg:hard-light":   Clay.LayerMode.HARDLIGHT,
    "svg:soft-light":   Clay.LayerMode.SOFTLIGHT,
    "svg:difference":   Clay.LayerMode.DIFFERENCE,
    "svg:color":        Clay.LayerMode.HSL_COLOR,
    "svg:luminosity":   Clay.LayerMode.HSV_VALUE,
    "svg:hue":          Clay.LayerMode.HSV_HUE,
    "svg:saturation":   Clay.LayerMode.HSV_SATURATION,
    "svg:plus":         Clay.LayerMode.ADDITION,
}

# There are less svg blending ops than we have Clay blend modes.
# We are going to map them as closely as possible.
clay_layermodes_map = {
    Clay.LayerMode.NORMAL:                  "svg:src-over",
    Clay.LayerMode.NORMAL_LEGACY:           "svg:src-over",
    Clay.LayerMode.MULTIPLY:                "svg:multiply",
    Clay.LayerMode.MULTIPLY_LEGACY:         "svg:multiply",
    Clay.LayerMode.SCREEN:                  "svg:screen",
    Clay.LayerMode.SCREEN_LEGACY:           "svg:screen",
    Clay.LayerMode.OVERLAY:                 "svg:overlay",
    Clay.LayerMode.OVERLAY_LEGACY:          "svg:overlay",
    Clay.LayerMode.DARKEN_ONLY:             "svg:darken",
    Clay.LayerMode.DARKEN_ONLY_LEGACY:      "svg:darken",
    Clay.LayerMode.LIGHTEN_ONLY:            "svg:lighten",
    Clay.LayerMode.LIGHTEN_ONLY_LEGACY:     "svg:lighten",
    Clay.LayerMode.DODGE:                   "svg:color-dodge",
    Clay.LayerMode.DODGE_LEGACY:            "svg:color-dodge",
    Clay.LayerMode.BURN:                    "svg:color-burn",
    Clay.LayerMode.BURN_LEGACY:             "svg:color-burn",
    Clay.LayerMode.HARDLIGHT:               "svg:hard-light",
    Clay.LayerMode.HARDLIGHT_LEGACY:        "svg:hard-light",
    Clay.LayerMode.SOFTLIGHT:               "svg:soft-light",
    Clay.LayerMode.SOFTLIGHT_LEGACY:        "svg:soft-light",
    Clay.LayerMode.DIFFERENCE:              "svg:difference",
    Clay.LayerMode.DIFFERENCE_LEGACY:       "svg:difference",
    Clay.LayerMode.HSL_COLOR:               "svg:color",
    Clay.LayerMode.HSL_COLOR_LEGACY:        "svg:color",
    Clay.LayerMode.HSV_VALUE:               "svg:luminosity",
    Clay.LayerMode.HSV_VALUE_LEGACY:        "svg:luminosity",
    Clay.LayerMode.HSV_HUE:                 "svg:hue",
    Clay.LayerMode.HSV_HUE_LEGACY:          "svg:hue",
    Clay.LayerMode.HSV_SATURATION:          "svg:saturation",
    Clay.LayerMode.HSV_SATURATION_LEGACY:   "svg:saturation",
    Clay.LayerMode.ADDITION:                "svg:plus",
    Clay.LayerMode.ADDITION_LEGACY:         "svg:plus",

    # FIXME Determine the closest available layer mode
    #       Alternatively we could add additional modes
    #       e.g. something like "clay:dissolve", this
    #       is what Krita seems to do too
    Clay.LayerMode.DISSOLVE:                "svg:src-over",
    Clay.LayerMode.DIVIDE:                  "svg:src-over",
    Clay.LayerMode.DIVIDE_LEGACY:           "svg:src-over",
    Clay.LayerMode.BEHIND:                  "svg:src-over",
    Clay.LayerMode.BEHIND_LEGACY:           "svg:src-over",
    Clay.LayerMode.GRAIN_EXTRACT:           "svg:src-over",
    Clay.LayerMode.GRAIN_EXTRACT_LEGACY:    "svg:src-over",
    Clay.LayerMode.GRAIN_MERGE:             "svg:src-over",
    Clay.LayerMode.GRAIN_MERGE_LEGACY:      "svg:src-over",
    Clay.LayerMode.COLOR_ERASE:             "svg:src-over",
    Clay.LayerMode.COLOR_ERASE_LEGACY:      "svg:src-over",
    Clay.LayerMode.LCH_HUE:                 "svg:src-over",
    Clay.LayerMode.LCH_CHROMA:              "svg:src-over",
    Clay.LayerMode.LCH_COLOR:               "svg:src-over",
    Clay.LayerMode.LCH_LIGHTNESS:           "svg:src-over",
    Clay.LayerMode.SUBTRACT:                "svg:src-over",
    Clay.LayerMode.SUBTRACT_LEGACY:         "svg:src-over",
    Clay.LayerMode.VIVID_LIGHT:             "svg:src-over",
    Clay.LayerMode.PIN_LIGHT:               "svg:src-over",
    Clay.LayerMode.LINEAR_LIGHT:            "svg:src-over",
    Clay.LayerMode.HARD_MIX:                "svg:src-over",
    Clay.LayerMode.EXCLUSION:               "svg:src-over",
    Clay.LayerMode.LINEAR_BURN:             "svg:src-over",
    Clay.LayerMode.LUMA_DARKEN_ONLY:        "svg:src-over",
    Clay.LayerMode.LUMA_LIGHTEN_ONLY:       "svg:src-over",
    Clay.LayerMode.LUMINANCE:               "svg:src-over",
    Clay.LayerMode.COLOR_ERASE:             "svg:src-over",
    Clay.LayerMode.ERASE:                   "svg:src-over",
    Clay.LayerMode.MERGE:                   "svg:src-over",
    Clay.LayerMode.SPLIT:                   "svg:src-over",
    Clay.LayerMode.PASS_THROUGH:            "svg:src-over",
}

def reverse_map(mapping):
    return dict((v,k) for k, v in mapping.items())

def get_image_attributes(orafile):
    xml = orafile.read('stack.xml')
    image = ET.fromstring(xml)
    stack = image.find('stack')
    w = int(image.attrib.get('w', ''))
    h = int(image.attrib.get('h', ''))

    return stack, w, h

def get_layer_attributes(layer):
    a = layer.attrib
    path = a.get('src', '')
    name = a.get('name', '')
    x = int(a.get('x', '0'))
    y = int(a.get('y', '0'))
    opac = float(a.get('opacity', '1.0'))
    visible = a.get('visibility', 'visible') != 'hidden'
    m = a.get('composite-op', 'svg:src-over')
    layer_mode = layermodes_map.get(m, Clay.LayerMode.NORMAL)
    selected = a.get('selected', 'false') == 'true'
    locked = a.get('edit-locked', 'false') == 'true'

    return path, name, x, y, opac, visible, layer_mode, selected, locked

def get_group_layer_attributes(layer):
    a = layer.attrib
    name = a.get('name', '')
    opac = float(a.get('opacity', '1.0'))
    visible = a.get('visibility', 'visible') != 'hidden'
    m = a.get('composite-op', 'svg:src-over')
    layer_mode = layermodes_map.get(m, Clay.LayerMode.NORMAL)
    selected = a.get('selected', 'false') == 'true'
    locked = a.get('edit-locked', 'false') == 'true'

    return name, 0, 0, opac, visible, layer_mode, selected, locked

def thumbnail_ora(procedure, file, thumb_size, args, data):
    tempdir = tempfile.mkdtemp('clay-plugin-file-openraster')
    orafile = zipfile.ZipFile(file.peek_path())
    stack, w, h = get_image_attributes(orafile)

    # create temp file
    tmp = os.path.join(tempdir, 'tmp.png')
    with open(tmp, 'wb') as fid:
        fid.write(orafile.read('Thumbnails/thumbnail.png'))

    thumb_file = Gio.file_new_for_path(tmp)
    pdb_proc   = Clay.get_pdb().lookup_procedure('file-png-load')
    pdb_config = pdb_proc.create_config()
    pdb_config.set_property('run-mode', Clay.RunMode.NONINTERACTIVE)
    pdb_config.set_property('file', thumb_file)
    result = pdb_proc.run(pdb_config)
    os.remove(tmp)
    os.rmdir(tempdir)

    if (result.index(0) == Clay.PDBStatusType.SUCCESS):
        img = result.index(1)
        # TODO: scaling

        return Clay.ValueArray.new_from_values([
            GObject.Value(Clay.PDBStatusType, Clay.PDBStatusType.SUCCESS),
            GObject.Value(Clay.Image, img),
            GObject.Value(GObject.TYPE_INT, w),
            GObject.Value(GObject.TYPE_INT, h),
            GObject.Value(Clay.ImageType, Clay.ImageType.RGB_IMAGE),
            GObject.Value(GObject.TYPE_INT, 1)
        ])
    else:
        return procedure.new_return_values(result.index(0), GLib.Error(result.index(1)))

def export_ora(procedure, run_mode, image, file, options, metadata, config, data):
    def write_file_str(zfile, fname, data):
        # work around a permission bug in the zipfile library:
        # http://bugs.python.org/issue3394
        zi = zipfile.ZipInfo(fname)
        zi.external_attr = int("100644", 8) << 16
        zfile.writestr(zi, data)

    Clay.progress_init("Exporting openraster image")
    tempdir = tempfile.mkdtemp('clay-plugin-file-openraster')

    # use .tmpsave extension, so we don't overwrite a valid file if
    # there is an exception
    orafile = zipfile.ZipFile(file.peek_path() + '.tmpsave', 'w', compression=zipfile.ZIP_STORED)

    write_file_str(orafile, 'mimetype', 'image/openraster') # must be the first file written

    # build image attributes
    xml_image = ET.Element('image')
    stack = ET.SubElement(xml_image, 'stack')
    a = xml_image.attrib
    a['w'] = str(image.get_width())
    a['h'] = str(image.get_height())

    def store_layer(image, drawable, path):
        tmp = os.path.join(tempdir, 'tmp.png')
        interlace, compression = 0, 2

        width, height = drawable.get_width(), drawable.get_height()
        tmp_img = Clay.Image.new(width, height, image.get_base_type())
        if (image.get_base_type() == Clay.ImageBaseType.INDEXED):
            tmp_img.set_palette(image.get_palette())

        tmp_layer = Clay.Layer.new_from_drawable (drawable, tmp_img)
        tmp_layer.set_offsets (0, 0)
        # Set opacity to 100% to prevent doubling it on import with the
        # XML's opacity setting
        tmp_layer.set_opacity (100.0)
        tmp_img.insert_layer (tmp_layer, None, 0)

        pdb_proc   = Clay.get_pdb().lookup_procedure('file-png-export')
        pdb_config = pdb_proc.create_config()
        pdb_config.set_property('run-mode', Clay.RunMode.NONINTERACTIVE)
        pdb_config.set_property('image', tmp_img)
        pdb_config.set_property('file', Gio.File.new_for_path(tmp))
        pdb_config.set_property('options', None)
        pdb_config.set_property('interlaced', interlace)
        pdb_config.set_property('compression', compression)
        # write all PNG chunks except oFFs(ets)
        pdb_config.set_property('bkgd', True)
        pdb_config.set_property('offs', False)
        pdb_config.set_property('phys', True)
        pdb_config.set_property('time', True)
        pdb_config.set_property('save-transparent', True)
        pdb_proc.run(pdb_config)
        if (os.path.exists(tmp)):
            orafile.write(tmp, path)
            os.remove(tmp)
        else:
            print("Error removing ", tmp)

        tmp_img.delete()

    def add_layer(parent, x, y, opac, clay_layer, path, visible=True):
        store_layer(image, clay_layer, path)
        # create layer attributes
        layer = ET.Element('layer')
        parent.append(layer)
        a = layer.attrib
        a['src'] = path
        a['name'] = clay_layer.get_name()
        a['x'] = str(x)
        a['y'] = str(y)
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = clay_layermodes_map.get(clay_layer.get_mode(), 'svg:src-over')

        # ORA Specification extensions
        if clay_layer in image.get_selected_layers():
            a['selected'] = "true"
        if clay_layer.get_lock_content():
            a['edit-locked'] = "true"
        return layer

    def add_group_layer(parent, opac, clay_layer, visible=True):
        # create layer attributes
        group_layer = ET.Element('stack')
        parent.append(group_layer)
        a = group_layer.attrib
        a['name'] = clay_layer.get_name()
        a['opacity'] = str(opac)
        a['visibility'] = 'visible' if visible else 'hidden'
        a['composite-op'] = clay_layermodes_map.get(clay_layer.get_mode(), 'svg:src-over')

        # ORA Specification extensions
        if clay_layer in image.get_selected_layers():
            a['selected'] = "true"
        if clay_layer.get_lock_content():
            a['edit-locked'] = "true"
        return group_layer


    def enumerate_layers(layers):
        for layer in layers:
            if not layer.is_group():
                yield layer
            else:
                yield layer
                for sublayer in enumerate_layers(layer.get_children()):
                    yield sublayer
                yield NESTED_STACK_END

    # save layers
    parent_groups = []
    i = 0

    layer_stack = image.get_layers()
    # Number of top level layers for tracking progress
    lay_cnt = len(layer_stack)

    for lay in enumerate_layers(layer_stack):
        prev_lay = i
        if lay is NESTED_STACK_END:
            parent_groups.pop()
            continue
        _, x, y = lay.get_offsets()
        opac = lay.get_opacity () / 100.0 # needs to be between 0.0 and 1.0

        if not parent_groups:
            path_name = 'data/{:03d}.png'.format(i)
            i += 1
        else:
            path_name = 'data/{}-{:03d}.png'.format(
                parent_groups[-1][1], parent_groups[-1][2])
            parent_groups[-1][2] += 1

        parent = stack if not parent_groups else parent_groups[-1][0]

        if lay.is_group():
            group = add_group_layer(parent, opac, lay, lay.get_visible())
            group_path = ("{:03d}".format(i) if not parent_groups else
                parent_groups[-1][1] + "-{:03d}".format(parent_groups[-1][2]))
            parent_groups.append([group, group_path , 0])
        else:
            add_layer(parent, x, y, opac, lay, path_name, lay.get_visible())

        if (i > prev_lay):
            Clay.progress_update(i/lay_cnt)

    # save mergedimage
    thumb = image.duplicate()
    thumb_layer = thumb.merge_visible_layers (Clay.MergeType.CLIP_TO_IMAGE)
    store_layer (thumb, thumb_layer, 'mergedimage.png')

    # save thumbnail
    w, h = image.get_width(), image.get_height()
    if max (w, h) > 256:
        # should be at most 256x256, without changing aspect ratio
        if w > h:
            w, h = 256, max(h*256/w, 1)
        else:
            w, h = max(w*256/h, 1), 256
        thumb_layer.scale(w, h, False)
    if thumb.get_precision() != Clay.Precision.U8_NON_LINEAR:
        thumb.convert_precision (Clay.Precision.U8_NON_LINEAR)
    store_layer(thumb, thumb_layer, 'Thumbnails/thumbnail.png')
    thumb.delete()

    # write stack.xml
    xml = ET.tostring(xml_image, encoding='UTF-8')
    write_file_str(orafile, 'stack.xml', xml)

    # finish up
    orafile.close()
    os.rmdir(tempdir)
    if os.path.exists(file.peek_path()):
        os.remove(file.peek_path()) # win32 needs that
    os.rename(file.peek_path() + '.tmpsave', file.peek_path())

    Clay.progress_end()

    return Clay.ValueArray.new_from_values([
        GObject.Value(Clay.PDBStatusType, Clay.PDBStatusType.SUCCESS)
    ])

def load_ora(procedure, run_mode, file, metadata, flags, config, data):
    tempdir = tempfile.mkdtemp('clay-plugin-file-openraster')
    orafile = zipfile.ZipFile(file.peek_path())
    stack, w, h = get_image_attributes(orafile)

    Clay.progress_init("Loading openraster image")

    img = Clay.Image.new(w, h, Clay.ImageBaseType.RGB)

    def get_layers(root):
        """iterates over layers and nested stacks"""
        for item in root:
            if item.tag == 'layer':
                yield item
            elif item.tag == 'stack':
                yield item
                for subitem in get_layers(item):
                    yield subitem
                yield NESTED_STACK_END

    parent_groups = []

    # Number of top level layers for tracking progress
    lay_cnt = len(stack)

    layer_no = 0
    selected_layers = []
    for item in get_layers(stack):
        prev_lay = layer_no

        if item is NESTED_STACK_END:
            parent_groups.pop()
            continue

        if item.tag == 'stack':
            name, x, y, opac, visible, layer_mode, selected, locked = get_group_layer_attributes(item)
            clay_layer = Clay.GroupLayer.new(img, name)

        else:
            path, name, x, y, opac, visible, layer_mode, selected, locked = get_layer_attributes(item)

            if not path.lower().endswith('.png') and not path.lower().endswith('.svg'):
                continue
            if not name:
                # use the filename without extension as name
                n = os.path.basename(path)
                name = os.path.splitext(n)[0]

            # create temp file. Needed because clay cannot load files from inside a zip file
            tmp = os.path.join(tempdir, 'tmp.png')
            if path.lower().endswith('.svg'):
                tmp = os.path.join(tempdir, 'tmp.svg')
            with open(tmp, 'wb') as fid:
                try:
                    data = orafile.read(path)
                except KeyError:
                    # support for bad zip files (saved by old versions of this plugin)
                    data = orafile.read(path.encode('utf-8'))
                    print('WARNING: bad OpenRaster ZIP file. There is an utf-8 encoded filename that does not have the utf-8 flag set:', repr(path))
                fid.write(data)

            # import layer, set attributes and add to image
            pdb_proc   = Clay.get_pdb().lookup_procedure('clay-file-load-layer')
            pdb_config = pdb_proc.create_config()
            pdb_config.set_property('run-mode', Clay.RunMode.NONINTERACTIVE)
            pdb_config.set_property('image', img)
            pdb_config.set_property('file', Gio.File.new_for_path(tmp))
            result = pdb_proc.run(pdb_config)
            if (result.index(0) == Clay.PDBStatusType.SUCCESS):
                clay_layer = result.index(1)
                os.remove(tmp)
            else:
                print("Error loading layer from openraster image.")

        clay_layer.set_name(name)
        clay_layer.set_mode(layer_mode)
        clay_layer.set_offsets(x, y)  # move to correct position
        clay_layer.set_opacity(opac * 100)  # a float between 0 and 100
        clay_layer.set_visible(visible)
        clay_layer.set_lock_content(locked)
        if selected:
            selected_layers.append(clay_layer)

        img.insert_layer(clay_layer,
                         parent_groups[-1][0] if parent_groups else None,
                         parent_groups[-1][1] if parent_groups else layer_no)
        if parent_groups:
            parent_groups[-1][1] += 1
        else:
            layer_no += 1

        if clay_layer.is_group():
            parent_groups.append([clay_layer, 0])

        if (layer_no > prev_lay):
            Clay.progress_update(layer_no/lay_cnt)

    if len(selected_layers) > 0:
        img.set_selected_layers(selected_layers)

    Clay.progress_end()

    os.rmdir(tempdir)

    return Clay.ValueArray.new_from_values([
        GObject.Value(Clay.PDBStatusType, Clay.PDBStatusType.SUCCESS),
        GObject.Value(Clay.Image, img),
    ]), flags


class FileOpenRaster (Clay.PlugIn):
    ## ClayPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'clay30-python', None

    def do_query_procedures(self):
        return [ 'file-openraster-load-thumb',
                 'file-openraster-load',
                 'file-openraster-export' ]

    def do_create_procedure(self, name):
        if name == 'file-openraster-export':
            procedure = Clay.ExportProcedure.new(self, name,
                                                 Clay.PDBProcType.PLUGIN,
                                                 False, export_ora, None)
            procedure.set_image_types("*");
            procedure.set_documentation (_('export an OpenRaster (.ora) file'),
                                         _('export an OpenRaster (.ora) file'),
                                         name)
            procedure.set_menu_label(_('OpenRaster'))
            procedure.set_extensions ("ora");
        elif name == 'file-openraster-load':
            procedure = Clay.LoadProcedure.new (self, name,
                                                Clay.PDBProcType.PLUGIN,
                                                load_ora, None)
            procedure.set_menu_label(_('OpenRaster'))
            procedure.set_documentation (_('load an OpenRaster (.ora) file'),
                                         _('load an OpenRaster (.ora) file'),
                                         name)
            procedure.set_mime_types ("image/openraster");
            procedure.set_extensions ("ora");
            procedure.set_thumbnail_loader ('file-openraster-load-thumb');
        else: # 'file-openraster-load-thumb'
            procedure = Clay.ThumbnailProcedure.new (self, name,
                                                     Clay.PDBProcType.PLUGIN,
                                                     thumbnail_ora, None)
            procedure.set_documentation (_('loads a thumbnail from an OpenRaster (.ora) file'),
                                         _('loads a thumbnail from an OpenRaster (.ora) file'),
                                         name)
        procedure.set_attribution('Jon Nordby', #author
                                  'Jon Nordby', #copyright
                                  '2009') #year
        return procedure

Clay.main(FileOpenRaster.__gtype__, sys.argv)
