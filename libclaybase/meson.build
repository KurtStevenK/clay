if platform_osx
  add_project_arguments('-ObjC', language : 'c')
endif

clayversion = configure_file(
  input : 'clayversion.h.in',
  output: 'clayversion.h',
  configuration: versionconfig,
)

stamp_base_enums = custom_target('stamp-claybaseenums.h',
  input : [
    files(
      'claybaseenums.h'
    ),
  ],
  output: [ 'stamp-claybaseenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'claybase',
    '#include <glib-object.h>\n'       +
    '#include "claybasetypes.h"\n'     +
    '#include "libclay/libclay-intl.h"\n',
    '',
    libclay_mkenums_dtails
  ],
  build_by_default: true
)

stamp_compat_enums = custom_target('stamp-claycompatenums.h',
  input : [
    files(
      'claycompatenums.h'
    ),
  ],
  output: [ 'stamp-claycompatenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(), meson.current_build_dir(),
    'claycompat',
    '#include <glib-object.h>\n'       +
    '#include "claybasetypes.h"\n',
    '#include "libclay/libclay-intl.h"',
  ],
  build_by_default: true
)

libclaybase_sources_introspectable = files(
  'claybasetypes.c',
  'claychecks.c',
  'claychoice.c',
  'claycpuaccel.c',
  'clayenv.c',
  'clayexportoptions.c',
  'claymemsize.c',
  'claymetadata.c',
  'clayparamspecs.c',
  'clayparasite.c',
  'clayparasiteio.c',
  'clayrectangle.c',
  'clayunit.c',
  'clayutils.c',
  'clayvaluearray.c',
)

libclaybase_sources = [
  libclaybase_sources_introspectable,
  'claybase-private.c',
  'clayprotocol.c',
  'clayreloc.c',
  'claysignal.c',
  'claywire.c',

  'claybaseenums.c',
  stamp_base_enums,

  'claycompatenums.c',
  stamp_compat_enums
]

libclaybase_headers_introspectable = files(
  'claybaseenums.h',
  'claybasetypes.h',
  'claychecks.h',
  'claychoice.h',
  'claycpuaccel.h',
  'clayenv.h',
  'clayexportoptions.h',
  'claylimits.h',
  'claymemsize.h',
  'claymetadata.h',
  'clayparamspecs.h',
  'clayparasite.h',
  'clayparasiteio.h',
  'clayrectangle.h',
  'clayunit.h',
  'clayutils.h',
  'clayvaluearray.h',
) + [
  clayversion,
]

libclaybase_headers = [
  libclaybase_headers_introspectable,
  'claybase.h',
  clayversion,
]

libclaybase_introspectable = [
  libclaybase_sources_introspectable,
  libclaybase_headers_introspectable,
]

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaybase_def = configure_file(
  input: 'claybase.def',
  output: 'claybase.def',
  copy: true
)

libclaybase = library('claybase-' + clay_api_version,
  libclaybase_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gio, math,
    # optionally depend on libexecinfo on platforms where it is not
    # internal to the libc.
    opt_execinfo,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibClayBase"',
    '-DCLAY_BASE_COMPILATION',
    '-Wno-deprecated-declarations',
  ],
  vs_module_defs: libclaybase_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaybase_headers,
  subdir: clay_api_name / 'libclaybase',
)

# Test program, not installed
executable('test-cpu-accel',
  'test-cpu-accel.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
  ],
  c_args: [
    '-DG_LOG_DOMAIN="LibClayBase"',
    '-DCLAY_BASE_COMPILATION',
  ],
  link_with: [
    libclaybase,
  ],
  install: false,
)
