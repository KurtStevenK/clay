#!/usr/bin/env clay-script-fu-interpreter-3.0

; A script that always fails
;
; Setup: copy this file w/ executable permission, and its parent dir to /plug-ins
; Example: to ~/.clay-2.99/plug-ins/always-fail/always-fail.scm

; Expect "Test>Always fail" in the menus
; Expect when chosen, message on Clay message bar "Failing"
; Expect a dialog in Clay app that requires an OK

(define (script-fu-always-fail)
  (begin
    (clay-message "Failing")
    ; since last expression, the result, and should mean error
    #f
  )
)

(script-fu-register "script-fu-always-fail"
  "Always fail"
  "Expect error dialog in Clay, or PDB execution error when called by another"
  "lkk"
  "lkk"
  "2022"
  ""  ; requires no image
  ; no arguments or dialog
)

(script-fu-menu-register "script-fu-always-fail" "<Image>/Test")
