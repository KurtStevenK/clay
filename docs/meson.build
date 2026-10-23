
# Configure_file is just here to rename the files
manconf = configuration_data()
manconf.set('CLAY_VERSION',   clay_version)
manconf.set('CLAY_APP_VERSION', clay_app_version)
manconf.set('claydatadir',    prefix / claydatadir)
manconf.set('claylocaledir',  prefix / localedir)
manconf.set('clayplugindir',  prefix / clayplugindir)
manconf.set('manpage_claydir', '$XDG_CONFIG_HOME/' + claydir / clay_app_version)
manconf.set('claysysconfdir', prefix / claysysconfdir)
manconf.set('mypaint_brushes_dir', mypaint_brushes_dir)

clayrc_man = custom_target('clayrc-' + clay_app_version + '.5',
                           output: 'clayrc-' + clay_app_version + '.5',
                           depends: [ clay_exe_depends ],
                           command: [ clay_exe, '--no-interface',
                                      '--dump-clayrc-manpage' ],
                           capture: true,
                           build_by_default: true,
                           env: clay_run_env,
                           install: true,
                           install_dir: get_option('mandir') / 'man5')

claytool_man = configure_file(input : 'claytool.1.in',
                              output: 'claytool-' + clay_app_version + '.1',
                              configuration: manconf)
man_files = [ claytool_man ]

if enable_console_bin
  clay_console_man = configure_file(input : 'clay.1.in',
                                    output: 'clay-console-' + clay_app_version + '.1',
                                    configuration: manconf)
  man_files += [ clay_console_man ]
endif

clay_man = configure_file(input : 'clay.1.in',
                          output: 'clay-' + clay_app_version + '.1',
                          configuration: manconf)
man_files += [ clay_man ]

install_man(man_files)

if enable_default_bin and not platform_windows
  install_symlink('claytool.1',
                  pointing_to: 'claytool-' + clay_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
  install_symlink('claytool-@0@.1'.format(api_version_major),
                  pointing_to: 'claytool-' + clay_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')

  install_symlink('clayrc.5',
                  pointing_to: 'clayrc-' + clay_app_version + '.5',
                  install_dir: get_option('mandir') + '/man5')
  install_symlink('clayrc-@0@.5'.format(api_version_major),
                  pointing_to: 'clayrc-' + clay_app_version + '.5',
                  install_dir: get_option('mandir') + '/man5')

  if enable_console_bin
    install_symlink('clay-console.1',
                    pointing_to: 'clay-console-' + clay_app_version + '.1',
                    install_dir: get_option('mandir') + '/man1')
    install_symlink('clay-console-@0@.1'.format(api_version_major),
                    pointing_to: 'clay-console-' + clay_app_version + '.1',
                    install_dir: get_option('mandir') + '/man1')
  endif

  install_symlink('clay.1',
                  pointing_to: 'clay-' + clay_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
  install_symlink('clay-@0@.1'.format(api_version_major),
                  pointing_to: 'clay-' + clay_app_version + '.1',
                  install_dir: get_option('mandir') + '/man1')
endif

# Optional Markdown versions for the website.
pandoc = find_program('pandoc', required: false)
custom_target('Markdown Clay man page',
              input: [ clay_man ],
              output: 'clay.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
custom_target('Markdown clayrc man page',
              input: [ clayrc_man ],
              output: 'clayrc.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
custom_target('Markdown claytool man page',
              input: [ claytool_man ],
              output: 'claytool.md',
              command: [ '../tools/man2md.py', '@INPUT@', '@OUTPUT@', prefix ],
              build_by_default: false)
