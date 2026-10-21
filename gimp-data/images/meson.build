crop_conf = configuration_data()
crop_conf.set('VARS', fs.read('gimp-splash.crop'))

## Splash Image and Welcome Dialog ##

# The startup splash is the finished PNG. Rebuilding it from the
# upstream XCF would put the old nebula back.
install_data('gimp-splash.png', install_dir: gimpdatadir / 'images')

## Windows Installer's Splash Image and Intro Banner ##

if get_option('windows-installer')
  data_env = gimp_run_env
  data_env.append('GIMP_TESTING_PLUG_INS', 'common/file-compressor', separator: ':')
  data_env.append('GIMP_TESTING_PLUG_INS', 'file-bmp', separator: ':')
  splash2installer_py = configure_file(
    input: 'splash2installer.py.in',
    output: 'splash2installer.py',
    configuration: crop_conf
  )
  installsplash = custom_target('installsplash.bmp',
                                input : [ splash2installer_py ],
                                output: [ 'stamp-installsplash.bmp', ],
                                depends: [ gimp_exe_depends ],
                                command: [ gimp_exe, '-nidfs',
                                           files('gimp-splash.xcf.xz'),
                                           '--batch-interpreter', 'python-fu-eval',
                                           '-b', '-', '--quit'],
                                feed: true,
                                console: true, #needed to not get disordered and lost on fast ninja output and
                                               #to spot very hard-to-debug stuckness (see #11385 and #16202)
                                build_by_default: true,
                                env: data_env)
endif

subdir('logo')

install_data('sidrat.data', install_dir: gimpdatadir / 'images')
