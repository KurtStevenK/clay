libapppropgui_sources = [
  'claypropgui-channel-mixer.c',
  'claypropgui-color-balance.c',
  'claypropgui-color-rotate.c',
  'claypropgui-color-to-alpha.c',
  'claypropgui-convolution-matrix.c',
  'claypropgui-diffraction-patterns.c',
  'claypropgui-eval.c',
  'claypropgui-focus-blur.c',
  'claypropgui-generic.c',
  'claypropgui-hue-saturation.c',
  'claypropgui-motion-blur-circular.c',
  'claypropgui-motion-blur-linear.c',
  'claypropgui-motion-blur-zoom.c',
  'claypropgui-newsprint.c',
  'claypropgui-panorama-projection.c',
  'claypropgui-recursive-transform.c',
  'claypropgui-shadows-highlights.c',
  'claypropgui-spiral.c',
  'claypropgui-supernova.c',
  'claypropgui-utils.c',
  'claypropgui-vignette.c',
  'claypropgui.c',
]

libapppropgui = static_library('apppropgui',
  libapppropgui_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-PropGUI"',
  dependencies: [
    gegl, gtk3, gexiv2
  ],
)
