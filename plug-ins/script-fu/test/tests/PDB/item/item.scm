; test item methods of PDB

; Define function that is a sequence of tests.
; Iterate over items of different types, applying test function.
;    1. test attributes of a minimal item
;    2. test transformations of item

; Test of clay-item-is-<ItemType> are elsewhere

; Test of tree/group (raise/lower, reorder) are elsewhere




; Implementation of test:
; function using assert must backquote ` and unquote , item.



; Test methods of bare, minimal item.
(define (test-bare-item item)
  ; item is a numeric ID is valid
  (assert `(clay-item-id-is-valid ,item))

  ; item is not a group (can have no children)
  (assert `(= (car (clay-item-is-group ,item))
              0))

  ; item has no color tag
  (assert `(= (car (clay-item-get-color-tag ,item))
              COLOR-TAG-NONE))

  ; item is not expanded
  (assert `(= (car (clay-item-get-expanded ,item))
              0))

  ; item has name, tattoo
  ; Test does not check returned value
  (assert `(clay-item-get-name ,item))
  (assert `(clay-item-get-tattoo ,item))

  ; item has no parasites, yields no list of string
  ; !!! C GStrv marshaled to empty list
  ; Scheme null? tests for empty list
  (assert `(null? (car (clay-item-get-parasite-list ,item))))

  ; item has no parent
  ; yields -1 for NULL ID
  (assert `(= (car (clay-item-get-parent ,item))
              -1))

  ; item has-a image
  ; Test does not compare item ID
  (assert `(clay-item-get-image ,item))

  ; item's content, position, visibility is not locked
  (assert `(= (car (clay-item-get-lock-content ,item))
              0))
  (assert `(= (car (clay-item-get-lock-position ,item))
              0))
  (assert `(= (car (clay-item-get-lock-visibility ,item))
              0))
)


; Test methods of image,item
(define (test-item-in-image image item)
  ; item can produce a selection
  (assert `(clay-image-select-item
              ,image
              CHANNEL-OP-ADD
              ,item))
)



; !!! ClayParasite does not have method new in PDB.
; But you can create one in ScriptFu as (list "name" <flags> "data")
; <flags>
; 0 - Not persistent and not UNDOable
; 1 - Persistent and not UNDOable
; 2 - Not persistent and UNDOable
; 3 - Persistent and UNDOable

; https://www.clayusers.com/forums/clay-user/12970-how-are-parasites-represented-in-script-fu
; https://www.mail-archive.com/clay-user@lists.xcf.berkeley.edu/msg20099.html

; A returned parasite in ScriptFu is-a list (list "name" <flags> "data")

; You can use this in testing but requires (quote ,testParasite) ???
;(define testParasite (list "Parasite New" 1 "Parasite Data"))

(define (test-item-parasite item)

  ; not has-a parasite
  ; !!! procedure expected to fail when no parasite
  (assert-error `(clay-item-get-parasite
                  ,item
                  "Test Parasite") ; name
                "Procedure execution of clay-item-get-parasite failed")

  ; can attach parasite
  (assert `(clay-item-attach-parasite
              ,item
              (list "Parasite New" 1 "Parasite Data")))
  ; attach was effective: now item has parasite
  ; and its name is as previously given
  (assert `(string=?
              ; !!! Parasite is list in list, and first element is name
              (caar (clay-item-get-parasite
                        ,item
                        "Parasite New")) ; name
              "Parasite New"))

  ; can detach parasite
  (assert `(clay-item-detach-parasite
              ,item
              "Parasite New"))
  ; detach was effective
  (assert-error `(clay-item-get-parasite
                  ,item
                  "Test Parasite") ; name
                "Procedure execution of clay-item-get-parasite failed")
)


; OLD use image,item instance extant from previous tests.

;        setup

; All the items in the same testImage
; See earlier tests, where setup is lifted from

(define testImage (testing:load-test-image "clay-logo.png"))
(define testLayer (vector-ref (car (clay-image-get-layers testImage ))
                                  0))
(define testSelection (car (clay-image-get-selection testImage)))
(define testFont (car (clay-context-get-font)))
(define
  testTextLayer
   (car (clay-text-font
              testImage
              -1     ; drawable.  -1 means NULL means create new text layer
              0 0   ; coords
              "bar" ; text
              1     ; border size
              1     ; antialias true
              31    ; fontsize
              testFont )))
(define testChannel (car (clay-channel-new
            testImage      ; image
            "Test Channel" ; name
            23 24          ; width, height
            50.0           ; opacity
            "red" )))      ; compositing color
; must add to image
(clay-image-insert-channel
            testImage
            testChannel
            0            ; parent, moot since channel groups not supported
            0)
(define
  testLayerMask
    (car (clay-layer-create-mask
                    testLayer
                    ADD-MASK-WHITE)))
; must add to layer
(clay-layer-add-mask
            testLayer
            testLayerMask)
(define testPath (car (clay-path-new testImage "Test Path")))
; must add to image
(clay-image-insert-path
                  testImage
                  testPath
                  0 0) ; parent=0 position=0



;                 tests start here

; layer
(test-bare-item testLayer)
(test-item-in-image testImage testLayer)
(test-item-parasite testLayer)

; text layer
(test-bare-item testTextLayer)
(test-item-in-image testImage testTextLayer)
(test-item-parasite testTextLayer)

; layerMask
(test-bare-item testLayerMask)
(test-item-in-image testImage testLayerMask)
(test-item-parasite testLayerMask)

; path
(test-bare-item testPath)
(test-item-in-image testImage testPath)
(test-item-parasite testPath)

; channel
(test-bare-item testChannel)
(test-item-in-image testImage testChannel)
(test-item-parasite testChannel)

; selection
(test-bare-item testSelection)
(test-item-in-image testImage testSelection)
(test-item-parasite testSelection)

; TODO other item types e.g. ?

; clay-image-get-item-position
; clay-image-raise-item
; clay-image-raise-item-to-top
; lower
; reorder
