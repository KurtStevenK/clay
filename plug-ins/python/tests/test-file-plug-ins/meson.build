if not stable or not release or clay_version.endswith('+git') or get_option('file-plug-ins-test')

# This does not get installed for releases
python_plugins += {
  'name': 'test-file-plug-ins',
  'srcs': [
    # modules
    'tests/test-file-plug-ins/claylogger.py',
    'tests/test-file-plug-ins/clayconfig.py',
    'tests/test-file-plug-ins/claytestframework.py',
    'tests/test-file-plug-ins/clayexporttests.py',
    # actual Clay plug-in
    'tests/test-file-plug-ins/test-file-plug-ins.py',
    # scripts to start the tests in batch mode
    'tests/test-file-plug-ins/batch-import-tests.py',
    'tests/test-file-plug-ins/batch-export-tests.py',
  ],
}

# configuration data for the tests
subdir('tests')

endif
