; Chris Gutteridge (cjg@ecs.soton.ac.uk)
; At ECS Dept, University of Southampton, England.

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


(define (script-fu-tile-blur inImage drawables inRadius inVert inHoriz inType)

  (let* (
        (theImage inImage)
        (theLayer (vector-ref (car (clay-image-get-selected-drawables theImage)) 0))
        (theHeight (car (clay-drawable-get-height theLayer)))
        (theWidth (car (clay-drawable-get-width theLayer)))
        (horizontalRadius (* 0.32 inRadius))
        (verticalRadius (* 0.32 inRadius))
        )

    (if (= inHoriz FALSE)
        (set! horizontalRadius 0)
    )
    (if (= inVert FALSE)
        (set! verticalRadius 0)
    )

    (define (pasteat xoff yoff)
      (let* (
             (pasted (car (clay-edit-paste theLayer FALSE)))
             (num-pasted (vector-length pasted))
             (floating-sel (vector-ref pasted (- num-pasted 1)))
            )
        (clay-layer-set-offsets floating-sel (* xoff theWidth) (* yoff theHeight) )
        (clay-floating-sel-anchor floating-sel)
      )
    )

    (clay-context-push)
    (clay-context-set-feather FALSE)
    (clay-image-undo-group-start theImage)

    (clay-layer-resize theLayer (* 3 theWidth) (* 3 theHeight) 0 0)

    (clay-image-select-rectangle theImage CHANNEL-OP-REPLACE 0 0 theWidth theHeight)
    (clay-edit-cut (vector theLayer))

    (clay-selection-none theImage)
    (clay-layer-set-offsets theLayer theWidth theHeight)

    (pasteat 1 1) (pasteat 1 2) (pasteat 1 3)
    (pasteat 2 1) (pasteat 2 2) (pasteat 2 3)
    (pasteat 3 1) (pasteat 3 2) (pasteat 3 3)

    (clay-selection-none theImage)
    (clay-drawable-merge-new-filter theLayer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0
                                    "std-dev-x" horizontalRadius "std-dev-y" verticalRadius "filter" "auto")

    (clay-layer-resize theLayer
                       theWidth theHeight (- 0 theWidth) (- 0 theHeight))
    (clay-layer-set-offsets theLayer 0 0)
    (clay-image-undo-group-end theImage)
    (clay-displays-flush)
    (clay-context-pop)
  )
)

(script-fu-register-filter "script-fu-tile-blur"
  _"_Tileable Blur..."
  _"Blur the edges of an image so the result tiles seamlessly"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "25th April 1998"
  "RGB*"
  SF-ONE-DRAWABLE
  SF-ADJUSTMENT _"Ra_dius"            '(5 0 128 1 5 0 0)
  SF-TOGGLE     _"Blur _vertically"   TRUE
  SF-TOGGLE     _"Blur _horizontally" TRUE
  SF-OPTION     _"Blur _type"         '(_"IIR" _"RLE")
)

(script-fu-menu-register "script-fu-tile-blur"
                         "<Image>/Filters/Blur")
