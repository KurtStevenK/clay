stamp_plug_in_enums = custom_target('stamp-plug-in-enums.h',
  input : [
    files(
      'plug-in-enums.h'
    ),
  ],
  output: [ 'stamp-plug-in-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'plug-in-',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "clay-intl.h"'
  ],
  build_by_default: true
)

libappplugin_sources = [
  'clayenvirontable.c',
  'claygpparams.c',
  'clayinterpreterdb.c',
  'clayplugin-cleanup.c',
  'clayplugin-context.c',
  'clayplugin-message.c',
  'clayplugin-proc.c',
  'clayplugin-progress.c',
  'clayplugin.c',
  'clayplugindebug.c',
  'clayplugindef.c',
  'claypluginerror.c',
  'claypluginmanager-call.c',
  'claypluginmanager-data.c',
  'claypluginmanager-file.c',
  'claypluginmanager-help-domain.c',
  'claypluginmanager-query.c',
  'claypluginmanager-restore.c',
  'claypluginmanager.c',
  'claypluginprocedure.c',
  'claypluginprocframe.c',
  'claypluginshm.c',
  'claytemporaryprocedure.c',
  'plug-in-menu-path.c',
  'plug-in-rc.c',

  'plug-in-enums.c',
  stamp_plug_in_enums,

  appcoremarshal[1],
]

libappplugin = static_library('appplug-in',
  libappplugin_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Plug-In"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2,
  ],
)
