; Test get/set methods of Channel class of the PDB


(script-fu-use-v3)


; setup (not in an assert and not quoted)

; new, empty image
(define testImage (clay-image-new 21 22 RGB))

(define testChannel
      (clay-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" ))       ; compositing color

(test! "insert-channel")

; new channel is not in image until inserted
(assert `(clay-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack


(test! "set/get channel attributes")

; color
(assert `(clay-channel-set-color ,testChannel "red"))
; effective, getter returns same color: red
(assert `(equal?
            (clay-channel-get-color ,testChannel)
            '(255 0 0 255)))

; opacity
(assert `(clay-channel-set-opacity ,testChannel 0.7))
; effective
; numeric equality
; Compare floats to some fixed epsilon precision
; Otherwise, the test is fragile to changes in the tested code babl, clay etc.
; Actual result is 0.7000000216
(assert `(equal-relative?
            (clay-channel-get-opacity ,testChannel)
            0.7
            0.0001))

; show-masked
(assert `(clay-channel-set-show-masked ,testChannel TRUE))
; effective
; procedure returns boolean, #t
(assert `(clay-channel-get-show-masked ,testChannel))




(test! "item methods applied to channel")

; clay-channel-set-name is deprecated
; clay-channel-set-visible is deprecated
; etc.

; name
(assert `(clay-item-set-name ,testChannel "New Name"))
; effective
(assert `(string=?
            (clay-item-get-name ,testChannel)
            "New Name"))

; visible
(assert `(clay-item-set-visible ,testChannel FALSE))
; effective
; procedure returns boolean #f
(assert `(not (clay-item-get-visible ,testChannel)))

; tattoo
(assert `(clay-item-set-tattoo ,testChannel 999))
; effective
(assert `(=
            (clay-item-get-tattoo ,testChannel)
            999))

; TODO  other item methods

(script-fu-use-v2)
