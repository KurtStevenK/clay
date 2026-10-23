;;; unsharp-mask.scm
;;; Time-stamp: <1998/11/17 13:18:39 narazaki@gimp.org>
;;; Author: Narazaki Shuji <narazaki@gimp.org>
;;; Version 0.8

; This script-fu-unsharp-mask is not in the menus.
; There is an equivalent GEGL filter at Filters>Enhance>Sharpen (Unsharp).
; This might be kept for compatibility and used by third party scripts.

; Seems not used by any script in the repo.
; FUTURE move to clay-data-extras or to scripts/test
; and maintain it with low priority.

; unsharp-mask is a filter AND renderer, creating a new, visible, dirty image
; from the given image.


(define (script-fu-unsharp-mask img drws mask-size mask-opacity)
  (let* (
        (drw (vector-ref drws 0))
        (drawable-width (car (clay-drawable-get-width drw)))
        (drawable-height (car (clay-drawable-get-height drw)))
        (new-image (car (clay-image-new drawable-width drawable-height RGB)))
        (original-layer (car (clay-layer-new new-image "Original"
                                             drawable-width drawable-height
                                             RGB-IMAGE
                                             100 LAYER-MODE-NORMAL)))
        (original-layer-for-darker 0)
        (original-layer-for-lighter 0)
        (blurred-layer-for-darker 0)
        (blurred-layer-for-lighter 0)
        (darker-layer 0)
        (lighter-layer 0)
        )

    (clay-selection-all img)
    (clay-edit-copy (vector drw))

    (clay-image-undo-disable new-image)

    (clay-image-insert-layer new-image original-layer 0 0)

    (let* (
           (pasted (car (clay-edit-paste original-layer FALSE)))
           (num-pasted (vector-length pasted))
           (floating-sel (vector-ref pasted (- num-pasted 1)))
          )
     (clay-floating-sel-anchor floating-sel)
    )

    (set! original-layer-for-darker (car (clay-layer-copy original-layer)))
    (clay-layer-add-alpha original-layer-for-darker)
    (set! original-layer-for-lighter (car (clay-layer-copy original-layer)))
    (clay-layer-add-alpha original-layer-for-lighter)
    (set! blurred-layer-for-darker (car (clay-layer-copy original-layer)))
    (clay-layer-add-alpha blurred-layer-for-darker)
    (clay-item-set-visible original-layer FALSE)
    (clay-display-new new-image)

    ;; make darker mask
    (clay-image-insert-layer new-image blurred-layer-for-darker 0 -1)
    (clay-drawable-merge-new-filter blurred-layer-for-darker "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" (* 0.32 mask-size) "std-dev-y" (* 0.32 mask-size) "filter" "auto")
    (set! blurred-layer-for-lighter
          (car (clay-layer-copy blurred-layer-for-darker)))
    (clay-layer-add-alpha blurred-layer-for-lighter)
    (clay-image-insert-layer new-image original-layer-for-darker 0 -1)
    (clay-layer-set-mode original-layer-for-darker LAYER-MODE-SUBTRACT)
    (set! darker-layer
          (car (clay-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (clay-item-set-name darker-layer "darker mask")
    (clay-item-set-visible darker-layer FALSE)

    ;; make lighter mask
    (clay-image-insert-layer new-image original-layer-for-lighter 0 -1)
    (clay-image-insert-layer new-image blurred-layer-for-lighter 0 -1)
    (clay-layer-set-mode blurred-layer-for-lighter LAYER-MODE-SUBTRACT)
    (set! lighter-layer
          (car (clay-image-merge-visible-layers new-image CLIP-TO-IMAGE)))
    (clay-item-set-name lighter-layer "lighter mask")

    ;; combine them
    (clay-item-set-visible original-layer TRUE)
    (clay-layer-set-mode darker-layer LAYER-MODE-SUBTRACT)
    (clay-layer-set-opacity darker-layer mask-opacity)
    (clay-item-set-visible darker-layer TRUE)
    (clay-layer-set-mode lighter-layer LAYER-MODE-ADDITION)
    (clay-layer-set-opacity lighter-layer mask-opacity)
    (clay-item-set-visible lighter-layer TRUE)

    (clay-image-undo-enable new-image)
    (clay-displays-flush)
  )
)

(script-fu-register-filter "script-fu-unsharp-mask"
  "Unsharp Mask..."
  "Make a new image from the current layer by applying the unsharp mask method"
  "Shuji Narazaki <narazaki@gimp.org>"
  "Shuji Narazaki"
  "1997,1998"
  "*"
  SF-ONE-OR-MORE-DRAWABLE
  SF-ADJUSTMENT _"Mask size"        '(5 1 100 1 1 0 1)
  SF-ADJUSTMENT _"Mask opacity"     '(50 0 100 1 1 0 1)
)
