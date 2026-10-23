; test Image of mode grayscale methods of PDB

; !!! Note inconsistent use in Clay of GRAY versus GRAYSCALE


(script-fu-use-v3)


;              setup

(define testImage (testing:load-test-image-basic-v3))



(test! "clay-image-convert-grayscale")
(assert `(clay-image-convert-grayscale ,testImage))

; conversion was effective:
; basetype of grayscale is GRAY
(assert `(= (clay-image-get-base-type ,testImage)
            GRAY))

; conversion was effective:
; grayscale image has-a colormap
(assert `(clay-image-get-palette ,testImage))

(test! "grayscale images have precision PRECISION-U8-NON-LINEAR")
; FIXME annotation of PDB procedure says CLAY_PRECISION_U8
(assert `(= (clay-image-get-precision ,testImage)
            PRECISION-U8-NON-LINEAR ))

(test! "drawable of grayscale image is also grayscale")
(assert `(clay-drawable-is-gray
           (clay-image-get-layer-by-name ,testImage "Background")))

; convert precision of grayscale image succeeds
(assert `(clay-image-convert-precision
            ,testImage
            PRECISION-U8-PERCEPTUAL))

(script-fu-use-v2)


