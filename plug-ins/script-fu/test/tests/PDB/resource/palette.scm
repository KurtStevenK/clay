; Test methods of palette subclass of Resource class

; !!! See also resource.scm

; !!! Testing depends on a fresh install of Clay.
; A prior testing failure may leave palettees in Clay.
; The existing palette may have the same name as hard coded in tests.
; In future, will be possible to create new palette with same name as existing.


(script-fu-use-v3)

; setup, not assert
; but tests the -new method
(define testNewPalette (clay-palette-new "testNewPalette"))




(test! "attributes of new palette")

; clay-palette-get-background deprecated => clay-context-get-background
; ditto foreground

; new palette has given name
; !!! Fails if not a fresh install, then name is like "testNewPalette #2"
(assert `(string=?
            (clay-resource-get-name ,testNewPalette)
            "testNewPalette"))

; new palette has zero colors
(assert `(= (clay-palette-get-color-count ,testNewPalette)
            0))

; new palette has empty colormap
; v2 returns (0 #())
; v3 returns #()
(assert `(= (vector-length (clay-palette-get-colors ,testNewPalette))
            0))

(test! "new palette has zero columns")
; procedure returns just the column count
(assert `(= (clay-palette-get-columns ,testNewPalette)
            0))

; new palette is-editable
; method on Resource class
(assert `(clay-resource-is-editable ,testNewPalette))

; can set new palette in context
; Despite having empty colormap
; returns void
(assert `(clay-context-set-palette ,testNewPalette))




(test! "attributes of existing palette named Bears")

; setup
(define testBearsPalette (clay-palette-get-by-name "Bears"))


; Max size palette is 256

; Bears palette has 256 colors
(assert `(= (clay-palette-get-color-count ,testBearsPalette)
            256))

; Bears palette colormap array is size 256 vector of 3-tuple lists
; v2 get_colors returns (256 #((8 8 8) ... ))
; v3            returns #((8 8 8) ... )
(assert `(= (vector-length (clay-palette-get-colors ,testBearsPalette))
            256))

; Bears palette has zero column count
; The procedure returns a count, and not the columns
(assert `(= (clay-palette-get-columns ,testBearsPalette)
            0))

; system palette is not editable
; returns #f
(assert `(not (clay-resource-is-editable ,testBearsPalette)))


;              setting attributes of existing palette

; Can not change column count on system palette
(assert-error `(clay-palette-set-columns ,testBearsPalette 1)
              "Procedure execution of clay-palette-set-columns failed")


;              add entry to full system palette

; error to add entry to palette which is non-editable and has full colormap
(assert-error `(clay-palette-add-entry ,testBearsPalette "fooEntryName" "red")
                "Procedure execution of clay-palette-add-entry failed ")



 ;             setting attributes of new palette

; succeeds
(assert `(clay-palette-set-columns ,testNewPalette 1))

; effective
(assert `(= (clay-palette-get-columns ,testNewPalette)
            1))


(test! "adding color entry to new palette")

; add first entry returns index 0
; v2 result is wrapped (0)
(assert `(= (clay-palette-add-entry ,testNewPalette "fooEntryName" "red")
            0))

; was effective: color is as given when entry created
; v3 returns (255 0 0)
; renamed clay-palette-entry-get-color=>clay-palette-get-entry-color
(assert `(equal? (clay-palette-get-entry-color ,testNewPalette 0)
                  (list 255 0 0 255)))  ; red
(display (clay-palette-get-entry-color testNewPalette 0))

; was effective on name
(assert `(string=? (clay-palette-get-entry-name ,testNewPalette 0)
                  "fooEntryName"))



(test! "delete colormap entry")

; succeeds
; FIXME: the name seems backward, could be entry-delete
; returns void
(assert `(clay-palette-delete-entry  ,testNewPalette 0))
; effective, color count is back to 0
(assert `(= (clay-palette-get-color-count ,testNewPalette)
            0))


;               adding color "entry" to new palette which is full


; TODO locked palette?  See issue about locking palette?





(test! "delete palette")

; can delete a new palette
(assert `(clay-resource-delete ,testNewPalette))

; delete was effective
; ID is now invalid
(assert `(not (clay-resource-id-is-palette ,testNewPalette)))

; delete was effective
; not findable by name anymore
; If the name DOES exist (because not started fresh) yields "substring out of bounds"
; Formerly returned error, now returns NULL i.e. -1
;(assert-error `(clay-palette-get-by-name "testNewPalette")
;              "Procedure execution of clay-palette-get-by-name failed on invalid input arguments: Palette 'testNewPalette' not found")
(assert `(= (clay-palette-get-by-name "testNewPalette")
            -1))



; see context.scm




;                   test deprecated methods

; These should give warnings in Clay Error Console.
; Now they are methods on Context, not Palette.

;(clay-palettes-set-palette testBearsPalette)

;(clay-palette-swap-colors)
;(clay-palette-set-foreground "pink")
;(clay-palette-set-background "purple")


(script-fu-use-v2)









