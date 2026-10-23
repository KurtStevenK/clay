;
; old-photo
;
;
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
;
; Branko Collin <collin@xs4all.nl> added the possibility to change
; the border size in October 2001.

; Define the function:

(define (script-fu-old-photo inImage inLayer inDefocus inBorderSize inSepia inMottle inCopy)
  (let (
       (theImage (if (= inCopy TRUE) (car (clay-image-duplicate inImage)) inImage))
       (theLayer 0)
       (theWidth 0)
       (theHeight 0)
       )
  (if (= inCopy TRUE)
     (clay-image-undo-disable theImage)
     (clay-image-undo-group-start theImage)
  )

  (clay-selection-all theImage)

  (set! theLayer (car (clay-image-flatten theImage)))
  (if (= inDefocus TRUE)
      (clay-drawable-merge-new-filter theLayer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 0.48 "std-dev-y" 0.48 "filter" "auto")
  )
  (if (> inBorderSize 0)
      (script-fu-fuzzy-border theImage (vector theLayer) '(255 255 255)
                              inBorderSize TRUE 8 FALSE 100 FALSE TRUE )
  )
  (set! theLayer (car (clay-image-flatten theImage)))

  (if (= inSepia TRUE)
      (begin (clay-drawable-merge-new-filter theLayer "clay:desaturate" 0 LAYER-MODE-REPLACE 1.0 "mode" DESATURATE-LIGHTNESS)
             (clay-drawable-merge-new-filter theLayer "clay:brightness-contrast" 0 LAYER-MODE-REPLACE 1.0
                 "brightness" -0.078125
                 "contrast"   -0.15625)
             (clay-drawable-merge-new-filter theLayer "clay:color-balance" 0 LAYER-MODE-REPLACE 1.0
                 "range"               TRANSFER-SHADOWS
                 "cyan-red"            0.3
                 "magenta-green"       0.0
                 "yellow-blue"         -0.3
                 "preserve-luminosity" TRUE)
      )
  )
  (set! theWidth (car (clay-image-get-width theImage)))
  (set! theHeight (car (clay-image-get-height theImage)))
  (if (= inMottle TRUE)
      (let (
	    (mLayer (car (clay-layer-new theImage "Mottle"
                                         theWidth theHeight RGBA-IMAGE
					 100 LAYER-MODE-DARKEN-ONLY)))
	    )

             (clay-image-insert-layer theImage mLayer 0 0)
             (clay-selection-all theImage)
             (clay-drawable-edit-clear mLayer)
             (clay-selection-none theImage)
             (clay-drawable-merge-new-filter mLayer "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                             "independent" TRUE "red" 0.0 "green" 0.0 "blue" 0.0 "alpha" 0.5
                                             "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)
             (clay-drawable-merge-new-filter mLayer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 1.6 "std-dev-y" 1.6 "filter" "auto")
             (set! theLayer (car (clay-image-flatten theImage)))
      )
  )
  (clay-selection-none theImage)

  (if (= inCopy TRUE)
      (begin  (clay-image-clean-all theImage)
              (clay-display-new theImage)
              (clay-image-undo-enable theImage)
      )
      (clay-image-undo-group-end theImage)
  )

  (clay-displays-flush)
  )
)

(script-fu-register-filter "script-fu-old-photo"
  _"_Old Photo..."
  _"Make an image look like an old photo"
  "Chris Gutteridge"
  "1998, Chris Gutteridge / ECS dept, University of Southampton, England."
  "16th April 1998"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-TOGGLE     _"Defocus"      TRUE
  SF-ADJUSTMENT _"Border size"  '(20 0 300 1 10 0 1)
     ; since this plug-in uses the fuzzy-border plug-in, I used the
     ; values of the latter, with the exception of the initial value
     ; and the 'minimum' value.
  SF-TOGGLE     _"Sepia"        TRUE
  SF-TOGGLE     _"Mottle"       FALSE
  SF-TOGGLE     _"Work on copy" TRUE
)

(script-fu-menu-register "script-fu-old-photo"
                         "<Image>/Filters/Decor")
