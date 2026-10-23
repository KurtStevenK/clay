; test Image methods of PDB

; loading this file changes testing state

; Using numeric equality operator '=' on numeric ID's

; Returned values not wrapped in lists.
(script-fu-use-v3)



;            setup

; method new from fresh Clay state returns ID 1
(define testImage (clay-image-new 21 22 RGB))





; method is_valid on new image yields 1 i.e. true
(assert `(clay-image-id-is-valid ,testImage))


(test! "attributes of new image")

; method is_dirty on new image is true
(assert `(clay-image-is-dirty ,testImage))

; method get_width on new image yields same width given when created
(assert `(=
            (clay-image-get-width ,testImage)
            21))

; method get_height on new image yields same height given when created
(assert `(=
            (clay-image-get-height ,testImage)
            22))

; method get-base-type yields same image type given when created
(assert `(=
            (clay-image-get-base-type ,testImage)
            RGB))

; new image is known to clay.
; Returns (<length> #(1))
; Test that the length is 1.
; !!! This is sensitive to retest, if a test leaves images.
(assert `(= (vector-length (clay-get-images))
             1))


(test! "new image has few components")

; !!!!
; New image has one drawable, the selection mask.
; Note there is no clay-image-get-drawables
; FIXME: this is susceptible to test order:
; subsequent images will have different ID for selection mask.
(assert `(clay-item-id-is-valid 1))
; Item 1 is the Selection Mask.
(assert `(string=? (clay-item-get-name 1)
                   "Selection Mask"))



; new image has zero layers
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            0))

; new image has zero paths
(assert `(= (vector-length (clay-image-get-paths ,testImage))
            0))

; new image has no parasites
; returns a GStrv i.e. just a list
(assert `(= (length
              (clay-image-get-parasite-list ,testImage))
            0))




(test! "new image has selection")

(assert `(clay-image-get-selection ,testImage))

; new image has no floating selection
(assert `(=
          (clay-image-get-floating-sel ,testImage)
          -1))



; new image has unit having ID 1
(assert `(=
            (clay-image-get-unit ,testImage)
            1))

; new image has name
(assert `(string=?
            (clay-image-get-name ,testImage)
            "[Untitled]"))

; since 3.0rc image-get-metadata private to libclay
; new image has empty metadata string
;(assert `(string=?
;            (clay-image-get-metadata ,testImage)
;            ""))


; since 3.0rc image-get-metadata private to libclay
;(test! "new image has an effective color profile")
;(assert `(clay-image-get-effective-color-profile ,testImage))



(test! "new image has no associated files")

; GFile is string in ScriptFu

; no file, xcf file, imported file, or exported file
(assert `(string=? (clay-image-get-file     ,testImage)
                    ""))
(assert `(string=? (clay-image-get-xcf-file ,testImage)
                    ""))
(assert `(string=? (clay-image-get-imported-file ,testImage)
                   ""))
(assert `(string=? (clay-image-get-exported-file ,testImage)
                   ""))



(test! "image delete method")

; !!! ID 1 is no longer valid

; method delete succeeds on new image
; returns 1 for true in v2.  returns #t in v3
(assert `(clay-image-delete ,testImage))

; ensure id invalid for deleted image
; returns 0 for false in v2.  returns #f in v3
(assert `(not (clay-image-id-is-valid ,testImage)))


; deleted image is not in clay
; Returns (<length> #())
; FUTURE Returns empty list `()
(assert `(=
            (vector-length (clay-get-images))
            0))
; !!! This only passes when testing is from fresh Clay restart


(test! "abnormal args to image-new")


; Dimension zero yields error
; It does NOT yield invalid ID -1
(assert-error `(clay-image-new 0 0 RGB)
               "argument 1 in call to clay-image-new has value 0 out of range: 1 to 524288")
; Not this: "Invalid value for argument 0")

; Since 3.0, parameter validation catches this earlier.
; Formerly,  "Procedure execution of clay-image-new failed on invalid input arguments: "
; "Procedure 'clay-image-new' has been called with value '0' for argument 'width' (#1, type gint)."))
; " This value is out of range."

(script-fu-use-v2)


