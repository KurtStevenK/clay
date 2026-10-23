#!/usr/bin/env clay-script-fu-interpreter-3.0

; Calls scheme (display ...)
; Tests it prints to any stdout console where Clay was started.

; Then calls (error ...)
; Tests Clay declares an error.

; in v2, (display ...) did not go to the terminal,
; but was prepended to any error message,
; or disappeared when there was no error.



(define (script-fu-test-display)

  ;       test display function

  ; display shows a passed string
  (display "foo")

  ; display shows repr of any atom or list
  (display '(1 2 "bar"))

  ; print is not defined in R5RS
  ; same as display but adds newline

  (clay-message "Called display: expect foo(1 2 bar)#<CLOSURE> in terminal")



  ;       test error function

  ; Call to error yields:
  ;    dialog when Clay Error Console not open
  ;    else text in the open Clay Error Console
  (clay-message "Called error: expect Clay dialog, OR error in Clay Error Console.")
  ; Scheme objects print their representation.
  ; Here clay-message should print as #<CLOSURE>
  (error "Reason" clay-message)

  ; Call to error returns to Clay, this should not be reached.
  (clay-message "Test failed: unreachable statement was reached.")
)

(script-fu-register "script-fu-test-display"
    "Test scheme display and error functions"
    "Test (display ...) to console, and (error ...) returns err to Clay"
    "lkk"
    "lkk"
    "2024"
    ""
)

(script-fu-menu-register "script-fu-test-display"
                         "<Image>/Filters/Development/Demos")
