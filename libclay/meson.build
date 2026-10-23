# Similarly to libclaythumb/claythumb-enums.c, libclay/clayenums.c is
# not versioned so we don't use the mkenums_wrap.
# Moreover it is generated in 2 steps, first with common mkenums
# arguments, then concatenating with a "tail" file.
clayenums_notail = custom_target('clayenums.c.notail',
  input : [ 'clayenums.h', stamp_enumcode],
  output: [ 'clayenums.c.notail', ],
  command: [
    clay_mkenums,
    '--fhead','#include "config.h"\n'
            + '#include <gio/gio.h>\n'
            + '#include <gegl.h>\n'
            + '#include "libclaybase/claybase.h"\n'
            + '#include "libclaybase/claybase-private.h"\n'
            + '#include "libclayconfig/clayconfigenums.h"\n'
            + '#include "pdb/stamp-enumcode.h"\n'
            + '#include "clayenums.h"\n',
    '--fprod',
      '/* enumerations from "@basename@" */',
    '--vhead',
      'GType\n'+
      '@enum_name@_get_type (void)\n'+
      '{\n'+
      '  static const G@Type@Value values[] =\n'+
      '  {',
    '--vprod',
      '    { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
    '--vtail',
      '    { 0, NULL, NULL }\n'+
      '  };\n',
    '--dhead',
      '  static const Clay@Type@Desc descs[] =\n'+
      '  {',
    '--dprod',
      '    { @VALUENAME@, @valuedesc@, @valuehelp@ },'+
      '@if (\'@valueabbrev@\' ne \'NULL\')@\n'+
      '    /* Translators: this is an abbreviated version of @valueudesc@.\n'+
      '       Keep it short. */\n'+
      '    { @VALUENAME@, @valueabbrev@, NULL },'+
      '@endif@',
    '--dtail',
      libclay_mkenums_dtails,
    '@INPUT0@',
  ],
  capture: true,
)

clayenums = custom_target('clayenums.c',
  input : [ clayenums_notail, 'clayenums.c.tail', ],
  output: [ 'clayenums.c', ],
  command: [python, '-c', 'import sys; [sys.stdout.write(open(f).read()) for f in sys.argv[1:]]','@INPUT@'],
  capture: true,
)

clayuimarshal = gnome.genmarshal('clayuimarshal',
  prefix: '_clayui_marshal',
  sources: 'clayuimarshal.list',
  install_header: false,
)

pdb_wrappers_sources = [
  'clay_pdb.c',
  'claybrush_pdb.c',
  'claybrushes_pdb.c',
  'claybrushselect_pdb.c',
  'claybuffer_pdb.c',
  'claychannel_pdb.c',
  'claycontext_pdb.c',
  'claydebug_pdb.c',
  'claydisplay_pdb.c',
  'claydrawable_pdb.c',
  'claydrawablecolor_pdb.c',
  'claydrawableedit_pdb.c',
  'claydrawablefilter_pdb.c',
  'claydrawableselect_pdb.c',
  'claydynamics_pdb.c',
  'clayedit_pdb.c',
  'clayfile_pdb.c',
  'clayfloatingsel_pdb.c',
  'clayfont_pdb.c',
  'clayfonts_pdb.c',
  'clayfontselect_pdb.c',
  'clayclayrc_pdb.c',
  'claygradient_pdb.c',
  'claygradients_pdb.c',
  'claygradientselect_pdb.c',
  'claygrouplayer_pdb.c',
  'clayhelp_pdb.c',
  'clayimage_pdb.c',
  'clayimageautocrop_pdb.c',
  'clayimagecolorprofile_pdb.c',
  'clayimageconvert_pdb.c',
  'clayimagegrid_pdb.c',
  'clayimageguides_pdb.c',
  'clayimagesamplepoints_pdb.c',
  'clayimageselect_pdb.c',
  'clayimagetransform_pdb.c',
  'clayimageundo_pdb.c',
  'clayitem_pdb.c',
  'clayitemtransform_pdb.c',
  'claylayer_pdb.c',
  'claylinklayer_pdb.c',
  'claymessage_pdb.c',
  'claypainttools_pdb.c',
  'claypalette_pdb.c',
  'claypalettes_pdb.c',
  'claypaletteselect_pdb.c',
  'claypath_pdb.c',
  'claypattern_pdb.c',
  'claypatterns_pdb.c',
  'claypatternselect_pdb.c',
  'clayprogress_pdb.c',
  'clayrasterizable_pdb.c',
  'clayresource_pdb.c',
  'clayselection_pdb.c',
  'claytextlayer_pdb.c',
  'claytexttool_pdb.c',
  'clayunit_pdb.c',
  'clayvectorlayer_pdb.c',
]

