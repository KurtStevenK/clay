; test methods of display

; A display is a tabbed window in the Clay app.
; Expect all the API calls are impotent if called in batch
; i.e. when there is no GUI Clay app.
; But we don't test that yet.

; A display is represented by numeric ID in ScriptFu.
; The space of ID's for displays overlaps the space of ID's for images.
; I.E. 1 can be both an ID for an image and display, at the same time.

; Testing has visible results: displays appear as tabs in the Clay app

; Testing might throw GLib CRITICAL; must not export G_DEBUG=fatal_criticals

(script-fu-use-v3)



; setup

; an image
(define testImage (testing:load-test-image-basic-v3))
; loaded "clay-logo.png"
; a second image
(define testImage2 (clay-image-new 21 22 RGB))


; new

; new succeeds
(assert `(clay-display-new ,testImage))

; store the ID of another new display on same image
(define testDisplay (clay-display-new testImage))

; new display has a valid ID
(assert `(clay-display-id-is-valid ,testDisplay))

; can get many displays of same image
(assert `(clay-display-new ,testImage))



; id-is-valid returns false for an invalid ID
(assert `(not (clay-display-id-is-valid 666)))


; misc

; has-a window-handle, which is a GBytes, i.e. a vector of numbers
; FIXME the docs for the API says the type varies by platform, but it is always GBytes?
; What varies is the interpretations of the GBytes:
; as sequence of characters, or as sequence of bytes of a multi-bit number
(assert `(vector? (clay-display-get-window-handle ,testDisplay)))
; get-window-handle is safe from invalid ID
(assert-error `(clay-display-get-window-handle 666)
                "Invalid value for argument 0")

; flush succeeds
(assert `(clay-displays-flush))

; present succeeds
; TODO what does "present" mean?
; I suppose "bring to front", when there are many tabbed displays
(assert `(clay-display-present ,testDisplay))
; present is safe from invalid ID

(assert-error `(clay-display-present 666)
              "Invalid value for argument 0")



; reconnect
; reconnect (to-image from-image)

; illegal to reconnect from an image that already has a display
; testImage is Wilber.png and is in two displays i.e. tabs
(assert-error `(clay-displays-reconnect ,testImage ,testImage )
              "Procedure execution of clay-displays-reconnect failed")

; illegal to reconnect to an image that is not displayed
; testImage2 is not displayed
(assert-error `(clay-displays-reconnect ,testImage2 ,testImage )
              "Procedure execution of clay-displays-reconnect failed")


; reconnect succeeds
; Make displays of clay-logo now show the testImage2 that is not now displayed
(assert `(clay-displays-reconnect ,testImage ,testImage2 ))
; display ID is still valid
(assert `(clay-display-id-is-valid ,testDisplay))
; effective.  TODO no API to know what image is in a display.

; reconnect is safe from invalid image ID
(assert-error `(clay-displays-reconnect 666 666)
              "Invalid value for argument 0")



; delete

; succeeds
(assert `(clay-display-delete ,testDisplay))
; effective: display ID is no longer valid
(assert `(not (clay-display-id-is-valid ,testDisplay)))
; safe from invalid ID
(assert-error `(clay-display-delete ,testDisplay)
              "Invalid value for argument 0")


; Testing leaves two displays visible in Clay app

(script-fu-use-v2)
