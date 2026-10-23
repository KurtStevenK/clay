
libclaymath_sources_introspectable = files(
  'claymatrix.c',
  'clayvector.c',
)

libclaymath_sources = [
  libclaymath_sources_introspectable,
]

libclaymath_headers_introspectable = files(
  'claymathtypes.h',
  'claymatrix.h',
  'clayvector.h',
)

libclaymath_headers = [
  libclaymath_headers_introspectable,
  'claymath.h',
]

libclaymath_introspectable = [
  libclaymath_sources_introspectable,
  libclaymath_headers_introspectable,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaymath_def = configure_file(
  input: 'claymath.def',
  output: 'claymath.def',
  copy: true
)

libclaymath = library('claymath-' + clay_api_version,
  libclaymath_sources,
  include_directories: rootInclude,
  dependencies: [
    glib, gobject, math,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayMath"', '-DCLAY_MATH_COMPILATION', ],
  link_with: [
    libclaybase,
  ],
  vs_module_defs: libclaymath_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaymath_headers,
  subdir: clay_api_name / 'libclaymath',
)
