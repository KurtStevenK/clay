; test Layer methods of PDB
; where methods are operations


(script-fu-use-v3)



; setup

(define testImage (clay-image-new 21 22 RGB))
(define testLayer (testing:layer-new testImage))
; assert layer is not inserted in image


(test! "errors when layer not in image")

; resize fails
(assert-error `(clay-layer-resize ,testLayer 23 24 0 0)
              (string-append
                "Procedure execution of clay-layer-resize failed on invalid input arguments: "))
                ;"Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; scale fails
(assert-error `(clay-layer-scale ,testLayer
                  23 24 ; width height
                  0)    ; is local origin?
              (string-append
                "Procedure execution of clay-layer-scale failed on invalid input arguments: "))
                ;"Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))

; UNTESTED clay-layer-resize-to-image-size fails when layer not in image

; clay-layer-remove-mask fails when layer not in image
(assert-error `(clay-layer-remove-mask
                  ,testLayer
                  MASK-APPLY)
              (string-append
                "Procedure execution of clay-layer-remove-mask failed on invalid input arguments: "))
                ; "Item 'LayerNew#2' (10) cannot be used because it has not been added to an image"))



;              alpha operations

; add-alpha succeeds
(assert `(clay-layer-add-alpha ,testLayer))

; and is effective
; Note method on superclass Drawable
; returns #t
(assert `(clay-drawable-has-alpha ,testLayer))

; flatten succeeds
(assert `(clay-layer-flatten ,testLayer))

; flatten was effective: no longer has alpha
; flatten a layer means "remove alpha"
; returns #f
(assert `(not (clay-drawable-has-alpha ,testLayer)))




(test! "layer-delete")

; clay-layer-delete is deprecated

; succeeds
(assert `(clay-item-delete ,testLayer))

; delete second time fails
(assert-error `(clay-item-delete ,testLayer)
              "Invalid value for argument 0")
; FORMERLY    "runtime: invalid item ID")

; Error for flatten:
; "Procedure execution of clay-layer-delete failed on invalid input arguments: "
; "Procedure 'clay-layer-delete' has been called with an invalid ID for argument 'layer'. "
; "Most likely a plug-in is trying to work on a layer that doesn't exist any longer."))


(script-fu-use-v2)
