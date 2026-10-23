; test get/set attributes of drawable

; The test script uses v3 binding of return values
(script-fu-use-v3)


; setup

(define testImage (testing:load-test-image-basic-v3))
; Loaded "clay-logo.png" i.e. Wilber having one layer
(define testDrawable (vector-ref (clay-image-get-layers testImage) 0))




; a drawable is represented by an ID
; As an item, it is type Drawable
(assert `(clay-item-id-is-drawable ,testDrawable))




(test! "getters of Drawable")

; only testing getters that are not of the superclass Item

; bytes per pixel
(assert `(number? (clay-drawable-get-bpp ,testDrawable)))
; height and width are single numbers
(assert `(number? (clay-drawable-get-height ,testDrawable)))
(assert `(number? (clay-drawable-get-width ,testDrawable)))
; offset is list of two numbers
(assert `(list? (clay-drawable-get-offsets ,testDrawable)))

; since 3.0rc2 drawable-get-format is private to libclay
; formats are strings encoded for babl
; (assert `(string? (clay-drawable-get-format ,testDrawable)))

; Since 3.0rc2, this is private to libclay
;(assert `(string? (clay-drawable-get-thumbnail-format ,testDrawable)))


; the test drawable has transparency
; FUTURE: inconsistent naming, should be clay-drawable-get-alpha?
(assert `(clay-drawable-has-alpha ,testDrawable))

; the test drawable has image base type RGB
(assert `(clay-drawable-is-rgb ,testDrawable))
(assert `(not (clay-drawable-is-gray ,testDrawable)))
(assert `(not (clay-drawable-is-indexed ,testDrawable)))

; the test drawable has type RGBA
(assert `(= (clay-drawable-type ,testDrawable)
            RGBA-IMAGE))


; These are deprecated.
; Scripts should use superclass clay-item-get.
; Which are tested elsewhere
;(assert `(clay-drawable-get-image ,testDrawable))
;(assert `(clay-drawable-get-name ,testDrawable))
;(assert `(clay-drawable-get-tattoo ,testDrawable))
; the test drawable is visible
;(assert-PDB-true `(clay-drawable-get-visible ,testDrawable))




; TODO setters


(script-fu-use-v2)