pdb_wrappers_headers = [
  'clay_pdb_headers.h',
  'clay_pdb.h',
  'claybrush_pdb.h',
  'claybrushes_pdb.h',
  'claybrushselect_pdb.h',
  'claybuffer_pdb.h',
  'claychannel_pdb.h',
  'claycontext_pdb.h',
  'claydebug_pdb.h',
  'claydisplay_pdb.h',
  'claydrawable_pdb.h',
  'claydrawablecolor_pdb.h',
  'claydrawableedit_pdb.h',
  'claydrawablefilter_pdb.h',
  'claydrawableselect_pdb.h',
  'claydynamics_pdb.h',
  'clayedit_pdb.h',
  'clayfile_pdb.h',
  'clayfloatingsel_pdb.h',
  'clayfont_pdb.h',
  'clayfonts_pdb.h',
  'clayfontselect_pdb.h',
  'clayclayrc_pdb.h',
  'claygradient_pdb.h',
  'claygradients_pdb.h',
  'claygradientselect_pdb.h',
  'claygrouplayer_pdb.h',
  'clayhelp_pdb.h',
  'clayimage_pdb.h',
  'clayimageautocrop_pdb.h',
  'clayimagecolorprofile_pdb.h',
  'clayimageconvert_pdb.h',
  'clayimagegrid_pdb.h',
  'clayimageguides_pdb.h',
  'clayimagesamplepoints_pdb.h',
  'clayimageselect_pdb.h',
  'clayimagetransform_pdb.h',
  'clayimageundo_pdb.h',
  'clayitem_pdb.h',
  'clayitemtransform_pdb.h',
  'claylayer_pdb.h',
  'claylinklayer_pdb.h',
  'claymessage_pdb.h',
  'claypainttools_pdb.h',
  'claypalette_pdb.h',
  'claypalettes_pdb.h',
  'claypaletteselect_pdb.h',
  'claypath_pdb.h',
  'claypattern_pdb.h',
  'claypatterns_pdb.h',
  'claypatternselect_pdb.h',
  'clayprogress_pdb.h',
  'clayrasterizable_pdb.h',
  'clayresource_pdb.h',
  'clayselection_pdb.h',
  'claytextlayer_pdb.h',
  'claytexttool_pdb.h',
  'clayunit_pdb.h',
  'clayvectorlayer_pdb.h',
]

libclay_sources_introspectable = [
  'clay.c',
  'claybatchprocedure.c',
  'claybrush.c',
  'claychannel.c',
  'claycurve.c',
  'claydisplay.c',
  'claydrawable.c',
  'claydrawablefilter.c',
  'claydrawablefilterconfig.c',
  'clayexportoptions.c',
  'clayfileprocedure.c',
  'clayfont.c',
  'clayclayrc.c',
  'claygradient.c',
  'claygrouplayer.c',
  'clayimage.c',
  'clayimagecolorprofile.c',
  'clayimagemetadata.c',
  'clayimagemetadata-save.c',
  'clayimageprocedure.c',
  'clayitem.c',
  'claylayer.c',
  'claylayermask.c',
  'claylinklayer.c',
  'clayloadprocedure.c',
  'claypalette.c',
  'clayparamspecs.c',
  'claypath.c',
  'claypattern.c',
  'claypdb.c',
  'clayplugin.c',
  'clayprocedure.c',
  'clayprocedure-params.c',
  'clayprocedureconfig.c',
  'clayprogress.c',
  'clayrasterizable.c',
  'clayresource.c',
  'clayexportprocedure.c',
  'clayselection.c',
  'claytextlayer.c',
  'claythumbnailprocedure.c',
  'clayvectorlayer.c',
  'clayvectorloadprocedure.c',
  clayenums,
  pdb_wrappers_sources,
]

