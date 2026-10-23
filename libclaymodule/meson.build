
libclaymodule_sources = files(
  'claymodule.c',
  'claymoduledb.c',
)

libclaymodule_headers = files(
  'claymodule.h',
  'claymoduledb.h',
  'claymoduletypes.h',
)

libclaymodule_introspectable = [
  libclaymodule_sources,
  libclaymodule_headers,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaymodule_def = configure_file(
  input: 'claymodule.def',
  output: 'claymodule.def',
  copy: true
)

libclaymodule = library('claymodule-' + clay_api_version,
  libclaymodule_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gio, glib, gmodule,
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayModule"', '-DCLAY_MODULE_COMPILATION', ],
  link_with: [
    libclaybase,
    libclayconfig,
  ],
  vs_module_defs: libclaymodule_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaymodule_headers,
  subdir: clay_api_name / 'libclaymodule',
)
