desktopfilename = clay_desktop_name + '.desktop'

desktop_conf = configuration_data()
desktop_conf.set('CLAY_COMMAND',      clay_command)
desktop_conf.set('CLAY_VERSION',      clay_version)
desktop_conf.set('CLAY_APP_VERSION',  clay_app_version)
desktop_conf.set('CLAY_DESKTOP_NAME', clay_desktop_name)
desktop_conf.set('MIME_TYPES',        'image/x-xcf;' + MIMEtypes_dep.get_variable('MIMEtypes'))
desktop_conf.set('BUG_REPORT_URL',    bug_report_url)

desktopfilein = configure_file(
  input : 'clay.desktop.in.in',
  output: desktopfilename+'.in',
  configuration: desktop_conf,
)

desktopfile = i18n.merge_file(
  input : desktopfilein,
  output: desktopfilename,
  type: 'desktop',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'applications',
)

appdatafilename = clay_desktop_name + '.appdata.xml'
appdatafilein = configure_file(
  input : 'org.clay.Clay.appdata.xml.in.in',
  output: appdatafilename+'.in',
  configuration: desktop_conf,
)
appdatafile = i18n.merge_file(
  input : appdatafilein,
  output: appdatafilename,
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'metainfo',
)

if desktop_validate.found()
  test('validate-desktop',
    desktop_validate,
    args: [ desktopfile ],
    suite: 'desktop',
  )
endif

if appstreamcli.found()
  test('appdata_file',
    find_program('test-appdata.py', required: true, dirs: [meson.current_source_dir()]),
    env: [
      'CLAY_TESTING_BUILDDIR=' + meson.current_build_dir(),
      'CLAY_RELEASE=' + (release and not clay_rc_git ? '1' : '0'),
    ],
    suite: 'desktop',
  )
endif
