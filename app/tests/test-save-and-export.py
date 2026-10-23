#!/usr/bin/env python3

# NOTE: these tests were originally mostly supposed to be GUI tests, but
# the early tests were much too flimsy. And to be fair, they were not
# proper GUI tests either, as they would hack-call some core functions,
# but without actually loading the full process and clicking on buttons
# or actually interacting with the interface.
# These replacement tests are further away since they use the public API
# (but this one does use core code too!).
#
# Now the ideal implementation would use dedicated GUI testing
# frameworks, which actually simulate human interactions with GUI
# applications through automated scripts. When we'll have these, we may
# replace the below tests.

tmpdir = Clay.temp_directory()

# Tests that the URIs are correct for a newly created image.
image = Clay.Image.new(800, 600, Clay.ImageBaseType.RGB)
#Clay.Display.new(image)
clay_assert("New image has no file associated", image.get_xcf_file() is None)
clay_assert("New image has no imported file", image.get_imported_file() is None);
clay_assert("New image has no exported file", image.get_exported_file() is None);
image.delete()


# Tests that ClayImage URIs are correct for an XCF file that has just been opened.
path  = os.path.join(os.environ['CLAY_TESTING_ABS_TOP_SRCDIR'],
                     'app/tests/files/clay-2-6-file.xcf')
clay_assert("XCF file to load exists", GLib.file_test(path, GLib.FileTest.EXISTS))
file  = Gio.File.new_for_path(path)
image = Clay.file_load(Clay.RunMode.NONINTERACTIVE, file)
clay_assert("Loaded XCF's file equals the loaded file", image.get_xcf_file().equal(file));
clay_assert("Loaded XCF has no imported file", image.get_imported_file() is None);
clay_assert("Loaded XCF has no exported file", image.get_exported_file() is None);


# Tests that the URIs are correct for an image that has been imported and then saved.
path = os.path.join(tmpdir, 'clay-test.xcf')
saved_file = Gio.File.new_for_path(path)
clay_assert(f"Making sure {path} does not exists", not GLib.file_test(path, GLib.FileTest.EXISTS))
Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, saved_file, None)
clay_assert("XCF file was properly saved", GLib.file_test(path, GLib.FileTest.EXISTS))
clay_assert("Saved XCF's file is correct", image.get_xcf_file().equal(saved_file));
clay_assert("Saved XCF has no imported file", image.get_imported_file() is None);
clay_assert("Saved XCF has no exported file", image.get_exported_file() is None);


# Tests that the URIs for an exported, newly created file are correct.
path = os.path.join(tmpdir, 'clay-test.png')
exported_file  = Gio.File.new_for_path(path)
clay_assert(f"Making sure {path} does not exists", not GLib.file_test(path, GLib.FileTest.EXISTS))
Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, exported_file, None)
clay_assert("PNG file was properly saved", GLib.file_test(path, GLib.FileTest.EXISTS))
clay_assert("Exported image's file still equals the saved file", image.get_xcf_file().equal(saved_file));
clay_assert("Exported image has no imported file", image.get_imported_file() is None);
clay_assert("Exported image's exported file is correct", image.get_exported_file().equal(exported_file));

saved_file.delete()
exported_file.delete()
image.delete()


# Tests that URIs are correct for an imported image.
path  = os.path.join(os.environ['CLAY_TESTING_ABS_TOP_BUILDDIR'],
                     'clay-data/images/logo/clay64x64.png')
clay_assert("Image file to import exists", GLib.file_test(path, GLib.FileTest.EXISTS))
file  = Gio.File.new_for_path(path)
image = Clay.file_load(Clay.RunMode.NONINTERACTIVE, file)
clay_assert("Imported image has no file associated", image.get_xcf_file() is None)
clay_assert("Imported image's imported file equals the loaded file", image.get_imported_file().equal(file));
clay_assert("Imported image has no exported file", image.get_exported_file() is None);


# Tests that after a XCF file was imported then exported, the import URI is cleared.
# An image can not be considered both imported and exported at the same time.
clay_assert(f"Making sure {exported_file.peek_path()} does not exists",
            not GLib.file_test(exported_file.peek_path(), GLib.FileTest.EXISTS))
Clay.file_save(Clay.RunMode.NONINTERACTIVE, image, exported_file, None)
clay_assert("Imported then exported image has no file associated", image.get_xcf_file() is None)
clay_assert("Imported then exported image has no imported file", image.get_imported_file() is None)
clay_assert("Imported then exported image has a correct exported file", image.get_exported_file().equal(exported_file))

exported_file.delete()
image.delete()
