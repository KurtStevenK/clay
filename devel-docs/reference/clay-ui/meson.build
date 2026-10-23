# Extra markdown files
clay_ui_doc_content_files = [
  'widget-gallery.md',
]

clay_ui_doc_toml = configure_file(
  input: 'clay-ui-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'CLAY_VERSION': clay_version,
    'CLAY_LOGO': '../images/clay-devel-logo.png',
  },
)

clay_ui_docs = custom_target('clay-ui-docs',
  input: libclayui_gir[0],
  output: 'libclayui-@0@'.format(clay_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', clay_ui_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--add-include-path=@0@'.format(meson.project_build_root() / 'libclay'),
    '--add-include-path=@0@'.format(get_option('prefix') / 'share' / 'gir-1.0'),
    '@INPUT@',
  ],
  depends: libclay_gir[0],
  depend_files: [
    clay_ui_doc_toml,
    clay_ui_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc' / 'clay-' + clay_api_version,
)
