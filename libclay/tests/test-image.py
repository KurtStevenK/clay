#!/usr/bin/env python3

NEW_IMAGE_WIDTH=1920
NEW_IMAGE_HEIGHT=2001

image = Clay.Image.new(NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, Clay.ImageBaseType.RGB)
clay_assert('Clay.Image.new()', type(image) == Clay.Image)

images = Clay.get_images()
clay_assert('Clay.get_images()', len(images) == 1 and images[0] == image)

text_layer = Clay.TextLayer.new(image, "hello world", Clay.context_get_font(), 20, Clay.Unit.point())
clay_assert('Clay.TextLayer.new() with point unit', type(text_layer) == Clay.TextLayer)

clay_assert('Clay.Image.InsertLayer()', image.insert_layer(text_layer, None, 0))

text_layer = Clay.TextLayer.new(image, "hello world", Clay.context_get_font(), 20, Clay.Unit.pixel())
clay_assert('Clay.TextLayer.new() with pixel unit', type(text_layer) == Clay.TextLayer)
clay_assert('Clay.Image.InsertLayer()', image.insert_layer(text_layer, None, 0))
