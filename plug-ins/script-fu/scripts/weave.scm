; xyz.kainzmayer.ClayDesign
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Weave script --- make an image look as if it were woven
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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


; Copies the specified rectangle from/to the specified drawable

(define (copy-rectangle img
                        drawable
                        x1
                        y1
                        width
                        height
                        dest-x
                        dest-y)
  (clay-image-select-rectangle img CHANNEL-OP-REPLACE x1 y1 width height)
  (clay-edit-copy (vector drawable))
  (let* (
         (pasted (car (clay-edit-paste drawable FALSE)))
         (num-pasted (vector-length pasted))
         (floating-sel (vector-ref pasted (- num-pasted 1)))
        )
   (clay-layer-set-offsets floating-sel dest-x dest-y)
   (clay-floating-sel-anchor floating-sel)
  )
  (clay-selection-none img))

; Creates a single weaving tile

(define (create-weave-tile ribbon-width
                           ribbon-spacing
                           shadow-darkness
                           shadow-depth)
  (let* ((tile-size (+ (* 2 ribbon-width) (* 2 ribbon-spacing)))
         (darkness (* 255 (/ (- 100 shadow-darkness) 100)))
         (img (car (clay-image-new tile-size tile-size RGB)))
         (drawable (car (clay-layer-new img "Weave tile" tile-size tile-size RGB-IMAGE
                                        100 LAYER-MODE-NORMAL))))

    (clay-image-undo-disable img)
    (clay-image-insert-layer img drawable 0 0)

    (clay-context-set-background '(0 0 0))
    (clay-drawable-edit-fill drawable FILL-BACKGROUND)

    ; Create main horizontal ribbon

    (clay-context-set-foreground '(255 255 255))
    (clay-context-set-background (list darkness darkness darkness))

    (clay-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 0
                                 ribbon-spacing
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 ribbon-width)

    (clay-context-set-gradient-fg-bg-rgb)
    (clay-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 1 0
				      TRUE
				      (/ (+ (* 2 ribbon-spacing) ribbon-width -1) 2) 0
				      0 0)

    ; Create main vertical ribbon

    (clay-image-select-rectangle img
                                 CHANNEL-OP-REPLACE
                                 (+ (* 2 ribbon-spacing) ribbon-width)
                                 0
                                 ribbon-width
                                 (+ (* 2 ribbon-spacing) ribbon-width))

    (clay-drawable-edit-gradient-fill drawable
				      GRADIENT-BILINEAR (- 100 shadow-depth)
				      FALSE 1 0
				      TRUE
				      0 (/ (+ (* 2 ribbon-spacing) ribbon-width -1) 2)
				      0 0)

    ; Create the secondary horizontal ribbon

    (copy-rectangle img
                    drawable
                    0
                    ribbon-spacing
                    (+ ribbon-width ribbon-spacing)
                    ribbon-width
                    (+ ribbon-width ribbon-spacing)
                    (+ (* 2 ribbon-spacing) ribbon-width))

    (copy-rectangle img
                    drawable
                    (+ ribbon-width ribbon-spacing)
                    ribbon-spacing
                    ribbon-spacing
                    ribbon-width
                    0
                    (+ (* 2 ribbon-spacing) ribbon-width))

    ; Create the secondary vertical ribbon

    (copy-rectangle img
                    drawable
                    (+ (* 2 ribbon-spacing) ribbon-width)
                    0
                    ribbon-width
                    (+ ribbon-width ribbon-spacing)
                    ribbon-spacing
                    (+ ribbon-width ribbon-spacing))

    (copy-rectangle img
                    drawable
                    (+ (* 2 ribbon-spacing) ribbon-width)
                    (+ ribbon-width ribbon-spacing)
                    ribbon-width
                    ribbon-spacing
                    ribbon-spacing
                    0)

    ; Done

    (clay-image-undo-enable img)
    (list img drawable)))

; Creates a complete weaving mask

(define (create-weave width
                      height
                      ribbon-width
                      ribbon-spacing
                      shadow-darkness
                      shadow-depth)
  (let* ((tile (create-weave-tile ribbon-width ribbon-spacing shadow-darkness
                                  shadow-depth))
         (tile-img (car tile))
         (tile-layer (cadr tile))
          (weaving (plug-in-tile #:run-mode RUN-NONINTERACTIVE #:image tile-img #:drawables (vector tile-layer) #:new-width width #:new-height height #:new-image TRUE)))
    (clay-image-delete tile-img)
    weaving))

; Creates a single tile for masking

(define (create-mask-tile ribbon-width
                          ribbon-spacing
                          r1-x1
                          r1-y1
                          r1-width
                          r1-height
                          r2-x1
                          r2-y1
                          r2-width
                          r2-height
                          r3-x1
                          r3-y1
                          r3-width
                          r3-height)
  (let* ((tile-size (+ (* 2 ribbon-width) (* 2 ribbon-spacing)))
         (img (car (clay-image-new tile-size tile-size RGB)))
         (drawable (car (clay-layer-new img "Mask" tile-size tile-size RGB-IMAGE
                                        100 LAYER-MODE-NORMAL))))
    (clay-image-undo-disable img)
    (clay-image-insert-layer img drawable 0 0)

    (clay-context-set-background '(0 0 0))
    (clay-drawable-edit-fill drawable FILL-BACKGROUND)

    (clay-image-select-rectangle img CHANNEL-OP-REPLACE r1-x1 r1-y1 r1-width r1-height)
    (clay-image-select-rectangle img CHANNEL-OP-ADD r2-x1 r2-y1 r2-width r2-height)
    (clay-image-select-rectangle img CHANNEL-OP-ADD r3-x1 r3-y1 r3-width r3-height)

    (clay-context-set-background '(255 255 255))
    (clay-drawable-edit-fill drawable FILL-BACKGROUND)
    (clay-selection-none img)

    (clay-image-undo-enable img)

    (list img drawable)))

; Creates a complete mask image

(define (create-mask final-width
                     final-height
                     ribbon-width
                     ribbon-spacing
                     r1-x1
                     r1-y1
                     r1-width
                     r1-height
                     r2-x1
                     r2-y1
                     r2-width
                     r2-height
                     r3-x1
                     r3-y1
                     r3-width
                     r3-height)
  (let* ((tile (create-mask-tile ribbon-width ribbon-spacing
                                 r1-x1 r1-y1 r1-width r1-height
                                 r2-x1 r2-y1 r2-width r2-height
                                 r3-x1 r3-y1 r3-width r3-height))
         (tile-img (car tile))
         (tile-layer (cadr tile))
         (mask (plug-in-tile #:run-mode   RUN-NONINTERACTIVE
                             #:image      tile-img
                             #:drawables  (vector tile-layer)
                             #:new-width  final-width
                             #:new-height final-height
                             #:new-image  TRUE)))
    (clay-image-delete tile-img)
    mask))

; Creates the mask for horizontal ribbons

(define (create-horizontal-mask ribbon-width
                                ribbon-spacing
                                final-width
                                final-height)
  (create-mask final-width
               final-height
               ribbon-width
               ribbon-spacing
               0
               ribbon-spacing
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-width
               0
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-spacing
               ribbon-width
               (+ ribbon-width ribbon-spacing)
               (+ (* 2 ribbon-spacing) ribbon-width)
               (+ ribbon-width ribbon-spacing)
               ribbon-width))

; Creates the mask for vertical ribbons

(define (create-vertical-mask ribbon-width
                              ribbon-spacing
                              final-width
                              final-height)
  (create-mask final-width
               final-height
               ribbon-width
               ribbon-spacing
               (+ (* 2 ribbon-spacing) ribbon-width)
               0
               ribbon-width
               (+ (* 2 ribbon-spacing) ribbon-width)
               ribbon-spacing
               0
               ribbon-width
               ribbon-spacing
               ribbon-spacing
               (+ ribbon-width ribbon-spacing)
               ribbon-width
               (+ ribbon-width ribbon-spacing)))

; Adds a threads layer at a certain orientation to the specified image

(define (create-threads-layer img
                              width
                              height
                              length
                              density
                              orientation)
  (let* ((drawable (car (clay-layer-new img "Threads" width height RGBA-IMAGE
                                        100 LAYER-MODE-NORMAL)))
         (dense (/ density 100.0)))
    (clay-image-insert-layer img drawable 0 -1)
    (clay-context-set-background '(255 255 255))
    (clay-drawable-edit-fill drawable FILL-BACKGROUND)
    (clay-drawable-merge-new-filter drawable "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                    "independent" FALSE "red" dense "alpha" dense
                                    "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)
    (clay-drawable-merge-new-filter drawable "gegl:stretch-contrast" 0 LAYER-MODE-REPLACE 1.0 "keep-colors" FALSE)
    (cond ((eq? orientation 'horizontal)
           (clay-drawable-merge-new-filter drawable "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 length) "std-dev-y" 0.0 "filter" "auto"))
          ((eq? orientation 'vertical)
           (clay-drawable-merge-new-filter drawable "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 0.0 "std-dev-y" (* 0.32 length) "filter" "auto")))
    (clay-drawable-merge-new-filter drawable "gegl:stretch-contrast" 0 LAYER-MODE-REPLACE 1.0 "keep-colors" FALSE)
    drawable))

(define (create-complete-weave width
                               height
                               ribbon-width
                               ribbon-spacing
                               shadow-darkness
                               shadow-depth
                               thread-length
                               thread-density
                               thread-intensity)
  (let* ((weave (create-weave width height ribbon-width ribbon-spacing
                              shadow-darkness shadow-depth))
         (w-img (car weave))
         (w-layer (cadr weave))

         (h-layer (create-threads-layer w-img width height thread-length
                                        thread-density 'horizontal))
         (h-mask (car (clay-layer-create-mask h-layer ADD-MASK-WHITE)))

         (v-layer (create-threads-layer w-img width height thread-length
                                        thread-density 'vertical))
         (v-mask (car (clay-layer-create-mask v-layer ADD-MASK-WHITE)))

         (hmask (create-horizontal-mask ribbon-width ribbon-spacing
                                        width height))
         (hm-img (car hmask))
         (hm-layer (cadr hmask))

         (vmask (create-vertical-mask ribbon-width ribbon-spacing width height))
         (vm-img (car vmask))
         (vm-layer (cadr vmask)))

    (clay-layer-add-mask h-layer h-mask)
    (clay-selection-all hm-img)
    (clay-edit-copy (vector hm-layer))
    (clay-image-delete hm-img)
    (let* (
           (pasted (car (clay-edit-paste h-mask FALSE)))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (clay-floating-sel-anchor floating-sel)
    )
    (clay-layer-set-opacity h-layer thread-intensity)
    (clay-layer-set-mode h-layer LAYER-MODE-MULTIPLY)

    (clay-layer-add-mask v-layer v-mask)
    (clay-selection-all vm-img)
    (clay-edit-copy (vector vm-layer))
    (clay-image-delete vm-img)
    (let* (
           (pasted (car (clay-edit-paste v-mask FALSE)))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (clay-floating-sel-anchor floating-sel)
    )
    (clay-layer-set-opacity v-layer thread-intensity)
    (clay-layer-set-mode v-layer LAYER-MODE-MULTIPLY)

    ; Uncomment this if you want to keep the weaving mask image
    ; (clay-display-new (car (clay-image-duplicate w-img)))

    (list w-img
          (car (clay-image-flatten w-img)))))

; The main weave function

(define (script-fu-weave img
                         drawables
                         ribbon-width
                         ribbon-spacing
                         shadow-darkness
                         shadow-depth
                         thread-length
                         thread-density
                         thread-intensity)
  (clay-context-push)
  (clay-image-undo-group-start img)

  (let* (
        (drawable (vector-ref drawables 0))
        (d-img (car (clay-item-get-image drawable)))
        (d-width (car (clay-drawable-get-width drawable)))
        (d-height (car (clay-drawable-get-height drawable)))
        (d-offsets (clay-drawable-get-offsets drawable))

        (weaving (create-complete-weave d-width
                                        d-height
                                        ribbon-width
                                        ribbon-spacing
                                        shadow-darkness
                                        shadow-depth
                                        thread-length
                                        thread-density
                                        thread-intensity))
        (w-img (car weaving))
        (w-layer (cadr weaving))
        )

    (clay-context-set-paint-mode LAYER-MODE-NORMAL)
    (clay-context-set-opacity 100.0)
    (clay-context-set-feather FALSE)

    (clay-selection-all w-img)
    (clay-edit-copy (vector w-layer))
    (clay-image-delete w-img)
    (let* (
           (pasted (car (clay-edit-paste drawable FALSE)))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
          (clay-layer-set-offsets floating-sel
                                  (car d-offsets)
                                  (cadr d-offsets))
          (clay-layer-set-mode floating-sel LAYER-MODE-MULTIPLY)
          (clay-floating-sel-to-layer floating-sel)
    )
  )
  (clay-context-pop)
  (clay-image-undo-group-end img)
  (clay-displays-flush)
)

(script-fu-register-filter "script-fu-weave"
  _"_Weave..."
  _"Create a new layer filled with a weave effect to be used as an overlay or bump map"
  "Federico Mena Quintero"
  "Federico Mena Quintero"
  "June 1997"
  "RGB* GRAY*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Ribbon width"     '(30  0 256 1 10 1 1)
  SF-ADJUSTMENT _"Ribbon spacing"   '(10  0 256 1 10 1 1)
  SF-ADJUSTMENT _"Shadow darkness"  '(75  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Shadow depth"     '(75  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Thread length"    '(200 0 256 1 10 1 1)
  SF-ADJUSTMENT _"Thread density"   '(50  0 100 1 10 1 1)
  SF-ADJUSTMENT _"Thread intensity" '(100 0 100 1 10 1 1)
)

(script-fu-menu-register "script-fu-weave"
                         "<Image>/Filters/Artistic")
