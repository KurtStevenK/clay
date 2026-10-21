import os
import sys

gi.require_version('Gegl', '0.4')
from gi.repository import Gegl

src_dir = os.path.join('gimp-data', 'images', 'logo')
input_path = os.path.join(src_dir, 'clay-logo.svg')
output_path = os.path.join(src_dir, 'gimp.ico')

sizes = [16, 20, 22, 24, 32, 40, 48, 64, 96, 128, 256]
image_size = max(sizes)

image = Gimp.Image.new(image_size, image_size, Gimp.ImageBaseType.RGB)
image.undo_disable()

load = Gimp.get_pdb().lookup_procedure('file-svg-load')
if load is None:
  sys.stderr.write('ERROR: file-svg-load not found\n')
  sys.exit(70)

for size in sizes:
  config = load.create_config()
  config.set_property('file', Gio.file_new_for_path(input_path))
  config.set_property('width', size)
  config.set_property('height', size)
  v = Gimp.Procedure.run(load, config)
  if v.index(0) != Gimp.PDBStatusType.SUCCESS:
    sys.stderr.write(f'ERROR: svg load failed at {size}px\n')
    sys.exit(70)
  tmp_image = v.index(1)
  drawables = tmp_image.get_selected_drawables()
  layer = Gimp.Layer.new_from_drawable(drawables[0], image)
  image.insert_layer(layer, None, 0)
  layer.set_name(str(size))
  tmp_image.delete()

export = Gimp.get_pdb().lookup_procedure('file-ico-export')
if export is None:
  sys.stderr.write('ERROR: file-ico-export not found\n')
  sys.exit(70)

config = export.create_config()
config.set_property('image', image)
config.set_property('file', Gio.file_new_for_path(output_path))
v = Gimp.Procedure.run(export, config)
if v.index(0) != Gimp.PDBStatusType.SUCCESS:
  sys.stderr.write(f'ERROR: ico export failed: {Gimp.get_pdb().get_last_error()}\n')
  sys.exit(70)

sys.stderr.write(f'WROTE {output_path}\n')
