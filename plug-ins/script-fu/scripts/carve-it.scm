;  CARVE-IT
;   Carving, embossing, & stamping
;   Process taken from "The Photoshop 3 WOW! Book"
;   http://www.peachpit.com
;   This script requires a grayscale image containing a single layer.
;   This layer is used as the mask for the carving effect
;   NOTE: This script requires the image to be carved to either be an
;   RGB color or grayscale image with a single layer. An indexed file
;   can not be used due to the use of clay-drawable-histogram and
;   clay-drawable-levels.


(define (carve-scale val scale)
  (* (sqrt val) scale))

(define (calculate-inset-gamma img layer)
  (let* ((stats (clay-drawable-histogram layer 0 0.0 1.0))
         (mean (car stats)))
    (cond ((< mean 127) (+ 1.0 (* 0.5 (/ (- 127 mean) 127.0))))
          ((>= mean 127) (- 1.0 (* 0.5 (/ (- mean 127) 127.0)))))))


(define (copy-layer-carve-it dest-image dest-drawable source-image source-drawable)
  (clay-selection-all dest-image)
  (clay-drawable-edit-clear dest-drawable)
  (clay-selection-none dest-image)
  (clay-selection-all source-image)
  (clay-edit-copy (vector source-drawable))
  (let* (
         (pasted (car (clay-edit-paste dest-drawable FALSE)))
         (floating-sel (vector-ref pasted (- (vector-length pasted) 1)))
        )
        (clay-floating-sel-anchor floating-sel)
  )
)



