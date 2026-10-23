; Test cut methods of Edit module of the PDB

; See elsewhere for other cases e.g. copy, paste


(script-fu-use-v3)


; setup

; Load test image that already has drawable
(define testImage (testing:load-test-image-basic-v3))

; get all the root layers
; testImage has exactly one root layer.
(define testLayers (clay-image-get-layers testImage))
;testLayers is-a vector
(define testLayer (vector-ref testLayers 0))

; returns a new floating sel layer
;(define makeFloatingSel )
; paste when clipboard is not empty returns a vector of length one
;(assert `(= (vector-length (clay-edit-paste ,testLayer TRUE)) ; paste-into
;           1))

; function to create a multi-layer image
; Defines globally: testImage, testLayer, testLayer2
(define (make-multi-layer-image)
  (define testImage (testing:load-test-image-basic-v3))
  ; get root layers
  (define testLayers (clay-image-get-layers testImage))
  ;testLayers is-a vector
  (define testLayer (vector-ref testLayers 0))
  (assert `(= (vector-length (clay-image-get-layers ,testImage))
              1))
  (define testLayer2 (testing:layer-new-inserted testImage))
  (assert `(= (vector-length (clay-image-get-layers ,testImage))
              2)))




(test! "edit-cut when selection")

; setup, create a selection
(assert `(clay-selection-all ,testImage))
(assert `(not (clay-selection-is-empty ,testImage)))

; edit-cut requires a vector of drawables to cut from.
; Pass it only the first layer.
(assert `(clay-edit-cut (vector ,testLayer)))
; There are still one layers in the image
(assert `(= (vector-length (clay-image-get-layers ,testImage)) 1))
; !!! No API method is-clipboard-empty


(test! "edit-named-cut when selection does not remove the layer")

; setup: count current named buffers
; earlier tests left a buffer?
(define bufferCount (length (clay-buffers-get-name-list ""))) ; empty regex string

(assert `(clay-edit-named-cut (vector ,testLayer) "testBufferName2"))
; There are still one layers
(assert `(= (vector-length (clay-image-get-layers ,testImage)) 1))

(test! "edit-named-cut when selection creates buffer")
; There is another named buffer
(assert `(= (length (clay-buffers-get-name-list ""))
            (+ ,bufferCount 1)))


(test! "cut from single layer image when no selection removes the layer")

; setup, delete selection
(assert `(clay-selection-none ,testImage))
(assert `(clay-selection-is-empty ,testImage))

; cut when no selection cuts given layers out of image
; Cut the only layer out.
; returns #t when succeeds
(assert `(clay-edit-cut (vector ,testLayer)))
; effective: count layers now 0
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            0))



(test! "cut a floating selection")

; Restore a layer to the image
(define testLayer2 (testing:layer-new-inserted testImage))
; assert layer is inserted in image

(define testFloatingSelInVector
  ; edit-paste returns a vector containing a floating-sel
  (clay-edit-paste
                testLayer2
                TRUE)); paste-into

; edit-cut takes that vector
(assert `(clay-edit-cut ,testFloatingSelInVector))

; edit-cut effective: ID no longer valid
(assert `(not (clay-item-id-is-valid
                (vector-ref ,testFloatingSelInVector 0))))


(test! "sequential cuts from a multi layer image")

; setup restore the testImage
(define testImage (testing:load-test-image-basic-v3))
; get root layers
(define testLayers (clay-image-get-layers testImage))
;testLayers is-a vector
(define testLayer (vector-ref testLayers 0))
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            1))
(define testLayer2 (testing:layer-new-inserted testImage))
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            2))

(clay-selection-none testImage)
(assert `(clay-selection-is-empty ,testImage))

; cut the original layer, one of two layers.

; cut when no selection cuts given layers out of image
; returns #t when succeeds
(assert `(clay-edit-cut (vector ,testLayer)))
; effective: count layers now 1
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            1))

; cut the second layer
(assert `(clay-edit-cut (vector ,testLayer2)))
; effective: count layers now 0
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            0))


(test! "multiple cuts at once from a multi layer image")

(make-multi-layer-image)
; edit-cut takes a vector of many layers
; FIXME: this should not throw an error?  both layers ARE inserted?
(assert-error `(clay-edit-cut (vector ,testLayer ,testLayer2))
               "Procedure execution of clay-edit-cut failed on invalid input arguments:")
; Item 'Background' (12) cannot be used because it has not been added to an image


;(clay-display-new testImage)


(script-fu-use-v2)