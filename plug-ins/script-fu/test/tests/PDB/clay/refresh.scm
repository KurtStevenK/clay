; test refresh methods

; make the app read resources from configuration files

; methods of the app
; the app manages collections of resources
; app can refresh and list the resources.

; A collection is named by the plural of the singular element,
; i.e. brushes is a collection of brush.




; Deprecations:
; clay-palette-refresh
; clay-brushes-list => clay-brushes-get-list etc.
; clay-parasite-list => clay-get-parasite-list



(script-fu-use-v3)

(test! "refresh resources")

; always succeeds
; Returns #t
(assert `(clay-brushes-refresh))
(assert `(clay-dynamics-refresh))
(assert `(clay-fonts-refresh))
(assert `(clay-gradients-refresh))
(assert `(clay-palettes-refresh))
(assert `(clay-patterns-refresh))


(test!  "list resources")

; always succeeds
; Takes an optional regex string.
; Returns a vector of object ID's.
; !!! The name says its a list, but its an array
(assert `(vector? (clay-brushes-get-list "")))
(assert `(vector? (clay-fonts-get-list "")))
(assert `(vector? (clay-gradients-get-list "")))
(assert `(vector? (clay-palettes-get-list "")))
(assert `(vector? (clay-patterns-get-list "")))


;            listing app's collection of things not resources
; But taking a regex

; Returns list of names, not a vector of object ID's
(assert `(list? (clay-dynamics-get-name-list "")))
(assert `(list? (clay-buffers-get-name-list "")))


;           listing app's other collections not resources
; Not taking a regex

; FIXME the naming does not follow the pattern, should be plural parasites
; Not: (clay-parasites-get-list "")
(assert `(list? (clay-get-parasite-list)))

; the app, images, vectors, drawables, items
; can all have parasites.
; Tested elsewhere.

(test! "get images")

; clay-get-images does not follow the pattern:
; it doesn't take a regex
; and it returns a vector of image objects #()
(assert `(vector? (clay-get-images)))


(script-fu-use-v2)