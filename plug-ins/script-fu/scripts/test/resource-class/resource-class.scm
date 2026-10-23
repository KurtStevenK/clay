#!/usr/bin/env clay-script-fu-interpreter-3.0

; A script that tests resource classes in Clay
; Tests the marshalling of parameters and return values in ScriptFu
;
; Setup: copy this file w/ executable permission, and its parent dir to /plug-ins
; Example: to ~/.clay-2.99/plug-ins/always-fail/always-fail.scm

; Delete .config/Clay so that resources are in a standard state.

; Expect various resource names in the console
; Expect no "Fail" in the console


(define (script-fu-test-resource-class)

  (define (expect expression
                  expected-value )
      ; use equal?, don't use eq?
      (if (equal? expression expected-value)
          #t
          (clay-message "Fail")
      )
  )

  ; redirect messages to the console
  (clay-message-set-handler 1)

  (let* (
        ; Test as a return value
        ; These calls return a list with one element, use car
        (brush    (car (clay-context-get-brush)))
        (font     (car (clay-context-get-font)))
        (gradient (car (clay-context-get-gradient)))
        (palette  (car (clay-context-get-palette)))
        (pattern  (car (clay-context-get-pattern)))

        ; font and pattern cannot be new(), duplicate(), delete()

        ; new() methods
        (brushnew    (car (clay-brush-new    "Brush New")))
        (gradientnew (car (clay-gradient-new "Gradient New")))
        (palettenew  (car (clay-palette-new  "Palette New")))

        ; copy() methods
        ; copy method is named "duplicate"
        ; Takes an existing brush and a desired name
        (brushcopy    (car (clay-brush-duplicate    brushnew    "brushcopy")))
        (gradientcopy (car (clay-gradient-duplicate gradientnew "gradientcopy")))
        (palettecopy  (car (clay-palette-duplicate  palettenew  "palettecopy")))

        ; See below, we test rename later
        )

    ; write names to console
    (clay-message brush)
    (clay-message font)
    (clay-message gradient)
    (clay-message palette)
    (clay-message pattern)

    (clay-message brushnew)
    (clay-message gradientnew)
    (clay-message palettenew)

    (clay-message brushcopy)
    (clay-message gradientcopy)
    (clay-message palettecopy)

    ; Note equal? works for strings, but eq? and eqv? do not
    (clay-message "Expect resources from context have de novo installed Clay names")
    (expect (equal? brush "2. Hardness 050")   #t)
    (expect (equal? font "Sans-serif")         #t)
    (expect (equal? gradient "FG to BG (RGB)") #t)
    (expect (equal? palette "Color History")   #t)
    (expect (equal? pattern "Pine") #t)

    (clay-message "Expect new resource names are the names given when created")
    (expect (equal? brushnew    "Brush New")    #t)
    (expect (equal? gradientnew "Gradient New") #t)
    (expect (equal? palettenew  "Palette New")  #t)

    (clay-message "Expect copied resources have names given when created")
    ; !!! TODO Clay appends " copy" and does not use the given name
    ; which contradicts the docs for the procedure
    (expect (equal? brushcopy    "Brush New copy")    #t)
    (expect (equal? gradientcopy "Gradient New copy") #t)
    (expect (equal? palettecopy  "Palette New copy")  #t)

    ; rename() methods
    ; Returns new resource proxy, having possibly different name than requested
    ; ScriptFu marshals to a string
    ; !!! Must assign it to the same var,
    ; else the var becomes an invalid reference since it has the old name
    (set! brushcopy    (car (clay-brush-rename    brushcopy    "Brush Copy Renamed")))
    (set! gradientcopy (car (clay-gradient-rename gradientcopy "Gradient Copy Renamed")))
    (set! palettecopy  (car (clay-palette-rename  palettecopy  "Palette Copy Renamed")))

    ; write renames to console
    (clay-message brushcopy)
    (clay-message gradientcopy)
    (clay-message palettecopy)

    (clay-message "Expect renamed have new names")
    (expect (equal? brushcopy    "Brush Copy Renamed")    #t)
    (expect (equal? gradientcopy "Gradient Copy Renamed") #t)
    (expect (equal? palettecopy  "Palette Copy Renamed")  #t)

    (clay-message  "Expect class method id_is_valid of the ClayResource class")
    ; the class method takes a string.
    ; ScriptFu already has a string var, and marshalling is trivial
    ; For now, returns (1), not #t
    (expect (car (clay-brush-id-is-valid    brush))    1)
    (expect (car (clay-font-id-is-valid     font))     1)
    (expect (car (clay-gradient-id-is-valid gradient)) 1)
    (expect (car (clay-palette-id-is-valid  palette))  1)
    (expect (car (clay-pattern-id-is-valid  pattern))  1)

    (clay-message "Expect class method id_is_valid for invalid name")
    ; Expect false, but no error dialog from Clay
    ; Returns (0), not #f
    (expect (car (clay-brush-id-is-valid    "invalid_name")) 0)
    (expect (car (clay-font-id-is-valid     "invalid_name")) 0)
    (expect (car (clay-gradient-id-is-valid "invalid_name")) 0)
    (expect (car (clay-palette-id-is-valid  "invalid_name")) 0)
    (expect (car (clay-pattern-id-is-valid  "invalid_name")) 0)

    (clay-message "Expect as a parameter to context works")
    ; Pass each resource class instance back to Clay
    (clay-context-set-brush    brush)
    (clay-context-set-font     font)
    (clay-context-set-gradient gradient)
    (clay-context-set-palette  palette)
    (clay-context-set-pattern  pattern)

    (clay-message "Expect delete methods work without error")
    ; call superclass method
    (clay-resource-delete brushnew)
    (clay-resource-delete gradientnew)
    (clay-resource-delete palettenew)

    (clay-message "Expect var holding deleted resource is still defined, but is invalid reference")
    ;  Returns (0), not #f
    (expect (car (clay-brush-id-is-valid    brushnew))    0)
    (expect (car (clay-gradient-id-is-valid gradientnew)) 0)
    (expect (car (clay-palette-id-is-valid  palettenew))  0)

    ; We don't test the specialized methods of the classes here, see elsewhere
  )
)

(script-fu-register "script-fu-test-resource-class"
  "Test resource classes of Clay"
  "Expect no errors in the console"
  "lkk"
  "lkk"
  "2022"
  ""  ; requires no image
  ; no arguments or dialog
)

(script-fu-menu-register "script-fu-test-resource-class" "<Image>/Test")
