subdir('dynamics')
subdir('environ')
subdir('gradients')
subdir('interpreters')
subdir('palettes')
subdir('tags')
subdir('tips')
subdir('tool-presets')


meson.add_install_script(python, '-c',
  'from pathlib import Path; import sys, os; destdir = os.environ.get("MESON_INSTALL_DESTDIR_PREFIX"); (Path(destdir) / sys.argv[1] / "fonts").mkdir(parents=True, exist_ok=True)',
  claydatadir
)
release_conf = configuration_data()
release_conf.set('REVISION', get_option('revision'))

clay_release = 'clay-release'
configure_file(
  input : clay_release + '.in',
  output: clay_release,
  configuration: release_conf,
  install: true,
  install_dir: claydatadir,
)
