; Test methods related to paint dynamics



; setup

; an image, drawable, and path
(define testImage (testing:load-test-image "clay-logo.png"))
(define testLayer (vector-ref (car (clay-image-get-layers testImage ))
                                  0))
(define testPath (car (clay-path-new testImage "Test Path")))
; must add to image
(clay-image-insert-path
                  testImage
                  testPath
                  0 0) ; parent=0 position=0
; Add stroke to path
; TODO enum still named "VECTORS" => PATH
(clay-path-stroke-new-from-points
            testPath
            PATH-STROKE-TYPE-BEZIER
            (vector 1 2 83 84 5 6 7 8 9 10 11 12) ; control points
            FALSE) ; not closed

; make test harder by using float precision
(clay-image-convert-precision testImage PRECISION-DOUBLE-NON-LINEAR)
; ensure testing is stroking with paint (versus line)
(clay-context-set-stroke-method STROKE-PAINT-METHOD)
; ensure testing is painting with paintbrush (versus pencil, airbrush, etc.)
(clay-context-set-paint-method "clay-paintbrush")
; make test harder by using a big, color brush
(clay-context-set-brush (car (clay-brush-get-by-name "Wilber")))




; methods of the clay module

; introspection: clay module returns list of names of dynamics
; second arg is a regex
; FORMERLY get-list
(assert `(list? (clay-dynamics-get-name-list "")))

; refresh: clay module will load newly installed dynamics
; method is void and should never fail.
(assert `(clay-dynamics-refresh))

; TODO install a new dynamic and test that refresh is effective






; context setting

; the dynamics setting defaults to true
; !!! test requires freshly installed Clay OR no prior testing
(assert-PDB-true `(clay-context-are-dynamics-enabled))

; the dynamics-enabled setting can be set to false
; TODO #f instead of 0
(assert `(clay-context-enable-dynamics 0))
; setting to false was effective
(assert-PDB-false `(clay-context-are-dynamics-enabled))
; restore to enabled for further testing
(assert `(clay-context-enable-dynamics 1))


; the dynamics setting can be set to the name of a dynamics
(assert `(clay-context-set-dynamics-name "Tilt Angle"))
; setting was effective
; formerly context-[set,get]-dynamics
(assert `(string=? (car (clay-context-get-dynamics-name))
                   "Tilt Angle"))



;  TODO test all the dynamics seems to work

; Test that all dynamics seem to work:
;    set context to dynamics
;    stroke a drawable along a path with current brush and dynamics


(define dynamicsList (car (clay-dynamics-get-name-list "")))

(define (testDynamics dynamics)
    ; Test that every dynamics can be set on the context
    (clay-context-set-dynamics-name dynamics)

    (display dynamics)
    ; paint with paintbrush and dynamics, under the test harness
    (assert `(clay-drawable-edit-stroke-item ,testLayer ,testPath))
   )

; apply testDynamics to each dynamics kind.
; This is not a difficult test since the stroke is uniform w/r to dynamics.
; The stroke does not vary by e.g. pressure.
(for-each
  testDynamics
  dynamicsList)

;(clay-display-new testImage)
