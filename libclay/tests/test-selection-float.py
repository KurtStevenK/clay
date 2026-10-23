#!/usr/bin/env python3

image = Clay.Image.new(32,32,Clay.ImageBaseType.RGB)
clay_assert('Clay.Image.new', image is not None)

layer1 = Clay.Layer.new(image, "layer1", 20, 10,
                        Clay.ImageType.RGBA_IMAGE, 100.0,
                        Clay.LayerMode.NORMAL)
image.insert_layer(layer1,None,0)

group1 = Clay.GroupLayer.new(image, None)
image.insert_layer(group1,None,-1)

layer2 = Clay.Layer.new(image, "layer2", 10, 20,
                        Clay.ImageType.RGBA_IMAGE, 100.0,
                        Clay.LayerMode.NORMAL)

clay_assert('insert layer inside group', image.insert_layer(layer2,group1,-1) == True)

# Make floating selection

# 1. Fail with no selection
clay_assert('Clay.Selection.float - no selection',
            Clay.Selection.float(image,[layer1],10,10) is None)

# 2. Fail on a group layer
clay_assert('Clay.Selection.float - group layer',
            Clay.Selection.float(image,[group1],10,10) is None)

# Create a selection
image.select_rectangle(Clay.ChannelOps.REPLACE, 5, 5, 20, 20)

# 3. Succeed on a normal layer
clay_assert('take selected layers: layer1',
            image.take_selected_layers([layer1]) is True)
float1 = Clay.Selection.float(image,[layer1],10,10)
clay_assert('Clay.Selection.float - normal layer',
            float1 is not None)
clay_assert('Remove float1 layer from image',
            Clay.floating_sel_remove(float1) is True)

# Create a selection
image.select_rectangle(Clay.ChannelOps.REPLACE, 5, 5, 20, 20)

# 4. Succeed on a layer inside a group
clay_assert('take selected layers: layer2',
            image.take_selected_layers([layer2]) is True)
sel_drawables = image.get_selected_drawables()
clay_assert('selected drawables', sel_drawables is not None)
float2 = Clay.Selection.float(image,sel_drawables,10,10)
clay_assert('Clay.Selection.float - layer inside group', float2 is not None)
clay_assert('Remove float2 layer from image',
            Clay.floating_sel_remove(float2) is True)