libclay_sources = [
  libclay_sources_introspectable,
  'clay-debug.c',
  'clay-shm.c',
  'claygpparams.c',
  'clayparamspecs-desc.c',
  'claypdb_pdb.c',
  'claypdbprocedure.c',
  'claypixbuf.c',
  'clayplugin_pdb.c',
  'clayresourceselect.c',
  'claytilebackendplugin.c',
]

libclay_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'clay.h',
  'claytypes.h',
  'clayenums.h',

  # Other headers
  'claybatchprocedure.h',
  'claybrush.h',
  'claychannel.h',
  'claycurve.h',
  'claydisplay.h',
  'claydrawable.h',
  'claydrawablefilter.h',
  'claydrawablefilterconfig.h',
  'clayexportoptions.h',
  'clayfileprocedure.h',
  'clayfont.h',
  'clayclayrc.h',
  'claygradient.h',
  'claygrouplayer.h',
  'clayimage.h',
  'clayimagecolorprofile.h',
  'clayimagemetadata.h',
  'clayimageprocedure.h',
  'clayitem.h',
  'claylayer.h',
  'claylayermask.h',
  'claylinklayer.h',
  'clayloadprocedure.h',
  'claypalette.h',
  'clayparamspecs.h',
  'claypath.h',
  'claypattern.h',
  'claypdb.h',
  'clayplugin.h',
  'clayprocedure.h',
  'clayprocedure-params.h',
  'clayprocedureconfig.h',
  'clayprogress.h',
  'clayrasterizable.h',
  'clayresource.h',
  'clayexportprocedure.h',
  'clayselection.h',
  'claytextlayer.h',
  'claythumbnailprocedure.h',
  'clayvectorlayer.h',
  'clayvectorloadprocedure.h',
  pdb_wrappers_headers,
]

libclay_headers = [
  libclay_headers_introspectable,
]

libclayui_sources_introspectable = [
  'clayaspectpreview.c',
  'claybrushchooser.c',
  'claydrawablechooser.c',
  'claydrawablepreview.c',
  'clayfontchooser.c',
  'claygradientchooser.c',
  'clayimagechooser.c',
  'clayimagecombobox.c',
  'clayitemchooser.c',
  'clayitemcombobox.c',
  'claypalettechooser.c',
  'claypatternchooser.c',
  'clayprocbrowserdialog.c',
  'clayproceduredialog.c',
  'clayprocview.c',
  'clayprogressbar.c',
  'claypropwidgets.c',
  'clayresourcechooser.c',
  'clayexportproceduredialog.c',
  'clayui.c',
  'clayzoompreview.c',
  'clayvectorloadproceduredialog.c',
]

libclayui_sources = [
  libclayui_sources_introspectable,
  clayuimarshal,

  # TODO: eventually move to libclaywidgets.
  'clayresolutionentry-private.c'
]

libclayui_headers_introspectable = [
  # Order matters. Leave these headers at the top.
  'clayui.h',
  'clayuitypes.h',

  # Other headers
  'clayaspectpreview.h',
  'claybrushchooser.h',
  'claydrawablechooser.h',
  'claydrawablepreview.h',
  'clayfontchooser.h',
  'claygradientchooser.h',
  'clayimagechooser.h',
  'clayimagecombobox.h',
  'clayitemchooser.h',
  'clayitemcombobox.h',
  'claypalettechooser.h',
  'claypatternchooser.h',
  'clayprocbrowserdialog.h',
  'clayproceduredialog.h',
  'clayprocview.h',
  'clayprogressbar.h',
  'claypropwidgets.h',
  'clayresourcechooser.h',
  'clayexportproceduredialog.h',
  'clayvectorloadproceduredialog.h',
  'clayzoompreview.h',
]

libclay_introspectable = [
  libclay_sources_introspectable,
  libclay_headers_introspectable,
]