(define (script-fu-carve-it bg-img bg-layers mask-img mask-drawable carve-white)
  (let* (
        (src-layer (vector-ref bg-layers 0))
        (width (car (clay-drawable-get-width mask-drawable)))
        (height (car (clay-drawable-get-height mask-drawable)))
        (type (car (clay-drawable-type src-layer)))
        (img (car (clay-image-new width height (cond ((= type RGB-IMAGE) RGB)
                                                     ((= type RGBA-IMAGE) RGB)
                                                     ((= type GRAY-IMAGE) GRAY)
                                                     ((= type GRAYA-IMAGE) GRAY)
                                                     ((= type INDEXED-IMAGE) INDEXED)
                                                     ((= type INDEXEDA-IMAGE) INDEXED)))))
        (size (min width height))
        (offx (carve-scale size 0.33))
        (offy (carve-scale size 0.25))
        (feather (carve-scale size 0.3))
        (brush-size (carve-scale size 0.3))
        (brush (car (clay-brush-new "Carve It")))
        (mask (car (clay-channel-new img "Engraving Mask" width height 50 '(0 0 0))))
        (inset-gamma (calculate-inset-gamma (car (clay-item-get-image src-layer)) src-layer))
        (mask-fat 0)
        (mask-emboss 0)
        (mask-highlight 0)
        (mask-shadow 0)
        (shadow-layer 0)
        (highlight-layer 0)
        (cast-shadow-layer 0)
        (csl-mask 0)
        (inset-layer 0)
        (il-mask 0)
        (bg-width (car (clay-drawable-get-width src-layer)))
        (bg-height (car (clay-drawable-get-height src-layer)))
        (bg-type (car (clay-drawable-type src-layer)))
        (bg-image (car (clay-item-get-image src-layer)))
        (layer1 (car (clay-layer-new img "Layer1" bg-width bg-height bg-type 100 LAYER-MODE-NORMAL)))
        )

    (clay-context-push)
    (clay-context-set-defaults)

    (clay-image-undo-disable img)

    (clay-image-insert-layer img layer1 0 0)

    (clay-selection-all img)
    (clay-drawable-edit-clear layer1)
    (clay-selection-none img)
    (copy-layer-carve-it img layer1 bg-image src-layer)

    (clay-edit-copy (vector mask-drawable))
    (clay-image-insert-channel img mask -1 0)

    (plug-in-tile #:run-mode   RUN-NONINTERACTIVE
                  #:image      img
                  #:drawables  (vector layer1)
                  #:new-width  width
                  #:new-height height
                  #:new-image  FALSE)
    (let* (
           (pasted (car (clay-edit-paste mask FALSE)))
           (floating-sel (vector-ref pasted(- (vector-length pasted) 1)))
          )
          (clay-floating-sel-anchor floating-sel)
    )
    (if (= carve-white FALSE)
        (clay-drawable-merge-new-filter mask "gegl:invert-gamma" 0 LAYER-MODE-REPLACE 1.0))

    (set! mask-fat (car (clay-channel-copy mask)))
    (clay-image-insert-channel img mask-fat -1 0)
    (clay-image-select-item img CHANNEL-OP-REPLACE mask-fat)

    (clay-brush-set-shape brush BRUSH-GENERATED-CIRCLE)
    (clay-brush-set-spikes brush 2)
    (clay-brush-set-hardness brush 1.0)
    (clay-brush-set-spacing brush 25)
    (clay-brush-set-aspect-ratio brush 1)
    (clay-brush-set-angle brush 0)
    (cond (<= brush-size 17) (clay-brush-set-radius brush (\ brush-size 2))
	  (else clay-brush-set-radius brush (\ 19 2)))
    (clay-context-set-brush brush)

    (clay-context-set-foreground '(255 255 255))
    (clay-drawable-edit-stroke-selection mask-fat)
    (clay-selection-none img)

    (set! mask-emboss (car (clay-channel-copy mask-fat)))
    (clay-image-insert-channel img mask-emboss -1 0)
    (clay-drawable-merge-new-filter mask-emboss "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 feather) "std-dev-y" (* 0.32 feather) "filter" "auto")
    (clay-drawable-merge-new-filter mask-emboss "gegl:emboss" 0 LAYER-MODE-REPLACE 1.0 "azimuth" 315.0 "elevation" 45.0 "depth" 7 "type" "emboss")

    (clay-context-set-background '(180 180 180))
    (clay-image-select-item img CHANNEL-OP-REPLACE mask-fat)
    (clay-selection-invert img)
    (clay-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (clay-image-select-item img CHANNEL-OP-REPLACE mask)
    (clay-drawable-edit-fill mask-emboss FILL-BACKGROUND)
    (clay-selection-none img)

    (set! mask-highlight (car (clay-channel-copy mask-emboss)))
    (clay-image-insert-channel img mask-highlight -1 0)
    (clay-drawable-levels mask-highlight 0
			  0.7056 1.0 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (set! mask-shadow mask-emboss)
    (clay-drawable-levels mask-shadow 0
			  0.0 0.70586 TRUE
			  1.0
			  0.0 1.0 TRUE)

    (clay-edit-copy (vector mask-shadow))
    (let* (
           (pasted (car (clay-edit-paste layer1 FALSE)))
           (floating-sel (vector-ref pasted (- (vector-length pasted) 1)))
          )
          (set! shadow-layer floating-sel)
          (clay-floating-sel-to-layer shadow-layer)
    )
    (clay-layer-set-mode shadow-layer LAYER-MODE-MULTIPLY)

    (clay-edit-copy (vector mask-highlight))
    (let* (
           (pasted (car (clay-edit-paste shadow-layer FALSE)))
           (floating-sel (vector-ref pasted (- (vector-length pasted) 1)))
          )
          (set! highlight-layer floating-sel)
          (clay-floating-sel-to-layer highlight-layer)
    )
    (clay-layer-set-mode highlight-layer LAYER-MODE-SCREEN)

    (clay-edit-copy (vector mask))
    (let* (
           (pasted (car (clay-edit-paste highlight-layer FALSE)))
           (floating-sel (vector-ref pasted (- (vector-length pasted) 1)))
          )
          (set! cast-shadow-layer floating-sel)
          (clay-floating-sel-to-layer cast-shadow-layer)
    )
    (clay-layer-set-mode cast-shadow-layer LAYER-MODE-MULTIPLY)
    (clay-layer-set-opacity cast-shadow-layer 75)

    (clay-drawable-merge-new-filter cast-shadow-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 feather) "std-dev-y" (* 0.32 feather) "filter" "auto")
    (clay-item-transform-translate cast-shadow-layer offx offy)

    (set! csl-mask (car (clay-layer-create-mask cast-shadow-layer ADD-MASK-BLACK)))
    (clay-layer-add-mask cast-shadow-layer csl-mask)
    (clay-image-select-item img CHANNEL-OP-REPLACE mask)
    (clay-context-set-background '(255 255 255))
    (clay-drawable-edit-fill csl-mask FILL-BACKGROUND)

    (set! inset-layer (car (clay-layer-copy layer1)))
    (clay-layer-add-alpha inset-layer)
    (clay-image-insert-layer img inset-layer 0 1)

    (set! il-mask (car (clay-layer-create-mask inset-layer ADD-MASK-BLACK)))
    (clay-layer-add-mask inset-layer il-mask)
    (clay-image-select-item img CHANNEL-OP-REPLACE mask)
    (clay-context-set-background '(255 255 255))
    (clay-drawable-edit-fill il-mask FILL-BACKGROUND)
    (clay-selection-none img)
    (clay-selection-none bg-image)
    (clay-drawable-levels inset-layer 0 0.0 1.0 TRUE inset-gamma 0.0 1.0 TRUE)
    (clay-image-remove-channel img mask)
    (clay-image-remove-channel img mask-fat)
    (clay-image-remove-channel img mask-highlight)
    (clay-image-remove-channel img mask-shadow)

    (clay-item-set-name layer1 _"Carved Surface")
    (clay-item-set-name shadow-layer _"Bevel Shadow")
    (clay-item-set-name highlight-layer _"Bevel Highlight")
    (clay-item-set-name cast-shadow-layer _"Cast Shadow")
    (clay-item-set-name inset-layer _"Inset")

    (clay-resource-delete brush)

    (clay-display-new img)
    (clay-image-undo-enable img)

    (clay-context-pop)
  )
)

(script-fu-register-filter "script-fu-carve-it"
    _"Stencil C_arve..."
    _"Use the specified drawable as a stencil to carve from the specified image."
    "Spencer Kimball"
    "Spencer Kimball"
    "1997"
    "GRAY"
    SF-ONE-OR-MORE-DRAWABLE
    SF-IMAGE    _"Mask image"        0
    SF-DRAWABLE _"Mask drawable"     0
    SF-TOGGLE   _"Carve white areas" TRUE
)

(script-fu-menu-register "script-fu-carve-it"
                         "<Image>/Filters/Decor")
