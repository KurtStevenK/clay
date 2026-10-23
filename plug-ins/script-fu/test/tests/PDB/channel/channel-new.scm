; Test methods of Channel class of the PDB


(script-fu-use-v3)


; setup
; new, empty image
(define testImage (clay-image-new 21 22 RGB))


; new image has no custom channels
(assert `(= (vector-length (clay-image-get-channels ,testImage))
            0))

; setup (not in an assert and not quoted)
; vectors-new succeeds
(define testChannel (clay-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" ))      ; compositing color





(test! "new channel is not in image until inserted")
; get-channels yields (0 #())
(assert `(= (vector-length (clay-image-get-channels ,testImage))
            0))

; channel ID is valid
(assert `(clay-item-id-is-channel ,testChannel))


(test! "new channel attributes")

; color attribute is as given during creation
(assert `(equal?
            (clay-channel-get-color ,testChannel)
            '(255 0 0 255)))  ; red
; clay-channel-get-name is deprecated
(assert `(string=?
            (clay-item-get-name ,testChannel)
            "Test Channel"))




(test! "insert-channel")

; insert succeeds
(assert `(clay-image-insert-channel
            ,testImage
            ,testChannel
            0            ; parent, moot since channel groups not supported
            0))          ; position in stack

; insert was effective: testImage now has one channel
(assert `(= (vector-length (clay-image-get-channels ,testImage))
            1))

; insert was effective: image now knows by name
; capture the ID of channel we just newed
(assert `(=
           (clay-image-get-channel-by-name
                ,testImage
                "Test Channel")
           ,testChannel))


(test! "remove-channel")

; Note the difference between remove and delete:
; Docs say that delete is only useful for a channel not added to the image.

; remove does not throw
(assert `(clay-image-remove-channel ,testImage ,testChannel))

; Effective: image now has zero channels
(assert `(= (vector-length (clay-image-get-channels ,testImage))
            0))

; After remove, channel ID is NOT valid
(assert `(not (clay-item-id-is-channel ,testChannel)))

; Delete throws error when channel already removed
; clay-channel-delete is deprecated
(assert-error `(clay-item-delete ,testChannel)
               "Invalid value for argument 0")
; FORMERLY     "runtime: invalid item ID"  )


(test! "item-delete on channel")

; Can delete a new channel not yet added to image

(define testChannel2 (clay-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" ))      ; compositing color

; Does not throw
(assert `(clay-item-delete ,testChannel2))

; Effective: ID is not valid
(assert `(not (clay-item-id-is-channel ,testChannel)))


(script-fu-use-v2)
