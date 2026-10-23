# Unlike other enums file, we don't use the mkenums_wrap because this
# one is not versioned in the repository (not sure why). Moreover the
# options are quite different from the other generated enums, so it
# didn't make sense to overdo it.
claythumbenums = custom_target('claythumb-enums.c',
  input : [ 'claythumb-enums.h', ],
  output: [ 'claythumb-enums.c', ],
  command: [
    clay_mkenums,
    '--fhead','#include "config.h"\n'+
              '#include <glib-object.h>\n'+
              '#include "claythumb-enums.h"\n',
    '--fprod','/* enumerations from "@filename@" */',
    '--vhead','GType\n'+
              '@enum_name@_get_type (void)\n'+
              '{\n'+
              '  static const G@Type@Value values[] =\n'+
              '  {',
    '--vprod','    { @VALUENAME@, @valuedesc@, "@valuenick@" },',
    '--vtail','    { 0, NULL, NULL }\n'+
              '  };\n'+
              '\n'+
              '  static GType type = 0;\n'+
              '\n'+
              '  if (G_UNLIKELY (! type))\n'+
              '    type = g_@type@_register_static ("@EnumName@", values);\n'+
              '\n'+
              '  return type;\n'+
              '}\n',
    '@INPUT@',
  ],
  capture: true,
)

libclaythumb_sources_introspectable = files(
  'claythumb-error.c',
  'claythumb-utils.c',
  'claythumbnail.c',
)

libclaythumb_sources = [
  libclaythumb_sources_introspectable,
  claythumbenums,
]

libclaythumb_headers_introspectable = files(
  'claythumb-enums.h',
  'claythumb-error.h',
  'claythumb-types.h',
  'claythumb-utils.h',
  'claythumbnail.h',
)

libclaythumb_headers = [
  libclaythumb_headers_introspectable,
  'claythumb.h',
]

libclaythumb_introspectable = [
  libclaythumb_sources_introspectable,
  libclaythumb_headers_introspectable,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaythumb_def = configure_file(
  input: 'claythumb.def',
  output: 'claythumb.def',
  copy: true
)

libclaythumb = library('claythumb-'+ clay_api_version,
  libclaythumb_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, glib, gobject, gdk_pixbuf, gio,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayThumb"', '-DCLAY_THUMB_COMPILATION', ],
  link_with:  [
    libclaybase,
  ],
  vs_module_defs: libclaythumb_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaythumb_headers,
  subdir: clay_api_name / 'libclaythumb',
)


# Test program, not installed
clay_thumbnail_list = executable('clay-thumbnail-list',
  'clay-thumbnail-list.c',
  include_directories: rootInclude,
  dependencies: [
    gdk_pixbuf,
  ],
  c_args: '-DG_LOG_DOMAIN="LibClayThumb"',
  link_with: [
    libclaybase,
    libclaythumb,
  ],
  install: false,
)
