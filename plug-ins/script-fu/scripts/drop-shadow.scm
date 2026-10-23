; xyz.kainzmayer.ClayDesign
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
;
;
; drop-shadow.scm   version 1.05   2011/4/21
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - fixed the problem with a remaining copy of the selection
; 1.02 - some code cleanup, no real changes
; 1.03 - can't call clay-drawable-edit-fill until layer is added to image!
; 1.04
; 1.05 - replaced deprecated function calls with new ones for 2.8
;
; Copyright (C) 1997-1999 Sven Neumann <sven@gimp.org>
;
;
; Adds a drop-shadow of the current selection or alpha-channel.
;
; This script is derived from my script add-shadow, which has become
; obsolete now. Thanks to Andrew Donkin (ard@cs.waikato.ac.nz) for his
; idea to add alpha-support to add-shadow.


(define (script-fu-drop-shadow image
                               drawables
                               shadow-transl-x
                               shadow-transl-y
                               shadow-blur
                               shadow-color
                               shadow-opacity
                               allow-resize)
  (let* (
        (drawable (vector-ref drawables 0))
        (shadow-blur (max shadow-blur 0))
        (shadow-opacity (min shadow-opacity 100))
        (shadow-opacity (max shadow-opacity 0))
        (type (car (clay-drawable-type-with-alpha drawable)))
        (image-width (car (clay-image-get-width image)))
        (image-height (car (clay-image-get-height image)))
        (from-selection 0)
        (active-selection 0)
        (shadow-layer 0)
        )

  (clay-context-push)
  (clay-context-set-defaults)

  (clay-image-set-selected-layers image (make-vector 1 drawable))

  (clay-image-undo-group-start image)

  (clay-layer-add-alpha drawable)
  (if (= (car (clay-selection-is-empty image)) TRUE)
      (begin
        (clay-image-select-item image CHANNEL-OP-REPLACE drawable)
        (set! from-selection FALSE))
      (begin
        (set! from-selection TRUE)
        (set! active-selection (car (clay-selection-save image)))))

  (let* ((selection-bounds (clay-selection-bounds image))
         (select-offset-x (cadr selection-bounds))
         (select-offset-y (caddr selection-bounds))
         (select-width (- (cadr (cddr selection-bounds)) select-offset-x))
         (select-height (- (caddr (cddr selection-bounds)) select-offset-y))

         (shadow-width (+ select-width (* 2 shadow-blur)))
         (shadow-height (+ select-height (* 2 shadow-blur)))

         (shadow-offset-x (- select-offset-x shadow-blur))
         (shadow-offset-y (- select-offset-y shadow-blur)))

    (if (= allow-resize TRUE)
        (let* ((new-image-width image-width)
               (new-image-height image-height)
               (image-offset-x 0)
               (image-offset-y 0))

          (if (< (+ shadow-offset-x shadow-transl-x) 0)
              (begin
                (set! image-offset-x (- 0 (+ shadow-offset-x
                                             shadow-transl-x)))
                (set! shadow-offset-x (- 0 shadow-transl-x))
                (set! new-image-width (+ new-image-width image-offset-x))))

          (if (< (+ shadow-offset-y shadow-transl-y) 0)
              (begin
                (set! image-offset-y (- 0 (+ shadow-offset-y
                                             shadow-transl-y)))
                (set! shadow-offset-y (- 0 shadow-transl-y))
                (set! new-image-height (+ new-image-height image-offset-y))))

          (if (> (+ (+ shadow-width shadow-offset-x) shadow-transl-x)
                 new-image-width)
              (set! new-image-width
                    (+ (+ shadow-width shadow-offset-x) shadow-transl-x)))

          (if (> (+ (+ shadow-height shadow-offset-y) shadow-transl-y)
                 new-image-height)
              (set! new-image-height
                    (+ (+ shadow-height shadow-offset-y) shadow-transl-y)))

          (clay-image-resize image
                             new-image-width
                             new-image-height
                             image-offset-x
                             image-offset-y)
        )
    )

    (set! shadow-layer (car (clay-layer-new image
                                            "Drop Shadow"
                                            shadow-width
                                            shadow-height
                                            type
                                            shadow-opacity
                                            LAYER-MODE-NORMAL)))
    (clay-image-set-selected-layers image (make-vector 1 drawable))
    (clay-image-insert-layer image shadow-layer 0 -1)
    (clay-layer-set-offsets shadow-layer
                            shadow-offset-x
                            shadow-offset-y))

  (clay-drawable-fill shadow-layer FILL-TRANSPARENT)
  (clay-context-set-background shadow-color)
  (clay-drawable-edit-fill shadow-layer FILL-BACKGROUND)
  (clay-selection-none image)
  (clay-layer-set-lock-alpha shadow-layer FALSE)
  (if (>= shadow-blur 1.0)
    (clay-drawable-merge-new-filter shadow-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 shadow-blur)
                                                                                               "std-dev-y" (* 0.32 shadow-blur)
                                                                                               "filter" "auto")
    )
  (clay-item-transform-translate shadow-layer shadow-transl-x shadow-transl-y)

  (if (= from-selection TRUE)
      (begin
        (clay-image-select-item image CHANNEL-OP-REPLACE active-selection)
        (clay-drawable-edit-clear shadow-layer)
        (clay-image-remove-channel image active-selection)))

  (if (and
       (= (car (clay-layer-is-floating-sel drawable)) 0)
       (= from-selection FALSE))
      (clay-image-raise-item image drawable))

  (clay-image-set-selected-layers image (make-vector 1 drawable))
  (clay-image-undo-group-end image)
  (clay-displays-flush)

  (clay-context-pop)
  )
)

(script-fu-register-filter "script-fu-drop-shadow"
  _"_Drop Shadow (legacy)..."
  _"Add a drop shadow to the selected region (or alpha)"
  "Sven Neumann <sven@gimp.org>"
  "Sven Neumann"
  "1999/12/21"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Offset X"       '(4 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Offset Y"       '(4 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Blur radius"    '(15 0 1024 1 10 0 1)
  SF-COLOR      _"Color"          "black"
  SF-ADJUSTMENT _"Opacity"        '(60 0 100 1 10 0 0)
  SF-TOGGLE     _"Allow resizing" TRUE
)

(script-fu-menu-register "script-fu-drop-shadow"
                         "<Image>/Filters/Light and Shadow/[Shadow]")
