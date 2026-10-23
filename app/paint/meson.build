stamp_paint_enums = custom_target('stamp-paint-enums.h',
  input : [
    files(
      'paint-enums.h'
    ),
  ],
  output: [ 'stamp-paint-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'paint-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libapppaint_sources = [
  'clay-paint.c',
  'clayairbrush.c',
  'clayairbrushoptions.c',
  'claybrushcore-loops.cc',
  'claybrushcore.c',
  'clayclone.c',
  'claycloneoptions.c',
  'clayconvolve.c',
  'clayconvolveoptions.c',
  'claydodgeburn.c',
  'claydodgeburnoptions.c',
  'clayeraser.c',
  'clayeraseroptions.c',
  'clayheal.c',
  'clayink-blob.c',
  'clayink.c',
  'clayinkoptions.c',
  'clayinkundo.c',
  'claymybrushcore.c',
  'claymybrushoptions.c',
  'claymybrushsurface.c',
  'claypaintbrush.c',
  'claypaintcore-loops.cc',
  'claypaintcore-stroke.c',
  'claypaintcore.c',
  'claypaintcoreundo.c',
  'claypaintoptions.c',
  'claypencil.c',
  'claypenciloptions.c',
  'clayperspectiveclone.c',
  'clayperspectivecloneoptions.c',
  'claysmudge.c',
  'claysmudgeoptions.c',
  'claysourcecore.c',
  'claysourceoptions.c',

  'paint-enums.c',
  stamp_paint_enums,
]

libapppaint = static_library('apppaint',
  libapppaint_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Paint"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2, libmypaint,
  ],
)
