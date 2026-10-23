rootAppInclude = include_directories('.')

subdir('actions')
subdir('core')
subdir('dialogs')
subdir('display')
subdir('file')
subdir('file-data')
subdir('gegl')
subdir('gui')
subdir('menus')
subdir('operations')
subdir('paint')
subdir('path')
subdir('pdb')
subdir('plug-in')
subdir('propgui')
subdir('text')
subdir('tools')
subdir('widgets')
subdir('xcf')


# For app/config
app_debug_files = files(
  'clay-debug.c',
  'clay-log.c',
)

# Top-level library

libapp_sources = [
  'app.c',
  'errors.c',
  'claycoreapp.c',
  'clayconsoleapp.c',
  'clay-debug.c',
  'clay-log.c',
  'clay-update.c',
  'clay-version.c',
  'language.c',
  'sanity.c',
  'signals.c',
  'unique.c',
  gitversion_h,
  claydbusservice_gen,
]

if platform_windows
  # for ClayDashboard and ClayBacktrace
  psapi_cflags = [ '-DPSAPI_VERSION=1' ]
  link_args = [ '-lpsapi' ]

else
  psapi_cflags = [ ]
  link_args = []
endif

if platform_osx
  link_args += osx_ldflags
endif

libapp_c_args = [
  '-DG_LOG_DOMAIN="Clay"',
  '-DCLAY_APP_GLUE_COMPILATION',
  psapi_cflags,
]

# Put Clay core in a lib so we can conveniently link against that in test cases
# and main Clay executable. Note that we should not use this static library for
# the console executable as it uses different macro flags.
libapp = static_library('app',
  libapp_sources,
  include_directories: [ rootInclude, rootAppInclude, configInclude, ],
  c_args: libapp_c_args,
  dependencies: [ gdk_pixbuf, gegl, gexiv2, gtk3, ],
)

clayconsole_deps = [
  cairo,
  dbghelp,
  drmingw,
  gdk_pixbuf,
  gegl,
  gexiv2,
  gio,
  gio_specific,
  lcms,
  libbacktrace,
  pangocairo,
  pangoft2,
  rpc,
]

console_libapps = [
    libappcore,
    libappfile,
    libappfiledata,
    libappgegl,
    libappinternalprocs,
    libapplayermodes,
    libapplayermodeslegacy,
    libappoperations,
    libapppaint,
    libapppath,
    libapppdb,
    libappplugin,
    libapptext,
    libappxcf,
]

libapp_dep = declare_dependency(
  dependencies: [
    clayconsole_deps,
    gtk3,
  ],
  link_with: [
    libapp,
    console_libapps,
  ],
  include_directories: [
    rootInclude,
    rootAppInclude,
  ],
  compile_args: psapi_cflags,
  link_args: link_args,
)

# Those subdirs need to link against the first ones
subdir('config')

console_libapps += [ libappconfig ]

app_libclays = [
  libclaybase,
  libclaycolor,
  libclayconfig,
  libclaymath,
  libclaymodule,
  libclaythumb,
]

gui_libapps = [
  console_libapps,
  libappactions,
  libappdialogs,
  libappdisplay,
  libappgui,
  libappmenus,
  libapppropgui,
  libapptools,
  libappwidgets,
  libclaywidgets,
]

# Executables

if platform_windows
  console_rc_name = 'clay-console-'+ clay_app_version
  console_rc_file = windows.compile_resources(
    clay_plugins_rc,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', console_rc_name+'.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', console_rc_name),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )

  gui_rc_name = 'clay-'+ clay_app_version
  gui_rc_file = windows.compile_resources(
    clay_app_rc,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', gui_rc_name+'.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', gui_rc_name),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
else
  console_rc_file = []
  gui_rc_file = []
endif

if enable_console_bin
  clayconsole_exe = executable(clayconsole_exe_name,
    'main.c',
    libapp_sources,
    console_rc_file,
    c_args: [ libapp_c_args, '-DCLAY_CONSOLE_COMPILATION', ],
    dependencies: clayconsole_deps,
    link_with: [
      app_libclays,
      console_libapps
    ],
    include_directories: [
      rootInclude,
      rootAppInclude,
    ],
    win_subsystem: 'console',
    link_args: link_args,
    install: true,
  )
endif

claymain_exe = executable(claymain_exe_name,
  'main.c',
  gui_rc_file,
  c_args: libapp_c_args,
  dependencies: libapp_dep,
  link_with: [
    app_libclays,
    gui_libapps,
  ],
  win_subsystem: 'windows',
  install: true,
)

if enable_default_bin
  if not platform_windows
    install_symlink(fs.name(claymain_exe.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(claymain_exe.full_path()),
                    install_dir: get_option('bindir'))
    install_symlink(fs.name(claymain_exe.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(claymain_exe.full_path()),
                    install_dir: get_option('bindir'))
  else
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             claymain_exe.full_path(), fs.name(claymain_exe.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             claymain_exe.full_path(), fs.name(claymain_exe.name()).replace(exec_ver, '-@0@.exe'), clay_mutex_version) #see the rationale on main meson.build
  endif
  if enable_console_bin
    if not platform_windows
      install_symlink(fs.name(clayconsole_exe.full_path()).replace(exec_ver, ''),
                      pointing_to: fs.name(clayconsole_exe.full_path()),
                      install_dir: get_option('bindir'))
      install_symlink(fs.name(clayconsole_exe.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                      pointing_to: fs.name(clayconsole_exe.full_path()),
                      install_dir: get_option('bindir'))
    else
      meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                               clayconsole_exe.full_path(), fs.name(clayconsole_exe.name()).replace(exec_ver, '.exe'))
      meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                               clayconsole_exe.full_path(), fs.name(clayconsole_exe.name()).replace(exec_ver, '-@0@.exe'), clay_mutex_version) #see the rationale on main meson.build
    endif
  endif
endif
