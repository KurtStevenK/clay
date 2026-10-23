stamp_operations_enums = custom_target('stamp-operations-enums.h',
  input : [
    files(
      'operations-enums.h'
    ),
  ],
  output: [ 'stamp-operations-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'operations-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libappoperations_sources = [
  'clay-operation-config.c',
  'clay-operations.c',
  'claybrightnesscontrastconfig.c',
  'claycageconfig.c',
  'claycolorbalanceconfig.c',
  'claycurvesconfig.c',
  'clayhuesaturationconfig.c',
  'claylevelsconfig.c',
  'clayoperationborder.c',
  'clayoperationbrightnesscontrast.c',
  'clayoperationbuffersourcevalidate.c',
  'clayoperationcagecoefcalc.c',
  'clayoperationcagetransform.c',
  'clayoperationcolorbalance.c',
  'clayoperationcolorize.c',
  'clayoperationcomposecrop.c',
  'clayoperationcurves.c',
  'clayoperationdesaturate.c',
  'clayoperationequalize.c',
  'clayoperationfillsource.c',
  'clayoperationflood.c',
  'clayoperationgradient.c',
  'clayoperationgrow.c',
  'clayoperationhistogramsink.c',
  'clayoperationhuesaturation.c',
  'clayoperationlevels.c',
  'clayoperationmaskcomponents.cc',
  'clayoperationoffset.c',
  'clayoperationpointfilter.c',
  'clayoperationposterize.c',
  'clayoperationprofiletransform.c',
  'clayoperationscalarmultiply.c',
  'clayoperationsemiflatten.c',
  'clayoperationsetalpha.c',
  'clayoperationsettings.c',
  'clayoperationshrink.c',
  'clayoperationthreshold.c',
  'clayoperationthresholdalpha.c',

  'operations-enums.c',
  stamp_operations_enums,
]

libappoperations = static_library('appoperations',
  libappoperations_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Operations"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2
  ],
)

subdir('layer-modes')
subdir('layer-modes-legacy')
subdir('tests')
