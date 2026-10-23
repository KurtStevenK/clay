if platform_windows or platform_osx
  clay_debug_tool_dir = get_option('bindir')
else
  clay_debug_tool_dir = get_option('libexecdir')
endif


if platform_windows
  clay_debug_tool_rc = windows.compile_resources(
    clay_plugins_rc,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', 'clay-debug-tool' + exec_ver + '.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', 'clay-debug-tool' + exec_ver),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
else
  clay_debug_tool_rc = []
endif

clay_debug_tool = executable('clay-debug-tool' + exec_ver,
  'clay-debug-tool.c',
  clay_debug_tool_rc,
  include_directories: rootInclude,
  dependencies: [
    fontconfig,
    gio,
    gegl,
    gtk3,
  ],
  link_with: [
    libapp,
    libappwidgets,
    libclaybase,
  ],
  win_subsystem: 'windows',
  install: true,
  install_dir: clay_debug_tool_dir
)

if enable_default_bin
  if not platform_windows
    install_symlink(fs.name(clay_debug_tool.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(clay_debug_tool.full_path()),
                    install_dir: clay_debug_tool_dir)
    install_symlink(fs.name(clay_debug_tool.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(clay_debug_tool.full_path()),
                    install_dir: clay_debug_tool_dir)
  else
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             clay_debug_tool.full_path(), fs.name(clay_debug_tool.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             clay_debug_tool.full_path(), fs.name(clay_debug_tool.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())
  endif
endif