libclayui_introspectable = [
  libclayui_sources_introspectable,
  libclayui_headers_introspectable,
]


if glib.version().version_compare('>=2.86')
  if not platform_windows
    gio_specific_gir = 'GioUnix-2.0'
    gio_specific_vapi = 'gio-unix-2.0'
  else
    gio_specific_gir = 'GioWin32-2.0'
    gio_specific_vapi = 'gio-windows-2.0'
  endif
else
  gio_specific_gir = ''
  gio_specific_vapi = ''
endif

if glib.version().version_compare('>=2.87.3')
  if not platform_windows
    glib_specific_gir = 'GLibUnix-2.0'
  else
    glib_specific_gir = 'GLibWin32-2.0'
  endif
else
  glib_specific_gir = ''
endif

libclay_deps_table = [
    { 'gir': 'Babl-0.1',          'vapi': 'babl-0.1',        },
    { 'gir': 'cairo-1.0',         'vapi': 'cairo-1.0',       },
    { 'gir': 'GdkPixbuf-2.0',     'vapi': 'gdk-pixbuf-2.0',  },
    { 'gir': 'Gegl-0.4',          'vapi': 'gegl-0.4',        },
    { 'gir': 'GExiv2-0.16',       'vapi': 'gexiv2-0.16',     },
    { 'gir': 'Gio-2.0',           'vapi': 'gio-2.0',         },
    { 'gir': gio_specific_gir,    'vapi': gio_specific_vapi, },
    { 'gir': 'GLib-2.0',          'vapi': 'glib-2.0',        },
    { 'gir': glib_specific_gir,   'vapi': 'glib-2.0',        },
    { 'gir': 'GObject-2.0',       'vapi': 'gobject-2.0',     },
    { 'gir': 'Pango-1.0',         'vapi': 'gtk+-3.0',        },
  ]

#Needed otherwise defcheck.py fails
libclay_def = configure_file(
  input: 'clay.def',
  output: 'clay.def',
  copy: true
)


