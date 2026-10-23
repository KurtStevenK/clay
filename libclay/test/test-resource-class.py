
"""
Python scripts to test libclay API.
Specifically, test ClayResource class and subclasses.

Test methods common to most resource classes, and class methods.
Methods specific to each class are tested elsewhere.

Usage:
=====

This is not a Clay plugin.
Just copy and paste into the Clay Python Console.
Expect no exceptions.
You can quickly look for red text.
There are no messages from  the test program,
only exceptions from assert to indicate a failure.

Requires PyGObject binding to libclay,
which the Clay Python Console does automatically.

Trash:
=====

This test program may leave trash, especially for failed runs.
Trash means resources persisted by Clay.
!!! Do not run this in a production environment (using Clay to create graphics.)
You can run it in a Clay development environment,
but you may need to clean up trash manually.
Deleting the .config/Clay directory will clean up the trash.

Setup in the Clay app:
=====================

Choose current resources to match what this test uses:
e.g. brush '2. Hardness 050'
Use the standard default initial resources for a clean build,
by deleting .config/Clay.

Ensure test resources from prior runs are deleted
otherwise they interfere with testing i.e. name clash.
E.g. delete brush  '2. Hardness 050 copy #1'
"""


"""
Test return value is-a resource.
Get resource from context
"""
brush    = Clay.context_get_brush()
font     = Clay.context_get_font()
gradient = Clay.context_get_gradient()
palette  = Clay.context_get_palette()
pattern  = Clay.context_get_pattern()

assert isinstance(brush,    Clay.Brush)
assert isinstance(font,     Clay.Font)
assert isinstance(gradient, Clay.Gradient)
assert isinstance(palette,  Clay.Palette)
assert isinstance(pattern,  Clay.Pattern)

"""
Test is_valid method
Expect True
"""
assert brush.is_valid()
assert font.is_valid()
assert gradient.is_valid()
assert palette.is_valid()
assert pattern.is_valid()

"""
Test get_id method
Expect string
"""
print( brush.get_id() )
print( font.get_id() )
print( gradient.get_id() )
print( palette.get_id() )
print( pattern.get_id() )

assert brush.get_id()    == '2. Hardness 050'
assert font.get_id()     == 'Sans-serif'
assert gradient.get_id() == 'FG to BG (RGB)'
assert palette.get_id()  == 'Color History'
assert pattern.get_id()  == 'Pine'

"""
Test resource as an arg
Pass the resource back to the context
Expect no exceptions
"""
assert Clay.context_set_brush(brush)        == True
assert Clay.context_set_font(font)          == True
assert Clay.context_set_gradient(gradient)  == True
assert Clay.context_set_palette(palette)    == True
assert Clay.context_set_pattern(pattern)    == True

"""
Test new() methods

Not Font, Pattern.
Pass desired name.
"""
brush_new = Clay.Brush.new("New Brush")
# expect a valid brush object, not a name
assert brush_new.is_valid()
# expect id is the desired one (assuming no clash)
assert brush_new.get_id()=="New Brush"

gradient_new = Clay.Gradient.new("New Gradient")
assert gradient_new.is_valid()
assert gradient_new.get_id()=="New Gradient"

palette_new = Clay.Palette.new("New Palette")
assert palette_new.is_valid()
assert palette_new.get_id()=="New Palette"

"""
Test delete methods.
Delete the new resources we just made
After deletion, reference is invalid
"""
assert brush_new.delete() == True
assert brush_new.is_valid()==False

assert gradient_new.delete() == True
assert gradient_new.is_valid()==False

assert palette_new.delete() == True
assert palette_new.is_valid()==False

"""
Test copy methods.

Copy the original resources from context.
Assert that Clay adds " copy"
"""
brush_copy = brush.duplicate()
assert brush_copy.is_valid()
assert brush_copy.get_id()=='2. Hardness 050 copy'

gradient_copy = gradient.duplicate()
assert gradient_copy.is_valid()
assert gradient_copy.get_id()=='FG to BG (RGB) copy'

palette_copy = palette.duplicate()
assert palette_copy.is_valid()
assert palette_copy.get_id()== 'Color History copy'


"""
Test rename methods.

Renaming returns a reference that must be kept. !!!

Rename existing copy to a desired name.
!!! assign to a new named var
"""
brush_renamed = brush_copy.rename("Renamed Brush")
assert brush_renamed.is_valid()
assert brush_renamed.get_id()=="Renamed Brush"
# assert renaming invalidates any old reference,
assert brush_copy.is_valid()==False

gradient_renamed = gradient_copy.rename("Renamed Gradient")
assert gradient_renamed.is_valid()
assert gradient_renamed.get_id()=="Renamed Gradient"
# assert renaming invalidates any old reference,
assert gradient_copy.is_valid()==False

palette_renamed = palette_copy.rename("Renamed Palette")
assert palette_renamed.is_valid()
assert palette_renamed.get_id()=="Renamed Palette"
# assert renaming invalidates any old reference,
assert palette_copy.is_valid()==False


"""
Test renaming when name is already in use.
If "Renamed Brush" is already in use, then the name will be
"Renamed Brush copy #1"
that is, renaming added suffix "#1"

This is due to core behavior, not libclay resource class per se,
so we don't test it here.
"""

"""
Test delete methods
Delete the renamed copies we just made
After deletion, reference is invalid
"""
brush_renamed.delete()
assert brush_renamed.is_valid()==False

gradient_renamed.delete()
assert gradient_renamed.is_valid()==False

palette_renamed.delete()
assert palette_renamed.is_valid()==False


'''
Test class method

!!! Expect no error dialog in Clay.
The app code generated from foo.pdb should clear the GError.
'''
assert Clay.Brush.id_is_valid   ("invalid name")==False
assert Clay.Font.id_is_valid    ("invalid name")==False
assert Clay.Gradient.id_is_valid("invalid name")==False
assert Clay.Palette.id_is_valid ("invalid name")==False
assert Clay.Pattern.id_is_valid ("invalid name")==False

'''
Test constructor
Not something author's should do, but test it anyway
'''
brush2    = Clay.Brush()
font2     = Clay.Font()
gradient2 = Clay.Gradient()
palette2  = Clay.Palette()
pattern2  = Clay.Pattern()
assert isinstance(brush2,    Clay.Brush)
assert isinstance(font2,     Clay.Font)
assert isinstance(gradient2, Clay.Gradient)
assert isinstance(palette2,  Clay.Palette)
assert isinstance(pattern2,  Clay.Pattern)

# a created instance is invalid until it's ID is set
assert brush2.is_valid()==False
assert font2.is_valid()==False
assert gradient2.is_valid()==False
assert palette2.is_valid()==False
assert pattern2.is_valid()==False

"""
test set_property methods

Use a name that exists.  Creating de novo a reference to an existing brush.
Reference is valid since it's data exists in app core
"""

brush2.set_property("id", "2. Star")
assert brush2.get_id()=="2. Star"
assert brush2.is_valid()

font2.set_property("id", "Sans-serif")
assert font2.get_id()  =="Sans-serif"
assert font2.is_valid()

gradient2.set_property("id", 'FG to BG (RGB)')
assert gradient2.get_id()  =='FG to BG (RGB)'
assert gradient2.is_valid()

palette2.set_property("id", 'Color History')
assert palette2.get_id()  =='Color History'
assert palette2.is_valid()

pattern2.set_property("id", 'Pine')
assert pattern2.get_id()  =='Pine'
assert pattern2.is_valid()
