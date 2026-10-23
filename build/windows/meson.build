# Windows specific

clay_plugins_rc_configured = configure_file(
  input : 'clay-plug-ins.rc.in',
  output: 'clay-plug-ins.rc.configured',
  configuration: versionconfig,
)

clay_app_rc_configured = configure_file(
  input : 'clay.rc.in',
  output: 'clay.rc.configured',
  configuration: versionconfig,
)

# The copy depends on git-version.h so that header exists before windres
# runs. The configured .rc is the real file; windres includes git-version.h
# from it. See GNOME/gimp#6257.

clay_plugins_rc = custom_target('clay_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  input: clay_plugins_rc_configured,
  command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['clay-plug-ins.rc']
)

clay_app_rc = custom_target('clay_app_rc',
  build_by_default: true,
  build_always_stale: true,
  input: clay_app_rc_configured,
  command: [python, '-c', 'import shutil, sys; shutil.copy(sys.argv[1], sys.argv[2])', '@INPUT@', '@OUTPUT@'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['clay.rc']
)
