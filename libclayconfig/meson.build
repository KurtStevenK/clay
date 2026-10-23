
stamp_config_enums = custom_target('stamp-clayconfigenums.h',
  input : [
    files(
      'clayconfigenums.h'
    ),
  ],
  output: [ 'stamp-clayconfigenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'clayconfig',
    '#include <gio/gio.h>\n' +
    '#include "libclaybase/claybase.h"\n',
    '#include "libclay/libclay-intl.h"',
    libclay_mkenums_dtails
  ],
  build_by_default: true
)

libclayconfig_sources_introspectable = files(
  'claycolorconfig.c',
  'clayconfig-deserialize.c',
  'clayconfig-error.c',
  'clayconfig-iface.c',
  'clayconfig-params.c',
  'clayconfig-path.c',
  'clayconfig-register.c',
  'clayconfig-serialize.c',
  'clayconfig-utils.c',
  'clayconfigwriter.c',
  'clayscanner.c',
)

libclayconfig_sources = [
  libclayconfig_sources_introspectable,

  'clayconfigenums.c',
  stamp_config_enums
]

libclayconfig_headers_introspectable = files(
  'claycolorconfig.h',
  'clayconfig-deserialize.h',
  'clayconfig-error.h',
  'clayconfig-iface.h',
  'clayconfig-params.h',
  'clayconfig-path.h',
  'clayconfig-register.h',
  'clayconfig-serialize.h',
  'clayconfig-utils.h',
  'clayconfigenums.h',
  'clayconfigtypes.h',
  'clayconfigwriter.h',
  'clayscanner.h',
)

libclayconfig_headers = [
  libclayconfig_headers_introspectable,
  'clayconfig.h',
]

libclayconfig_introspectable = [
  libclayconfig_headers_introspectable,
  libclayconfig_sources_introspectable,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclayconfig_def = configure_file(
  input: 'clayconfig.def',
  output: 'clayconfig.def',
  copy: true
)

libclayconfig = library('clayconfig-' + clay_api_version,
  libclayconfig_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gexiv2, gio, gio_specific, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayConfig"', '-DCLAY_CONFIG_COMPILATION', ],
  link_with: [
    libclaybase,
    libclaycolor,
    libclaymath,
  ],
  vs_module_defs: libclayconfig_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclayconfig_headers,
  subdir: clay_api_name / 'libclayconfig',
)
