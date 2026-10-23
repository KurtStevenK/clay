; test Layer methods of PDB


(script-fu-use-v3)


;        setup

(define testImage (clay-image-new 21 22 RGB))

(define testLayer (testing:layer-new testImage))




(test! "new layer is not in the image until inserted")
; returns (length, list), check length is 0
(assert `(= (vector-length (clay-image-get-layers ,testImage))
            0))



(test! "attributes of new layer")

;        defaulted attributes

; apply-mask default false
(assert `(not (clay-layer-get-apply-mask ,testLayer)))

; blend-space default LAYER-COLOR-SPACE-AUTO
(assert `(= (clay-layer-get-blend-space ,testLayer)
            LAYER-COLOR-SPACE-AUTO))

; composite-mode default LAYER-COMPOSITE-AUTO
(assert `(= (clay-layer-get-composite-mode ,testLayer)
            LAYER-COMPOSITE-AUTO))

; composite-space default LAYER-COLOR-SPACE-AUTO
(assert `(= (clay-layer-get-composite-space ,testLayer)
            LAYER-COLOR-SPACE-AUTO))

; edit-mask default false
(assert `(not (clay-layer-get-edit-mask ,testLayer)))

; lock-alpha default false
; deprecated? clay-layer-get-preserve-trans
(assert `(not (clay-layer-get-lock-alpha ,testLayer)))

; mask not exist, ID -1
; clay-layer-mask is deprecated
(assert `(= (clay-layer-get-mask ,testLayer)
            -1))

; mode default LAYER-MODE-NORMAL
(assert `(= (clay-layer-get-mode ,testLayer)
            LAYER-MODE-NORMAL))

; show-mask default false
(assert `(not (clay-layer-get-show-mask ,testLayer)))

; visible default true
; FIXME doc says default false
; clay-layer-get-visible is deprecated.
(assert `(clay-item-get-visible ,testLayer))

; is-floating-sel default false
(assert `(not (clay-layer-is-floating-sel ,testLayer)))

; !!! No get-offsets




(test! "new layer attributes are as given when created")

; name is as given
; clay-layer-get-name is deprecated
(assert `(string=? (clay-item-get-name ,testLayer)
                  "LayerNew"))

; opacity is as given
(assert `(= (clay-layer-get-opacity ,testLayer)
            50.0))


;          generated attributes

; tattoo
; tattoo is generated unique within image?
; clay-layer-get-tattoo is deprecated
(assert `(= (clay-item-get-tattoo ,testLayer)
            2))



(script-fu-use-v2)





