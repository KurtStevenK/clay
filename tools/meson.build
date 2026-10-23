
subdir('claytool-tests')

if platform_windows
  # Each compile_resources() of the same .rc needs its own output name.
  # Meson names the resource target from that filename, and this directory
  # builds more than one executable.
  clay_debug_resume_rc_in = custom_target(
    'clay-debug-resume-rc-in',
    input: clay_plugins_rc,
    output: 'clay-debug-resume.rc',
    command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@'],
  )
  clay_debug_resume_rc = windows.compile_resources(
    clay_debug_resume_rc_in,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', 'clay-debug-resume.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', 'clay-debug-resume'),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
  clay_debug_resume = executable('clay-debug-resume',
    'clay-debug-resume.c',
    clay_debug_resume_rc,
  )

  claytool_rc_in = custom_target(
    'claytool-rc-in',
    input: clay_plugins_rc,
    output: 'claytool.rc',
    command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@'],
  )
  claytool_rc = windows.compile_resources(
    claytool_rc_in,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', 'claytool' + exec_ver + '.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', 'claytool' + exec_ver),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
  clay_test_clipboard_rc_in = custom_target(
    'clay-test-clipboard-rc-in',
    input: clay_plugins_rc,
    output: 'clay-test-clipboard.rc',
    command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@'],
  )
  clay_test_clipboard_rc = windows.compile_resources(
    clay_test_clipboard_rc_in,
    args: [
      rc_define_flag, rc_define_fmt.format('ORIGINALFILENAME_STR', 'clay-test-clipboard' + exec_ver + '.exe'),
      rc_define_flag, rc_define_fmt.format('INTERNALNAME_STR', 'clay-test-clipboard' + exec_ver),
    ],
    include_directories: [
      rootInclude, appInclude,
    ],
  )
else
  clay_debug_resume_rc = []
  claytool_rc = []
  clay_test_clipboard_rc = []
endif


claytool = executable('claytool' + exec_ver,
  'claytool.c',
  claytool_rc,
  include_directories: rootInclude,
  dependencies: [
    gegl, gtk3, gexiv2
  ],
  link_with: [
    libclaybase,
  ],
  c_args: [
    '-DDATADIR="@0@"'.format(get_option('datadir')),
  ],
  install: true,
)

clay_test_clipboard = executable('clay-test-clipboard' + exec_ver,
  'clay-test-clipboard.c',
  clay_test_clipboard_rc,
  include_directories: rootInclude,
  dependencies: [
    gegl, gtk3, gexiv2
  ],
  install: true,
)

if isocodes.found()
  native_gio    = dependency('gio-2.0', native: true)
  gen_languages = executable('gen-languages',
                             'gen-languages.c',
                             '../app/config/clayxmlparser.c',
                             include_directories: [ rootInclude, appInclude ],
                             dependencies: [ native_gio ],
                             c_args: [
                               '-DSRCDIR="@0@"'.format(meson.project_source_root()),
                             ],
                             native: true,
                             install: false)
endif

if enable_default_bin
  if not platform_windows
    install_symlink(fs.name(claytool.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(claytool.full_path()),
                    install_dir: get_option('bindir'))
    install_symlink(fs.name(claytool.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(claytool.full_path()),
                    install_dir: get_option('bindir'))

    install_symlink(fs.name(clay_test_clipboard.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(clay_test_clipboard.full_path()),
                    install_dir: get_option('bindir'))
    install_symlink(fs.name(clay_test_clipboard.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(clay_test_clipboard.full_path()),
                    install_dir: get_option('bindir'))
  else
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             claytool.full_path(), fs.name(claytool.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             claytool.full_path(), fs.name(claytool.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())

    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             clay_test_clipboard.full_path(), fs.name(clay_test_clipboard.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             clay_test_clipboard.full_path(), fs.name(clay_test_clipboard.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())
  endif
endif

executable('kernelgen',
  'kernelgen.c',
  include_directories: rootInclude,
  install: false,
)
