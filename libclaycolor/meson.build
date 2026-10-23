libclaycolor_sources = files(
  'clayadaptivesupersample.c',
  'claybilinear.c',
  'claycairo.c',
  'claycolor.c',
  'claycolor-parse.c',
  'claycolormanaged.c',
  'claycolorprofile.c',
  'claycolortransform.c',
  'claypixbuf.c',
)

libclaycolor_headers_introspectable = files(
  'claycolortypes.h',
  'clayadaptivesupersample.h',
  'claybilinear.h',
  'claycairo.h',
  'claycolormanaged.h',
  'claycolorprofile.h',
  'claycolortransform.h',
  'claypixbuf.h',

  'claycolor.h',
)

libclaycolor_headers = [
  libclaycolor_headers_introspectable,
]

libclaycolor_introspectable = [
  libclaycolor_sources,
  libclaycolor_headers_introspectable,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaycolor_def = configure_file(
  input: 'claycolor.def',
  output: 'claycolor.def',
  copy: true
)

libclaycolor = library('claycolor-' + clay_api_version,
  libclaycolor_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo, gdk_pixbuf, gegl, gexiv2, lcms, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayColor"', '-DCLAY_COLOR_COMPILATION', ],
  link_with: [ libclaybase, ],
  vs_module_defs: libclaycolor_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaycolor_headers,
  subdir: clay_api_name / 'libclaycolor',
)
