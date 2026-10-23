stamp_text_enums = custom_target('stamp-text-enums.h',
  input : [
    files(
      'text-enums.h'
    ),
  ],
  output: [ 'stamp-text-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'text-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libapptext_sources = [
  'clayfont.c',
  'clayfontfactory.c',
  'claytext-compat.c',
  'claytext-parasite.c',
  'claytext-path.c',
  'claytext-xlfd.c',
  'claytext.c',
  'claytextlayer-transform.c',
  'claytextlayer-xcf.c',
  'claytextlayer.c',
  'claytextlayout-render.c',
  'claytextlayout.c',
  'claytextundo.c',

  'text-enums.c',
  stamp_text_enums
]


libapptext = static_library('apptext',
  libapptext_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Text"',
  dependencies: [
    gegl, gdk_pixbuf, gexiv2, harfbuzz, pangocairo, pangoft2,
  ],
)
