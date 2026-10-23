apptests_links = [
  libappconfig,
  libappactions,
  libappdialogs,
  libappdisplay,
  libappgui,
  libappmenus,
  libapppropgui,
  libapptools,
  libappwidgets,
  libclaybase,
  libclaycolor,
  libclayconfig,
  libclaymath,
  libclaymodule,
  libclaythumb,
  libclaywidgets,
]


libapptestutils_sources = [
  'clay-app-test-utils.c',
  'clay-test-session-utils.c',
]

libapptestutils = static_library('apptestutils',
  libapptestutils_sources,
  dependencies: libapp_dep,
  link_with: apptests_links,
)

apptests_links += libapptestutils

# TODO: we need to implement proper GUI unit testing, actually based on
# the real Clay and simulating pointer clicks or keyboard inputs the
# proper way.
app_tests = [
  'core',
  'clayidtable',
#'session-2-8-compatibility-multi-window',
#'session-2-8-compatibility-single-window',
#'single-window-mode',
#'tools',
#'ui',
  'xcf',
]

# Prevent parallel builds for the tests
# The tests must not be run in parallel or in a different order as specified

prio = 1000
foreach test_name : app_tests
  test_exe = executable(test_name,
    'test-@0@.c'.format(test_name),
    'tests.c',
    dependencies: [ libapp_dep, appstream ],
    link_with: apptests_links,
  )

  test(test_name,
    test_exe,
    env: [
      'CLAY_TESTING_ABS_TOP_SRCDIR='  + meson.project_source_root(),
      'CLAY_TESTING_ABS_TOP_BUILDDIR='+ meson.project_build_root(),
      'CLAY_TESTING_PLUGINDIRS='      + meson.project_build_root()/'plug-ins'/'common',
      'UI_TEST=yes',
    ],
    suite: 'app',
    timeout: 60,
    is_parallel : false,
    priority: prio,
  )

  prio = prio - 10

endforeach

## Newer Tests using Clay directly ##

if not meson.can_run_host_binaries()
  warning('XCF loading unit testing disabled in cross-building or similar environments.')
  subdir_done()
endif

tests = [
  {
    'name': 'save-and-export',
  }
]

test_env=clay_run_env
test_env.set('CLAY_TESTING_ABS_TOP_SRCDIR', meson.project_source_root())
test_env.set('CLAY_TESTING_ABS_TOP_BUILDDIR', meson.project_build_root())
test_env.append('CLAY_TESTING_PLUG_INS', 'common/file-png', separator: ':')

run_python_test = find_program(meson.project_source_root() / 'libclay/tests/libclay-run-python-test.py')
foreach testinfo : tests
  test_name = testinfo['name']
  basename = 'test-' + testinfo['name']

  py_test  = meson.current_source_dir() / basename + '.py'
  if testinfo.has_key('input')
    test(test_name, run_python_test,
         args: [ clay_exe.full_path(), py_test, testinfo['input']],
         env: test_env,
         suite: ['app'],
         timeout: 90)
  else
    test(test_name, run_python_test,
         args: [ clay_exe.full_path(), py_test ],
         env: test_env,
         suite: ['app'],
         timeout: 90)
  endif
endforeach
