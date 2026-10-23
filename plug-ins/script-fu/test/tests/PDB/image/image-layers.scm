; test Image methods of PDB
; where methods deal with layers owned by image.


; setup
; Load test image that already has drawable
(define testImage (testing:load-test-image "clay-logo.png"))



;                 get-layers
; procedure returns (#(<layerID>)) ....in the REPL

; get-layers returns a list of layers
(assert `(vector? (car (clay-image-get-layers ,testImage))))

; the vector has one element
(assert `(= (vector-length (car (clay-image-get-layers ,testImage)))
            1))

; the vector can be indexed at first element
; and is a numeric ID
(assert `(number?
            (vector-ref (car (clay-image-get-layers ,testImage))
                        0)))

; store the layer ID
(define testLayer (vector-ref (car (clay-image-get-layers testImage))
                              0))

; FIXME seems to fail??? because name is actually "Background"

; the same layer can be got by name
; FIXME app shows layer name is "clay-logo.png" same as image name
(assert `(= (car (clay-image-get-layer-by-name ,testImage "Background"))
            ,testLayer))

; the single layer's position is zero
; clay-image-get-layer-position is deprecated
(assert `(= (car (clay-image-get-item-position ,testImage ,testLayer))
            0))


; TODO clay-image-get-layer-by-tattoo

; the single layer is selected  in freshly opened image
(assert `(vector? (car (clay-image-get-selected-layers ,testImage))))

; TODO test selected layer is same layer
