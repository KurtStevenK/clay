; test op methods of drawable

; operations change pixels of the drawable without reference to other objects,
; or with passed non-drawable args such as curves

; So that #t binds to boolean arg to PDB
(script-fu-use-v3)

; setup

(define testImage (testing:load-test-image-basic-v3))
; Wilber has one layer

; car is vector, first element is a drawable
(define testDrawable (vector-ref (clay-image-get-layers testImage) 0))


(test! "drawable operations")

; tests in alphabetic order

(assert `(clay-drawable-brightness-contrast ,testDrawable 0.1 -0.1))

(assert `(clay-drawable-color-balance ,testDrawable TRANSFER-MIDTONES 1 0.1 0.1 0.1))

(assert `(clay-drawable-colorize-hsl ,testDrawable 360 50 -50))

; requires vector of size 256 of floats
(assert `(clay-drawable-curves-explicit ,testDrawable HISTOGRAM-RED
      (make-vector 256 1.0)))

; two pairs of float control points of a spline, four floats in total
(assert `(clay-drawable-curves-spline ,testDrawable HISTOGRAM-RED #(0 0 25.0 25.0) ))

(assert `(clay-drawable-desaturate ,testDrawable DESATURATE-LUMA))

(assert `(clay-drawable-equalize ,testDrawable 1)) ; boolean mask-only

(assert `(clay-drawable-extract-component ,testDrawable SELECT-CRITERION-HSV-SATURATION #t #t))

(assert `(clay-drawable-fill ,testDrawable FILL-CIELAB-MIDDLE-GRAY))

(assert `(clay-drawable-foreground-extract ,testDrawable FOREGROUND-EXTRACT-MATTING ,testDrawable))

(assert `(clay-drawable-hue-saturation ,testDrawable HUE-RANGE-MAGENTA 0 1 2 3))

(assert `(clay-drawable-invert ,testDrawable 1)) ; boolean invert in linear space

(assert `(clay-drawable-levels
            ,testDrawable
            HISTOGRAM-LUMINANCE
            0.5 0.5 1 ; boolean clamp input
            8 0.5 0.5 1 ; boolean clamp output
            ))

(assert `(clay-drawable-levels-stretch ,testDrawable))

(assert `(clay-drawable-posterize ,testDrawable 2))

(assert `(clay-drawable-shadows-highlights
             ,testDrawable
             -50 50
             -10
             1300
             50
             0 100))

(assert `(clay-drawable-threshold
            ,testDrawable
            HISTOGRAM-ALPHA
            0.1 1))

(assert `(clay-drawable-desaturate ,testDrawable DESATURATE-LUMA))
(assert `(clay-drawable-desaturate ,testDrawable DESATURATE-LUMA))

(testing:show testImage)

(script-fu-use-v2)
