stamp_config_enums = custom_target('stamp-config-enums.h',
  input : [
    files(
      'config-enums.h'
    ),
  ],
  output: [ 'stamp-config-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'config-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libappconfig_sources = [
  'clayconfig-dump.c',
  'clayconfig-file.c',
  'clayconfig-utils.c',
  'claycoreconfig.c',
  'claydialogconfig.c',
  'claydisplayconfig.c',
  'claydisplayoptions.c',
  'clayearlyrc.c',
  'claygeglconfig.c',
  'clayguiconfig.c',
  'claypluginconfig.c',
  'clayrc-deserialize.c',
  'clayrc-serialize.c',
  'clayrc-unknown.c',
  'clayrc.c',
  'clayxmlparser.c',

  'config-enums.c',
  stamp_config_enums,
]

libappconfig = static_library('appconfig',
  libappconfig_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Config"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2, gio, gio_specific, libmypaint,
  ],
)

test('app-config',
  executable('test-config',
    [ 'test-config.c', app_debug_files, ],

    dependencies: [
      appstream,
      libapp_dep,
    ],
    link_with: [
      libclaybase,
      libclayconfig,
      libclaycolor,
      libclaymath,
      libclaymodule,
      libclaythumb,

      libappconfig,
    ],
    install: false,
  ),
  suite: 'app'
)
