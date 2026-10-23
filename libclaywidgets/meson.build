stamp_widgets_enums = custom_target('stamp-claywidgetsenums.h',
  input : [
    files(
      'claywidgetsenums.h'
    ),
  ],
  output: [ 'stamp-claywidgetsenums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'claywidgets',
    '#include <gio/gio.h>\n'              +
    '#include "libclaybase/claybase.h"\n',
    '#include "libclay/libclay-intl.h"',
    libclay_mkenums_dtails
  ],
  build_by_default: true
)

claywidgetsmarshal = gnome.genmarshal('claywidgetsmarshal',
  prefix: '_clay_widgets_marshal',
  sources: 'claywidgetsmarshal.list',
  install_header: false,
)

libclaywidgets_sources_introspectable = files(
  'claybrowser.c',
  'claybusybox.c',
  'claybutton.c',
  'claycairo-utils.c',
  'claycellrenderercolor.c',
  'claycellrenderertoggle.c',
  'claychainbutton.c',
  'claycolorarea.c',
  'claycolorbutton.c',
  'claycolordisplay.c',
  'claycolordisplaystack.c',
  'claycolorhexentry.c',
  'claycolornotebook.c',
  'claycolorprofilechooserdialog.c',
  'claycolorprofilecombobox.c',
  'claycolorprofilestore.c',
  'claycolorprofileview.c',
  'claycolorscale.c',
  'claycolorscaleentry.c',
  'claycolorscales.c',
  'claycolorselect.c',
  'claycolorselection.c',
  'claycolorselector.c',
  'claydialog.c',
  'clayenumcombobox.c',
  'clayenumlabel.c',
  'clayenumstore.c',
  'clayenumwidgets.c',
  'clayfilechooser.c',
  'clayframe.c',
  'clayhelpui.c',
  'clayhintbox.c',
  'clayicons.c',
  'clayintcombobox.c',
  'clayintradioframe.c',
  'clayintstore.c',
  'claylabelcolor.c',
  'claylabeled.c',
  'claylabelintwidget.c',
  'claylabelspin.c',
  'claylabelstringwidget.c',
  'claylabelentry.c',
  'claymemsizeentry.c',
  'claynumberpairentry.c',
  'clayoffsetarea.c',
  'claypageselector.c',
  'claypatheditor.c',
  'claypickbutton.c',
  'claypreview.c',
  'claypreviewarea.c',
  'claypropwidgets.c',
  'clayquerybox.c',
  'clayruler.c',
  'clayscaleentry.c',
  'clayscrolledpreview.c',
  'claysizeentry.c',
  'clayspinbutton.c',
  'clayspinscale.c',
  'claystringcombobox.c',
  'clayunitcombobox.c',
  'clayunitstore.c',
  'claywidgets-error.c',
  'claywidgets.c',
  'claywidgetsutils.c',
  'clayzoommodel.c',
)

libclaywidgets_sources = [
  libclaywidgets_sources_introspectable,
  'claycontroller.c',
  'clayeevl.c',
  'clayfileentry.c',
  'claywidgets-private.c',

  'claywidgetsenums.c',
  stamp_widgets_enums,

  claywidgetsmarshal,
  icons_imgs_sources,
  cursors_sources,
  pickers_sources,
]

libclaywidgets_headers_introspectable = files(
  'claybrowser.h',
  'claybusybox.h',
  'claybutton.h',
  'claycairo-utils.h',
  'claycellrenderercolor.h',
  'claycellrenderertoggle.h',
  'claychainbutton.h',
  'claycolorarea.h',
  'claycolorbutton.h',
  'claycolordisplay.h',
  'claycolordisplaystack.h',
  'claycolorhexentry.h',
  'claycolornotebook.h',
  'claycolorprofilechooserdialog.h',
  'claycolorprofilecombobox.h',
  'claycolorprofilestore.h',
  'claycolorprofileview.h',
  'claycolorscale.h',
  'claycolorscaleentry.h',
  'claycolorscales.h',
  'claycolorselect.h',
  'claycolorselection.h',
  'claycolorselector.h',
  'claydialog.h',
  'clayenumcombobox.h',
  'clayenumlabel.h',
  'clayenumstore.h',
  'clayenumwidgets.h',
  'clayfilechooser.h',
  'clayframe.h',
  'clayhelpui.h',
  'clayhintbox.h',
  'clayicons.h',
  'clayintcombobox.h',
  'clayintradioframe.h',
  'claylabelcolor.h',
  'claylabeled.h',
  'claylabelintwidget.h',
  'claylabelspin.h',
  'claylabelentry.h',
  'claylabelstringwidget.h',
  'clayintstore.h',
  'claymemsizeentry.h',
  'claynumberpairentry.h',
  'clayoffsetarea.h',
  'claypageselector.h',
  'claypatheditor.h',
  'claypickbutton.h',
  'claypreview.h',
  'claypreviewarea.h',
  'claypropwidgets.h',
  'clayquerybox.h',
  'clayruler.h',
  'clayscaleentry.h',
  'clayscrolledpreview.h',
  'claysizeentry.h',
  'clayspinbutton.h',
  'clayspinscale.h',
  'claystringcombobox.h',
  'clayunitcombobox.h',
  'clayunitstore.h',
  'claywidgets-error.h',
  'claywidgets.h',
  'claywidgetsenums.h',
  'claywidgetstypes.h',
  'claywidgetsutils.h',
  'clayzoommodel.h',
)

libclaywidgets_headers = [
  libclaywidgets_headers_introspectable,
  # The ClayController API is still considered experimental and can only
  # be run with a special macro CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION set.
  'claycontroller.h',
]

libclaywidgets_introspectable = [
  libclaywidgets_sources_introspectable,
  libclaywidgets_headers_introspectable,
]

if platform_osx
  libclaywidgets_sources += [
    'claypickbutton-quartz.c',
  ]
elif platform_windows
  libclaywidgets_sources += [
    'claypickbutton-win32.c',
  ]
else
  libclaywidgets_sources += [
    'claypickbutton-kwin.c',
    'claypickbutton-xdg.c',
  ]
endif

#Needed otherwise defcheck.py on libclay/meson.build fails
libclaywidgets_def = configure_file(
  input: 'claywidgets.def',
  output: 'claywidgets.def',
  copy: true
)

libclaywidgets = library('claywidgets-'+ clay_api_version,
  libclaywidgets_sources,
  include_directories: rootInclude,
  dependencies: [
    gegl, gexiv2, gtk3, lcms, math, mscms, screencapturekit
  ],
  c_args: [ '-DG_LOG_DOMAIN="LibClayWidgets"', '-DCLAY_WIDGETS_COMPILATION', ],
  link_with: [
    libclaybase,
    libclaycolor,
    libclayconfig,
  ],
  vs_module_defs: libclaywidgets_def,
  install: true,
  version: so_version,
  darwin_versions: darwin_versions,
)

install_headers(
  libclaywidgets_headers,
  subdir: clay_api_name / 'libclaywidgets',
)


# Test programs, not installed

test_preview_area = executable('test-preview-area',
  'test-preview-area.c',
  include_directories: rootInclude,
  dependencies: [
    gtk3,
  ],
  c_args: '-DG_LOG_DOMAIN="LibClayWidgets"',
  link_with: [ libclaywidgets, ],
  install: false,
  build_by_default: false,
)

test_eevl = executable('test-eevl',
  'test-eevl.c',
  include_directories: rootInclude,
  dependencies: [
    glib, gtk3, math,
  ],
  c_args: '-DG_LOG_DOMAIN="LibClayWidgets"',
  link_with: [ libclaywidgets, ],
  install: false,
)
