; Test methods of Buffer class of the PDB

; aka NamedBuffer i.e. the clipboard saved with a name.

; Edit methods that create buffers is tested elsewhere.
; The names of those methods is hard to understand:
; because they used "named" to mean "buffer"
; E.G. clay-edit-named-copy might be better named: "edit-copy-to-named-buffer"

; The API has no method to determine if the clipboard is empty
; buffers-get-name-list only gets the named buffers

; Many calls take a regex string

; FORMERLY buffers-get-list => buffers-get-name-list



; Prereq:  no buffer exists yet.

(script-fu-use-v3)

; setup
; Load test image that already has drawable
(define testImage (testing:load-test-image-basic-v3))

; the layer is the zeroeth element in the vector which is the second element
; but cadr returns the second element!!
; TODO make this a library routine: get-first-layer
; (1 #(<layerID>))
(define testDrawable (vector-ref (clay-image-get-layers testImage)
                                  0))

(test! "Create new named buffer")

; There is no clay-buffer-new method,
; instead it is a method of the Edit class so-to-speak
; You can't: #(testDrawable)
(define testBuffer (clay-edit-named-copy
                              (make-vector 1 testDrawable)
                              "bufferName"))
; Since no selection, the buffer is same size as image

; Creation was effective: clay knows the buffer
; get-name-list takes a regex, here empty ""
; get-name-list returns  (("bufferName")) : a list of strings
; and the first string is "bufferName"
(assert `(string=? (car (clay-buffers-get-name-list ""))
                    "bufferName"))

; buffer has same size as image when created with no selection
; test image is 128x128
(assert `(= (clay-buffer-get-width "bufferName")
            128))
(assert `(= (clay-buffer-get-height "bufferName")
            128))

; new buffer has alpha: the image is RGB but the buffer has bpp 4
; This is not well documented.
; FIXME the docs and the method name should say "bpp"
; or "bytes per pixel" instead of "bytes"
(assert `(= (clay-buffer-get-bytes "bufferName")
            4))

; image type is RGBA
; FIXME: the docs erroneously say "ImageBaseType" => "ImageType"
(assert `(= (clay-buffer-get-image-type "bufferName")
            RGBA-IMAGE))



(test! "buffer-rename")

; Renaming returns the given name if it doesn't clash with existing name.
(assert `(string=? (clay-buffer-rename "bufferName" "renamedName")
                  "renamedName"))

; Effect renaming: clay knows the renamed name
(assert `(string=? (car (clay-buffers-get-name-list ""))
                    "renamedName"))

; Renaming does not add another buffer
(assert `(= (length (clay-buffers-get-name-list ""))
            1))
(display (clay-buffers-get-name-list ""))


(test! "buffer-delete")

; Delete evaluates but is void
(assert `(clay-buffer-delete "renamedName"))

; Delete was effective: clay no longer knows
; and returns nil i.e. empty list (())
(assert `(null? (clay-buffers-get-name-list "")))


; TODO test two buffers

; TODO test renaming when name already in use

(script-fu-use-v2)
