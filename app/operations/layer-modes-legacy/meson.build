libapplayermodeslegacy_sources = [
  'claycolor-legacy.c',
  'clayoperationadditionlegacy.c',
  'clayoperationburnlegacy.c',
  'clayoperationdarkenonlylegacy.c',
  'clayoperationdifferencelegacy.c',
  'clayoperationdividelegacy.c',
  'clayoperationdodgelegacy.c',
  'clayoperationgrainextractlegacy.c',
  'clayoperationgrainmergelegacy.c',
  'clayoperationhardlightlegacy.c',
  'clayoperationhslcolorlegacy.c',
  'clayoperationhsvhuelegacy.c',
  'clayoperationhsvsaturationlegacy.c',
  'clayoperationhsvvaluelegacy.c',
  'clayoperationlightenonlylegacy.c',
  'clayoperationmultiplylegacy.c',
  'clayoperationscreenlegacy.c',
  'clayoperationsoftlightlegacy.c',
  'clayoperationsubtractlegacy.c',
]

libapplayermodeslegacy = static_library('applayermodeslegacy',
  libapplayermodeslegacy_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Layer-Modes-Legacy"',
  dependencies: [
    cairo, gegl, gdk_pixbuf, gexiv2
  ],
)
