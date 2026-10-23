# Windows specific

configure_file(
  input : 'clay-plug-ins.rc.in',
  output: 'clay-plug-ins.rc',
  configuration: versionconfig,
)

configure_file(
  input : 'clay.rc.in',
  output: 'clay.rc',
  configuration: versionconfig,
)

# Basically, the build rules below do nothing (a mere `python -c 'exit()'` call).
# But because they depends on `git-version.h`, meson ensure that it gets built first,
# Then the result of this targets is used in 35+ resource compiler build rules.
#
# Nasty trick indeed, but it fixes race condition issue described in GNOME/Clay#6257.

clay_plugins_rc = custom_target('clay_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['clay-plug-ins.rc']
)

clay_app_rc = custom_target('clay_app_rc',
  build_by_default: true,
  build_always_stale: true,
  command: [python, '-c', 'exit()'],
  depends: generate_version_h ? [gitversion_h] : [],
  output:  ['clay.rc']
)
