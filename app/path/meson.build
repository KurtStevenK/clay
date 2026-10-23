libapppath_sources = [
  'clayanchor.c',
  'claybezierstroke.c',
  'claypath.c',
  'claypath-compat.c',
  'claypath-export.c',
  'claypath-import.c',
  'claypath-preview.c',
  'claypath-warp.c',
  'claypathmodundo.c',
  'claypathpropundo.c',
  'claypathundo.c',
  'claystroke-new.c',
  'claystroke.c',
  'clayvectorlayer.c',
  'clayvectorlayeroptions.c',
  'clayvectorlayerundo.c',
]

libapppath = static_library('apppath',
  libapppath_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Path"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2
  ],
)
