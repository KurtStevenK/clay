devel_docs_build_root = meson.current_build_dir()

scan_args_common = [
  '--deprecated-guards=CLAY_DISABLE_DEPRECATED',
]

mkdb_args_common = [
  '--name-space=clay',
]

if gi_docgen.found() and have_gobject_introspection
  subdir('reference')
endif
