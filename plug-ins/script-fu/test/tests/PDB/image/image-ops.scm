; Test various operations on image

; The operand is the total image itself.
; For paint operations (changing a subset of the image) see paint.scm


(script-fu-use-v3)


;              setup
(define testImage (clay-image-new 21 22 RGB))


(test! "image transformations")

; flip
(assert `(clay-image-flip ,testImage ORIENTATION-HORIZONTAL))
(assert `(clay-image-flip ,testImage ORIENTATION-VERTICAL))

(assert-error `(clay-image-flip ,testImage ORIENTATION-UNKNOWN)
    (string-append
      "Procedure execution of clay-image-flip failed on invalid input arguments: "
      "Procedure 'clay-image-flip' has been called with value 'CLAY_ORIENTATION_UNKNOWN'"
      " for argument 'flip-type' (#2, type ClayOrientationType). This value is out of range."))

; rotate
; v2 ROTATE-90 => v3 ROTATE-DEGREES90
(assert `(clay-image-rotate ,testImage ROTATE-DEGREES90))
(assert `(clay-image-rotate ,testImage ROTATE-DEGREES180))
(assert `(clay-image-rotate ,testImage ROTATE-DEGREES270))

; scale
; up
(assert `(clay-image-scale ,testImage 100 100))

; down to min
(assert `(clay-image-scale ,testImage 1 1))

; up to max
; Performance:
; This seems to work fast when previous scaled to 1,1
; but then seems to slow down testing
; unless we scale down afterwards.
; This seems glacial if not scaled to 1,1 prior.
; FIXME throws GLib-GObject-CRITICAL value "524288.000000" of type 'gdouble'
; is invalid or out of range for property 'x' of type 'gdouble'
; but docs say 524288 is the max
; (assert `(clay-image-scale ,testImage 524288 524288))

; down to min does not throw
(assert `(clay-image-scale ,testImage 1 1))
; effective
(assert `(= (clay-image-get-height ,testImage)
            1))
; Note there is no get-size, only get-height and width, the origin is always (0,0)


; resize does not throw
(assert `(clay-image-resize ,testImage
            30 30 ; width height
            0 0)) ; offset
; effective
(assert `(= (clay-image-get-height ,testImage)
            30))

; resize to layers when image is empty of layers does not throw
(assert `(clay-image-resize-to-layers ,testImage))
; not effective: height remains the same
; effective
(assert `(= (clay-image-get-height ,testImage)
            30))

; TODO resize to layers when there is a layer smaller than canvas



; TODO crops that are plugins plug-in-zealouscrop et al

(test! "crop")

(assert `(clay-image-crop ,testImage
    2 2 ; width height
    2 2 ; x y offset
    ))



(test! "image transformation by policy ops")
; These perform operations (convert or rotate) using a policy in preferences

; 0 means non-interactive, else shows dialog in some cases
(assert `(clay-image-policy-color-profile ,testImage 0))

(assert `(clay-image-policy-rotate ,testImage 0))



(test! "freezing and unfreezing (avoid updates to dialogs)")

; Used for performance.
(assert `(clay-image-freeze-channels ,testImage))
(assert `(clay-image-freeze-layers ,testImage))
(assert `(clay-image-freeze-paths ,testImage))
(assert `(clay-image-thaw-channels ,testImage))
(assert `(clay-image-thaw-layers ,testImage))
(assert `(clay-image-thaw-paths ,testImage))

(test! "clean-all makes image not dirty")
(assert `(clay-image-clean-all ,testImage))
(assert `(not (clay-image-is-dirty ,testImage)))


; flatten is tested in layer-ops.scm

; cannot flatten empty image
(assert-error `(clay-image-flatten ,testImage)
  "Procedure execution of clay-image-flatten failed: Cannot flatten an image without any visible layer.")

(script-fu-use-v2)