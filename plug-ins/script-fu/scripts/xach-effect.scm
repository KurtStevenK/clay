; xyz.kainzmayer.ClayDesign
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; xach effect script
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Xach Beane <xach@mint.net>
;
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


(define (script-fu-xach-effect image
                               drawables
                               hl-offset-x
                               hl-offset-y
                               hl-color
                               hl-opacity-comp
                               ds-color
                               ds-opacity
                               ds-blur
                               ds-offset-x
                               ds-offset-y
                               keep-selection)
  (let* (
        (drawable (vector-ref drawables 0))
        (ds-blur (max ds-blur 0))
        (ds-opacity (min ds-opacity 100))
        (ds-opacity (max ds-opacity 0))
        (type (car (clay-drawable-type-with-alpha drawable)))
        (image-width (car (clay-image-get-width image)))
        (hl-opacity (list hl-opacity-comp hl-opacity-comp hl-opacity-comp))
        (image-height (car (clay-image-get-height image)))
        (active-selection 0)
        (from-selection 0)
        (theLayer 0)
        (hl-layer 0)
        (shadow-layer 0)
        (mask 0)
        )

    (clay-context-push)
    (clay-context-set-defaults)

    (clay-image-undo-group-start image)
    (clay-layer-add-alpha drawable)

    (if (= (car (clay-selection-is-empty image)) TRUE)
        (begin
          (clay-image-select-item image CHANNEL-OP-REPLACE drawable)
          (set! active-selection (car (clay-selection-save image)))
          (set! from-selection FALSE))
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (clay-selection-save image)))))

    (set! hl-layer (car (clay-layer-new image _"Highlight" image-width image-height type 100 LAYER-MODE-NORMAL)))
    (clay-image-insert-layer image hl-layer 0 -1)

    (clay-selection-none image)
    (clay-drawable-edit-clear hl-layer)
    (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)

    (clay-context-set-background hl-color)
    (clay-drawable-edit-fill hl-layer FILL-BACKGROUND)
    (clay-selection-translate image hl-offset-x hl-offset-y)
    (clay-drawable-edit-fill hl-layer FILL-BACKGROUND)
    (clay-selection-none image)
    (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)

    (set! mask (car (clay-layer-create-mask hl-layer ADD-MASK-WHITE)))
    (clay-layer-add-mask hl-layer mask)

    (clay-context-set-background hl-opacity)
    (clay-drawable-edit-fill mask FILL-BACKGROUND)

    (set! shadow-layer (car (clay-layer-new image
                                            _"Shadow"
                                            image-width
                                            image-height
                                            type
                                            ds-opacity
                                            LAYER-MODE-NORMAL)))
    (clay-image-insert-layer image shadow-layer 0 -1)
    (clay-selection-none image)
    (clay-drawable-edit-clear shadow-layer)
    (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (clay-selection-translate image ds-offset-x ds-offset-y)
    (clay-context-set-background ds-color)
    (clay-drawable-edit-fill shadow-layer FILL-BACKGROUND)
    (clay-selection-none image)
    (clay-drawable-merge-new-filter shadow-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" (* 0.32 ds-blur) "std-dev-y" (* 0.32 ds-blur) "filter" "auto")
    (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (clay-drawable-edit-clear shadow-layer)
    (clay-image-lower-item image shadow-layer)

    (if (= keep-selection FALSE)
        (clay-selection-none image))

    (clay-image-set-selected-layers image (vector drawable))
    (clay-image-remove-channel image active-selection)
    (clay-image-undo-group-end image)
    (clay-displays-flush)

    (clay-context-pop)
  )
)

(script-fu-register-filter "script-fu-xach-effect"
  _"_Xach-Effect..."
  _"Add a subtle translucent 3D effect to the selected region (or alpha)"
  "Adrian Likins <adrian@gimp.org>"
  "Adrian Likins"
  "9/28/97"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Highlight X offset"      '(-1 -100 100 1 10 0 1)
  SF-ADJUSTMENT _"Highlight Y offset"      '(-1 -100 100 1 10 0 1)
  SF-COLOR      _"Highlight color"         "white"
  SF-ADJUSTMENT _"Highlight opacity"       '(66 0 255 1 10 0 0)
  SF-COLOR      _"Drop shadow color"       "black"
  SF-ADJUSTMENT _"Drop shadow opacity"     '(100 0 100 1 10 0 0)
  SF-ADJUSTMENT _"Drop shadow blur radius" '(12 0 255 1 10 0 1)
  SF-ADJUSTMENT _"Drop shadow X offset"    '(5 0 255 1 10 0 1)
  SF-ADJUSTMENT _"Drop shadow Y offset"    '(5 0 255 1 10 0 1)
  SF-TOGGLE     _"Keep selection"          TRUE
)

(script-fu-menu-register "script-fu-xach-effect"
                         "<Image>/Filters/Light and Shadow/[Shadow]")
