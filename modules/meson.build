
modules_deps = [
  gtk3, babl, gegl, gexiv2, math,
]

color_selector_libs = [
  libclaybase,
  libclaycolor,
  libclayconfig,
  libclaymodule,
  libclaywidgets,
]
controller_libs = [
  libclaymodule,
  libclaywidgets,
]
display_filter_libs = [
  libclaybase,
  libclaycolor,
  libclayconfig,
  libclaymodule,
  libclaywidgets,
]

# Name, Sources, deps, link.
modules = [
  {
    'name': 'color-selector-cmyk',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-water',
    'link': color_selector_libs,
  }, {
    'name': 'color-selector-wheel',
    'srcs': [ 'color-selector-wheel.c', 'claycolorwheel.c', ],
    'link': color_selector_libs,
  }, {
    'name': 'display-filter-clip-warning',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-color-blind',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-aces-rrt',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-gamma',
    'link': display_filter_libs,
  }, {
    'name': 'display-filter-high-contrast',
    'link': display_filter_libs,
  },
]

if have_linuxinput
  modules += {
    'name': 'controller-linux-input',
    'srcs': [ 'controller-linux-input.c', 'clayinputdevicestore-gudev.c', ],
    'deps': gudev,
    'link': controller_libs,
  }
endif

if directx.found()
  modules += {
    'name': 'controller-dx-dinput',
    'srcs': [ 'controller-dx-dinput.c', 'clayinputdevicestore-dx.c', ],
    'deps': directx,
    'link': [ controller_libs, ],
    'link-args': [ '-lrpcrt4', ],
  }
endif

if not platform_windows and not platform_osx
  modules += {
    'name': 'controller-midi',
    'deps': alsa,
    'link': controller_libs,
  }
endif


foreach module : modules
  name = module.get('name')
  srcs = module.get('srcs', name + '.c')
  deps = module.get('deps', [])
  link = module.get('link', [])
  link_args = module.get('link-args', [])

  shared_module(name,
    srcs,
    include_directories: rootInclude,
    dependencies: modules_deps + [ deps ],
    link_with: link,
    link_args: link_args,
    install: true,
    install_dir: clayplugindir / 'modules',
  )
endforeach
