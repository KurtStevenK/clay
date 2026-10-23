#These source files are used by the AppImage, Windows and macOS bundling scripts
#(at build/ subdirs) after making Clay SDK

#https://developer.gimp.org/resource/writing-a-plug-in/tutorial-c-basic/
claytool_libclay_test = configure_file(
    input: 'c-hello-world.c',
    output: 'c-hello-world.c',
    copy: true
  )

#https://developer.gimp.org/resource/writing-a-plug-in/tutorial-c-gui/
claytool_libclayui_test = configure_file(
    input: 'c-hello-world-ui.c',
    output: 'c-hello-world-ui.c',
    copy: true
  )

#https://developer.gimp.org/resource/writing-a-filter/meta-operation/
claytool_libgegl_test = configure_file(
    input: 'clay-tutorial-meta-op.c',
    output: 'clay-tutorial-meta-op.c',
    copy: true
  )