libclay = library('clay-'+ clay_api_version,
  libclay_sources,
  include_directories: rootInclude,
  dependencies: [
    cairo,
    gdk_pixbuf,
    gegl,
    gexiv2,
    drmingw,
    math,
    pango,
    pangoft2,
  ],
  sources: [
    pdbgen
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClay"', '-DCLAY_COMPILATION', '-Wno-deprecated-declarations', ],
  link_with: [
    libclaybase,
    libclaycolor,
    libclayconfig,
    libclaymodule,
  ],
  vs_module_defs: libclay_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

# Define an internal dependency for ease of reuse
libclay_dep = declare_dependency(
  dependencies: [
    cairo,
    gdk_pixbuf,
    gegl,
    gexiv2,
  ],
  link_with: [
    libclay,
    libclaybase,
    libclaycolor,
    libclayconfig,
    libclaymath,
    libclaymodule,
  ],
  include_directories: rootInclude,
)


#Needed otherwise defcheck.py fails
libclayui_def = configure_file(
  input: 'clayui.def',
  output: 'clayui.def',
  copy: true
)

libclayui = library('clayui-'+ clay_api_version,
  libclayui_sources,
  dependencies: [
    libclay_dep,
    gtk3,
  ],
  sources: [
    pdbgen
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayUI"', '-DCLAY_COMPILATION', ],
  link_with: [
    libclaywidgets,
  ],
  vs_module_defs: libclayui_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

# Define an internal dependency for ease of reuse
libclayui_dep = declare_dependency(
  dependencies: [
    libclay_dep,
    gtk3,
  ],
  link_with: [
    libclayui,
    libclaywidgets,
  ],
  include_directories: rootInclude,
)


install_headers(
  libclay_headers,
  libclayui_headers_introspectable,
  subdir: clay_api_name / 'libclay',
)



### GObject introspection

libclay_introspectable_files = [
  libclay_introspectable,
  libclaybase_introspectable,
  libclaycolor_introspectable,
  libclayconfig_introspectable,
  libclaymath_introspectable,
  libclaymodule_introspectable,
  'clayparamspecs-body.c'
]

libclayui_introspectable_files = [
  libclayui_introspectable,
  libclaywidgets_introspectable,
]

if meson.is_cross_build() and not get_option('can-crosscompile-gir')
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while cross-compiling because',
    'GI tools do not manage to properly generate introspection data',
    'while cross-compiling. Nevertheless we consider our introspected',
    'API as a core component of Clay, thus you are expected to build',
    'this data natively.',
    'To forcefully build with GObject Introspection, set:',
    '-Dcan-crosscompile-gir=true'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
elif get_option('b_sanitize') != 'none'
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is disabled while building with a sanitizer.',
    'See: https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/375'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
elif get_option('default_library') == 'static'
  have_gobject_introspection = false
  gobject_introspection_warning = '\n      '.join(['',
    'GObject Introspection is not supported with static libraries.',
    'See: https://gitlab.gnome.org/GNOME/gobject-introspection/-/issues/187'
  ])
  warning(gobject_introspection_warning)
  warnings += gobject_introspection_warning
else
  have_gobject_introspection = true
  libclay_gir_includes = []
  foreach dep : libclay_deps_table
    if dep['gir'] != ''
      libclay_gir_includes += [dep['gir']]
    endif
  endforeach

  # Silence 'duplicate -rpath' and other noisy warnings from Xcode 15+
  # See: https://github.com/mesonbuild/meson/commit/17d1cc60ed8246b8e7f0786421bf1cdf5cb19254
  gir_env = environment()
  if platform_osx
    gir_env.append('CFLAGS', '-Wno-unused-command-line-argument', separator: ' ')
    gir_env.append('LDFLAGS', '-Wl,-w', separator: ' ')
  endif

  gir_args = [
    '-DCLAY_COMPILATION',
    '-DCLAY_BASE_COMPILATION',
    '-DCLAY_COLOR_COMPILATION',
    '-DCLAY_CONFIG_COMPILATION',
    '-DCLAY_MATH_COMPILATION',
    '-DCLAY_MODULE_COMPILATION',
    '-DCLAY_THUMB_COMPILATION',
    '-DCLAY_WIDGETS_COMPILATION',
    '--quiet',
  ]

  if meson.version().version_compare('>=1.2')
    libclay_gir = gnome.generate_gir(
      libclay,
      libclaybase,
      libclaycolor,
      libclayconfig,
      libclaymath,
      libclaymodule,

      sources: libclay_introspectable_files,
      nsversion: clay_api_version,
      namespace: 'Clay',
      identifier_prefix: 'Clay',
      symbol_prefix: 'clay',
      export_packages: 'clay-' + clay_api_version,
      header: 'libclay/clay.h',
      includes: libclay_gir_includes,
      extra_args: gir_args,
      include_directories: rootInclude,
      install: true,
      env: gir_env,
    )
  else
    libclay_gir = gnome.generate_gir(
      libclay,
      libclaybase,
      libclaycolor,
      libclayconfig,
      libclaymath,
      libclaymodule,

      sources: libclay_introspectable_files,
      nsversion: clay_api_version,
      namespace: 'Clay',
      identifier_prefix: 'Clay',
      symbol_prefix: 'clay',
      export_packages: 'clay-' + clay_api_version,
      header: 'libclay/clay.h',
      includes: libclay_gir_includes,
      extra_args: gir_args,
      include_directories: rootInclude,
      install: true,
    )
  endif

  if platform_osx
    patched_libclay_gir = custom_target('libclay-macos-typelib.stamp',
                            output: [ 'libclay-macos-typelib.stamp' ],
                            depends: [ libclay_gir ],
                            command: [
                              make_typelib, libclay_gir,
                              meson.current_build_dir(), prefix, g_ir_compiler
                            ],
                            build_by_default: true)

    custom_target('libclay-macos-final-typelib.stamp',
                  output: [ 'libclay-macos-final-typelib.stamp' ],
                  depends: [ patched_libclay_gir ],
                  command: [
                    make_typelib, libclay_gir,
                    meson.current_build_dir(), prefix, g_ir_compiler, 'nonreloc'
                  ],
                  build_by_default: true)
  endif

  if meson.version().version_compare('>=1.2')
    libclayui_gir = gnome.generate_gir(
      libclayui,
      libclaywidgets,

      sources: libclayui_introspectable_files,
      nsversion: clay_api_version,
      namespace: 'ClayUi',
      identifier_prefix: 'Clay',
      symbol_prefix: [ 'clay_ui', 'clay' ],
      export_packages: 'clay-ui-' + clay_api_version,
      header: 'libclay/clayui.h',
      includes: [ libclay_gir_includes, 'Gtk-3.0', libclay_gir[0] ],
      extra_args: gir_args,
      include_directories: rootInclude,
      install: true,
      env: gir_env,
    )
  else
    libclayui_gir = gnome.generate_gir(
      libclayui,
      libclaywidgets,

      sources: libclayui_introspectable_files,
      nsversion: clay_api_version,
      namespace: 'ClayUi',
      identifier_prefix: 'Clay',
      symbol_prefix: [ 'clay_ui', 'clay' ],
      export_packages: 'clay-ui-' + clay_api_version,
      header: 'libclay/clayui.h',
      includes: [ libclay_gir_includes, 'Gtk-3.0', libclay_gir[0] ],
      extra_args: gir_args,
      include_directories: rootInclude,
      install: true,
    )
  endif

  if platform_osx
    patched_libclayui_gir = custom_target('libclayui-macos-typelib.stamp',
                                          output: [ 'libclayui-macos-typelib.stamp' ],
                                          depends: [ libclay_gir, libclayui_gir ],
                                          command: [
                                            make_typelib, libclayui_gir,
                                            meson.current_build_dir(), prefix, g_ir_compiler
                                          ],
                                          build_by_default: true)

    custom_target('libclayui-macos-final-typelib.stamp',
                  output: [ 'libclayui-macos-final-typelib.stamp' ],
                  depends: [ patched_libclayui_gir ],
                  command: [
                    make_typelib, libclayui_gir,
                    meson.current_build_dir(), prefix, g_ir_compiler, 'nonreloc'
                  ],
                  build_by_default: true)
  endif

  if have_vala
    vapi_deps = []
    foreach dep : libclay_deps_table
      if dep['vapi'] != ''
        vapi_deps += [dep['vapi']]
      endif
    endforeach

    libclay_vapi = gnome.generate_vapi('clay-' + clay_api_version,
      sources: libclay_gir[0],
      packages: vapi_deps,
      install: true,
    )

    libclayui_vapi = gnome.generate_vapi('clay-ui-' + clay_api_version,
      sources: libclayui_gir[0],
      packages: vapi_deps + [ libclay_vapi ],
      install: true,
    )
  endif
endif

if host_machine.system() != 'emscripten'
  nm = find_program('nm', required: false)

  # Verify .def files for Windows linking (and for macOS too when -Wl,-exported_symbols_list is used).
  # We check this on non-Windows/non-macOS platform (Debian) on CI, and on Windows and macOS itself.
  # (This also checks if .gir files have parity with the library so parity with .def files too)
  # The corresponding gir file must be after all the libs of a given module.
  # As an exception, libclaythumb is not introspected and part of no module.
  custom_target('check-def-files',
    input: [
      libclaybase_def,
      libclaycolor_def,
      libclayconfig_def,
      libclaymath_def,
      libclaymodule_def,
      libclay_def,
      have_gobject_introspection ? libclay_gir : [],
      libclaywidgets_def,
      libclayui_def,
      have_gobject_introspection ? libclayui_gir : [],
      libclaythumb_def,
    ],
    depends: [
      libclaybase,
      libclaycolor,
      libclayconfig,
      libclay,
      libclayui,
      libclaymath,
      libclaymodule,
      libclaythumb,
      libclaywidgets
    ],
    env: nm.found() ? { 'NM': nm.full_path() } : {},
    output: [ 'check-def-files', ],
    command: [
      python, meson.project_source_root() / 'tools' / 'defcheck.py', '@INPUT@',
    ],
    build_by_default: true,
  )
endif
