extension_name = 'xyz.kainzmayer.clay.extension.mcp-bridge'
plugin_script = 'clay-mcp-bridge.py'

install_data(
  plugin_script,
  install_dir: clayplugindir / 'extensions' / extension_name,
  install_mode: 'rwxr-xr-x',
)

conf = configuration_data()
conf.set('PLUGIN_SCRIPT', plugin_script)

appdatafilename = extension_name + '.metainfo.xml'
appdatafilein = configure_file(
  input : appdatafilename + '.in.in',
  output: appdatafilename + '.in',
  configuration: conf,
)

i18n.merge_file(
  input : [ appdatafilein, ],
  output: appdatafilename,
  po_dir: po_plug_ins_dir,
  install: true,
  install_dir: clayplugindir / 'extensions' / extension_name,
)
