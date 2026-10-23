; test paint-method methods of API

; tests setting a paint-method in context,
; then painting (stroking) with it.


(script-fu-use-v3)

; setup

; an image, drawable, and path
(define testImage (testing:load-test-image-basic-v3))
(define testLayer (vector-ref (clay-image-get-layers testImage )
                                  0))
(define testPath (clay-path-new testImage "Test Path"))
; must add to image
(clay-image-insert-path
                  testImage
                  testPath
                  0 0) ; parent=0 position=0
; Add stroke to path
(clay-path-stroke-new-from-points
            testPath
            PATH-STROKE-TYPE-BEZIER
            (vector 1 2 3 4 5 6 7 8 9 10 11 12) ; control points
            FALSE) ; not closed



(test! "paint-methods are introspectable to a list of strings")
(assert `(list? (clay-context-list-paint-methods)))

; setup
(define paintMethods (clay-context-list-paint-methods))

; TODO
; test their names all have "clay-" prefix and lower case.

; Test that every returned name is valid to set on the context
; TODO this doesn't seem to work: illegal function
; Probably the assert wrapper screws something up
; (assert `(map clay-context-set-paint-method ,paintMethods))




 (test! "get/set paint-method on context")

(assert `(clay-context-set-paint-method "clay-ink"))

; getter succeeds and setter was effective
(assert `(string=? (clay-context-get-paint-method)
                  "clay-ink"))





; Test that all paint-methods seem to work:
;    set context stroke method to paint-method
;    stroke a drawable along a path with the paint method
;       (except some paintMethods not painted with)

(test! "set context to stroke with paint (versus line)")
(assert `(clay-context-set-stroke-method STROKE-PAINT-METHOD))


(test! "iterate over paintMethods, loosely testing they seem to work")

; test function that paints a path using a paint method.
; paintMethod is string
(define (testPaintMethod paintMethod)
    ; paintMethod can be set on the context
    (clay-context-set-paint-method paintMethod)

    ; Don't paint with paint methods that need a source image set
    ; The API does not have a way to set source image
    (if (not (or
                (string=? paintMethod "clay-clone")
                (string=? paintMethod "clay-heal")
                (string=? paintMethod "clay-perspective-clone")))
      ; paint with the method, under the test harness
      (begin
        (test! paintMethod)
        (assert `(clay-drawable-edit-stroke-item ,testLayer ,testPath)))
      ; else skip
      (test! (string-append "Skipping: " paintMethod))
    ))

; apply testPaintMethod to each paintMethod
(for-each
  testPaintMethod
  paintMethods)

(script-fu-use-v2)
