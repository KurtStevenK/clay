clay_tags_default = i18n.merge_file(
  input : 'clay-tags-default.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: potags_dir,
  install: true,
  install_dir: claydatadir / 'tags',
)

if xmllint.found()
  #Copy the DTD to the build directory so xmllint can find it without --path
  #(--path is limited, does not work if the environment have paths with spaces)
  clay_tags_dtd = configure_file(
    input: 'clay-tags.dtd',
    output: 'clay-tags.dtd',
    copy: true
  )

  custom_target('validate-tags',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--valid', clay_tags_default,
    ],
    depend_files: clay_tags_dtd,
    output: [ 'validate-tags-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
