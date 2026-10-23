; xyz.kainzmayer.ClayDesign
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Circuit board effect
; Copyright (c) 1997 Adrian Likins
;
; Generates what looks a little like the back of an old circuit board.
; Looks even better when gradient-mapp'ed with a suitable gradient.
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


(define (script-fu-circuit image
                           drawables
                           mask-size
                           seed
                           remove-bg
                           keep-selection
                           separate-layer)
  (let* (
        (layer (vector-ref drawables 0))
        (type (car (clay-drawable-type-with-alpha layer)))
        (image-width (car (clay-image-get-width image)))
        (image-height (car (clay-image-get-height image)))
        (active-selection 0)
        (from-selection 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (select-width 0)
        (select-height 0)
        (effect-layer 0)
        (active-layer 0)
        )

    (clay-context-push)
    (clay-context-set-defaults)

    (clay-image-undo-group-start image)

    (clay-layer-add-alpha layer)

    (if (= (car (clay-selection-is-empty image)) TRUE)
        (begin
          (clay-image-select-item image CHANNEL-OP-REPLACE layer)
          (set! active-selection (car (clay-selection-save image)))
          (set! from-selection FALSE))
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (clay-selection-save image)))))

    (set! selection-bounds (clay-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))

    (if (= separate-layer TRUE)
        (begin
          (set! effect-layer (car (clay-layer-new image
                                                  _"Effect layer"
                                                  select-width
                                                  select-height
                                                  type
                                                  100
                                                  LAYER-MODE-NORMAL)))

          (clay-image-insert-layer image effect-layer 0 -1)
          (clay-layer-set-offsets effect-layer select-offset-x select-offset-y)
          (clay-selection-none image)
          (clay-drawable-edit-clear effect-layer)
          (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)
          (clay-edit-copy (vector layer))

          (let* (
                 (pasted (car (clay-edit-paste effect-layer FALSE)))
                 (num-pasted (vector-length pasted))
                 (floating-sel (vector-ref pasted (- num-pasted 1)))
                )
           (clay-floating-sel-anchor floating-sel)
          )
          (clay-image-set-selected-layers image (vector effect-layer)))
          (set! effect-layer layer)
    )
    (set! active-layer effect-layer)

    (if (= remove-bg TRUE)
        (clay-context-set-foreground '(0 0 0))
        (clay-context-set-foreground '(14 14 14))
    )

    (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (clay-drawable-merge-new-filter active-layer "gegl:maze" 0 LAYER-MODE-REPLACE 1.0 "x" 5 "y" 5 "tileable" TRUE "algorithm-type" "depth-first"
                                                                                      "seed" seed
                                                                                      "fg-color" (car (clay-context-get-foreground))
                                                                                      "bg-color" (car (clay-context-get-background)))
    (clay-drawable-merge-new-filter active-layer "gegl:oilify" 0 LAYER-MODE-REPLACE 1.0 "mask-radius" (max 1 (/ mask-size 2)) "use-inten" FALSE)
    (clay-drawable-merge-new-filter active-layer "gegl:edge" 0 LAYER-MODE-REPLACE 1.0 "amount" 2.0 "border-behavior" "loop" "algorithm" "sobel")
    (if (= type RGBA-IMAGE)
      (clay-drawable-merge-new-filter active-layer "clay:desaturate" 0 LAYER-MODE-REPLACE 1.0 "mode" DESATURATE-LIGHTNESS))

    (if (and
         (= remove-bg TRUE)
         (= separate-layer TRUE))
        (begin
          (clay-image-select-color image CHANNEL-OP-REPLACE active-layer '(0 0 0))
          (clay-drawable-edit-clear active-layer)))

    (if (= keep-selection FALSE)
        (clay-selection-none image))

    (clay-image-remove-channel image active-selection)
    (clay-image-set-selected-layers image (vector layer))

    (clay-image-undo-group-end image)

    (clay-displays-flush)

    (clay-context-pop)
  )
)

(script-fu-register-filter "script-fu-circuit"
  _"_Circuit..."
  _"Fill the selected region (or alpha) with traces like those on a circuit board"
  "Adrian Likins <adrian@gimp.org>"
  "Adrian Likins"
  "10/17/97"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Oilify mask size" '(17 3 50 1 10 0 1)
  SF-ADJUSTMENT _"Circuit seed"     '(3 1 3000000 1 10 0 1)
  SF-TOGGLE     _"No background (only for separate layer)" FALSE
  SF-TOGGLE     _"Keep selection"   TRUE
  SF-TOGGLE     _"Separate layer"   TRUE
)

(script-fu-menu-register "script-fu-circuit"
                         "<Image>/Filters/Render")
