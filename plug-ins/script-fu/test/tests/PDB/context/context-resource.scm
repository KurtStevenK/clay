; test resource methods of Context

; !!! This requires a fresh install of Clay,
; so that the context is the default,
; and not the user's last choice from prior session.

(script-fu-use-v3)



; function to test methods on Resource for a valid Resource ID.
; Tests methods: get-name, id-is-valid, get-identifiers, is-editable.
(define (test-resource-methods resource)

  ; a resource is an int ID in ScriptFu
  (assert `(integer? ,resource))

  ; get-name returns a string
  (assert `(string? (clay-resource-get-name ,resource)))

  ; id-is-valid returns truth
  (assert `(clay-resource-id-is-valid ,resource))


  ; since 3.0rc3 resource-get-identifiers is private to libclay
  ; clay-resource-get-identifiers succeeds
  ; it returns a triplet
  ;(assert `(clay-resource-get-identifiers ,resource))

  ; clay-resource-get-identifiers returns numeric for is-internal
  ; Some of the fresh clay active resource are internal, some not !!!
  ;(assert `(number? (clay-resource-get-identifiers ,resource)))

  ; name from get-identifiers is same as from clay-resource-get-name
  ; name is second field of triplet i.e. cadr
  ;(assert `(string=? (cadr (clay-resource-get-identifiers ,resource))
  ;                  (clay-resource-get-name ,resource)))


  ; clay-resource-is-editable succeeds
  ; Returns a wrapped boolean
  ; !!! Checking the result.
  ; Since the test should be run with context the fresh default,
  ; all should be resources owned by Clay and not editable.
  (assert `(not (clay-resource-is-editable ,resource)))

  ; The fresh clay active resources are all system resources i.e. not editable
  ; returns 0 for #f
  (assert `(not (clay-resource-is-editable ,resource)))
)

;                  "Test Parasite") ; name
;               "Procedure execution of clay-resource-get-parasite failed")



; Since 3.0rc2 context-get-resource is private
; It returns active resource of given className
; (define testGradient (clay-context-get-resource "ClayGradient"))
; context-get-resource requires a valid type name
; (assert-error `(clay-context-get-resource "InvalidTypeName")
;              "Procedure execution of clay-context-get-resource failed")

; This is setup, not asserted.

(test! "clay-context-get-<resource>")

(define testBrush    (clay-context-get-brush))
(define testFont     (clay-context-get-font))
(define testGradient (clay-context-get-gradient))
(define testPalette  (clay-context-get-palette))
(define testPattern  (clay-context-get-pattern))
; FUTURE Dynamics and other Resource subclasses


(test! "methods on active resource ID's")

(test-resource-methods testBrush)
(test-resource-methods testFont)
(test-resource-methods testGradient)
(test-resource-methods testPalette)
(test-resource-methods testPattern)



(test! "resource-id-is-foo methods")

; the resource IDs from setup work with the specific id-is-foo methods
(assert `(clay-resource-id-is-brush ,testBrush))
(assert `(clay-resource-id-is-font ,testFont))
(assert `(clay-resource-id-is-gradient ,testGradient))
(assert `(clay-resource-id-is-palette ,testPalette))
(assert `(clay-resource-id-is-pattern ,testPattern))




(script-fu-use-v2)
