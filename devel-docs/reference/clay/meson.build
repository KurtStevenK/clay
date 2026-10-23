# Extra markdown files
clay_doc_content_files = [
]

clay_doc_toml = configure_file(
  input: 'clay-3.0.toml.in',
  output: '@BASENAME@',
  configuration: {
    'CLAY_VERSION': clay_version,
    'CLAY_LOGO_PATH': '../images/clay-devel-logo.png',
  },
)

clay_docs = custom_target('clay-docs',
  input: libclay_gir[0],
  output: 'libclay-@0@'.format(clay_api_version),
  command: [
    gi_docgen,
    'generate',
    '--quiet',
    '--fatal-warnings',
    '--config', clay_doc_toml,
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '--add-include-path=@0@'.format(get_option('prefix') / 'share' / 'gir-1.0'),
    '@INPUT@',
  ],
  depend_files: [
    clay_doc_toml,
    clay_doc_content_files,
  ],
  build_by_default: true,
  install: true,
  install_dir: get_option('datadir') / 'doc' / 'clay-' + clay_api_version,
)
