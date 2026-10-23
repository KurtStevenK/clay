stamp_gegl_enums = custom_target('stamp-clay-gegl-enums.h',
  input : [
    files(
      'clay-gegl-enums.h'
    ),
  ],
  output: [ 'stamp-clay-gegl-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'clay-gegl-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n' +
    '#include "core/core-enums.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libappgegl_loops = simd.check('clay-gegl-loops-simd',
  sse2: 'clay-gegl-loops-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libappgegl_sources = [
  'clay-babl-compat.c',
  'clay-babl.c',
  'clay-gegl-apply-operation.c',
  'clay-gegl-loops.cc',
  'clay-gegl-mask-combine.cc',
  'clay-gegl-mask.c',
  'clay-gegl-nodes.c',
  'clay-gegl-tile-compat.c',
  'clay-gegl-utils.c',
  'clay-gegl.c',
  'clayapplicator.c',
  'claytilehandlervalidate.c',

  'clay-gegl-enums.c',
  stamp_gegl_enums
]

libappgegl = static_library('appgegl',
  libappgegl_sources,
  link_with: libappgegl_loops[0],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-GEGL"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
    gexiv2
  ],
)
