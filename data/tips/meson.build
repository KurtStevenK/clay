clay_tips = i18n.merge_file(
  input : 'clay-tips.xml.in',
  output: '@BASENAME@',
  data_dirs: meson.current_source_dir(),
  po_dir: potips_dir,
  install: true,
  install_dir: claydatadir / 'tips',
)

fortunes_tips = custom_target('fortunes-tips',
  input : [ clay_tips, 'fortunes.xsl', ],
  output: 'fortunes-tips',
  command: [
    xsltproc,
    '--stringparam', 'lang', 'en',
    '--path', meson.current_source_dir(),
    '--output', '@OUTPUT@',
    '@INPUT1@',
    '@INPUT0@',
  ],
)

if xmllint.found()
  #Copy the DTD to the build directory so xmllint can find it without --path
  #(--path is limited, does not work if the environment have paths with spaces)
  clay_tips_dtd = configure_file(
    input: 'clay-tips.dtd',
    output: 'clay-tips.dtd',
    copy: true
  )

  custom_target('validate-tips',
    command: [
      xmllint,
      '--output', '@OUTPUT@',
      '--valid', clay_tips,
  ],
    depend_files: clay_tips_dtd,
    output: [ 'validate-tips-output.xml' ],
    build_by_default: true,
    install: false
  )
endif
