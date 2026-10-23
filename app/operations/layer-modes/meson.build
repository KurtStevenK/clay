libapplayermodes_composite = simd.check('clayoperationlayermode-composite-simd',
  sse2: 'clayoperationlayermode-composite-sse2.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_normal = simd.check('clayoperationnormal-simd',
  sse2: 'clayoperationnormal-sse2.c',
  sse41: 'clayoperationnormal-sse4.c',
  compiler: cc,
  include_directories: [ rootInclude, rootAppInclude, ],
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
  ],
)

libapplayermodes_sources = files(
  'clay-layer-modes.c',
  'clayoperationantierase.c',
  'clayoperationbehind.c',
  'clayoperationdissolve.c',
  'clayoperationerase.c',
  'clayoperationlayermode-blend.c',
  'clayoperationlayermode-composite.c',
  'clayoperationlayermode.c',
  'clayoperationmerge.c',
  'clayoperationnormal.c',
  'clayoperationpassthrough.c',
  'clayoperationoverwrite.c',
  'clayoperationreplace.c',
  'clayoperationsplit.c',
)

libapplayermodes = static_library('applayermodes',
  libapplayermodes_sources,
  link_with: [
    libapplayermodes_composite[0],
    libapplayermodes_normal[0],
  ],
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Clay-Layer-Modes"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
    gexiv2
  ],
)
