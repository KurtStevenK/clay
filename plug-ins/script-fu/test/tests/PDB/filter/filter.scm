; Test methods of DrawableFilter

; This only tests methods of a filter
; that exists but is not merged or appended to a layer.
; It doesn't test the algebra of methods that use the filter.


(script-fu-use-v3)

; setup

(define testImage (testing:load-test-image-basic-v3))
(define testLayer (vector-ref (clay-image-get-layers testImage) 0))
(define testFilter (clay-drawable-filter-new
                      testLayer
                      "gegl:ripple"
                      ""    ; user given "effect name"
                    ))


(test! "ID valid")
(assert `(clay-drawable-filter-id-is-valid ,testFilter))



(test! "getters of attributes of new filter")

; operation name is as given
(assert `(string=? (clay-drawable-filter-get-operation-name ,testFilter)
                   "gegl:ripple"))

; name is not the user given name yet, is the gegl op name, capitalized
(assert `(string=? (clay-drawable-filter-get-name ,testFilter)
                   "Ripple"))

; visible by default
(assert `(clay-drawable-filter-get-visible ,testFilter))

; opaque by default
(assert `(= (clay-drawable-filter-get-opacity ,testFilter)
            1.0))

; blend mode default
(assert `(=  (clay-drawable-filter-get-blend-mode ,testFilter)
             LAYER-MODE-REPLACE))

; (test! "get arguments")
; This is a method on an instance of Filter: takes an ID
; It returns the current settings of the filter.
; FIXME wire ClayParamValueArray unsupported
; (clay-drawable-filter-get-arguments testFilter)




(test! "Class methods on Filter")

; Note the arg is the string name, not a filter ID.
; That is, a method on named subclass of Filter, not on an instance.
; The name is qualified by "gegl:"

; since 3.0rc2 filter-get-number-arguments private to libclay
;(test! "get number arguments")
; This is a class method on the "subclass" of Filter, not on an instance.
;(assert `(=  (clay-drawable-filter-get-number-arguments "gegl:ripple")
;             8))

; since 3.0rc2 filter-get-pspec private to libclay
; get the pspec for the first argument
; FIXME: fails SF unhandled return type GParam
; (clay-drawable-filter-get-pspec "gegl:ripple" 1)




(test! "setters of filter")

; The only setter is for visible?
; All other
(clay-drawable-filter-set-visible testFilter #f)
; effective
(assert `(not (clay-drawable-filter-get-visible ,testFilter)))




(test! "filter delete")
(clay-drawable-filter-delete testFilter)

; effective, ID is no longer valid
(assert `(not (clay-drawable-filter-id-is-valid ,testFilter)))



; optional display result images
;(clay-display-new testImage)
;(clay-displays-flush)

(script-fu-use-v2)