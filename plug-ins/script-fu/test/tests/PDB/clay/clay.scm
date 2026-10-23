; Tests of clay module of the PDB
; These are not associated with an object class

; Generally speaking, prefix is "clay-get"

; since 3.0rc2 get-color-configuration is private to libclay
; The serialized color configuration is a string
; (assert '(string? (car (clay-get-color-configuration))))

; The default comment on images is a string
(assert '(string? (car (clay-get-default-comment))))

; The default unit is an integer enum for mm
(assert '(= (car (clay-get-default-unit))
            UNIT-MM))

; The list of open images is a vector of integer ID
; At test time, we don't know how many
(assert '(vector? (car (clay-get-images))))

; The list of modules to not be loaded is a list of strings
; Not double wrapped in a list, usually ("")
(assert '(list? (clay-get-module-load-inhibit)))

; the monitor resolution is two doubles e.g. (57.0 57.0)
; TODO in what units, pixels/in?
(assert '(real? (car (clay-get-monitor-resolution))))

; the PID is an integer
; FIXME should be renamed get-pid?
(assert '(number? (car (clay-getpid))))




; global parasites
; They persist from session to session

; the list of global parasites is a list
(assert '(list? (clay-get-parasite-list)))

; error to get by name not of a global parasite
(assert-error '(clay-get-parasite "never in the list")
              "Procedure execution of")

; Can create a named parasite on global namespace.
; See elsewhere, a Parasite is a list.
(assert '(clay-attach-parasite (list "foo" 1 "zed")))
; effective
(assert `(equal? (car (clay-get-parasite "foo"))
                 '("foo" 1 "zed")))

; Can reattach, regardless whether same name already attached.
(assert '(clay-attach-parasite (list "foo" 2 "bar")))
; effective
(assert `(equal? (car (clay-get-parasite "foo"))
                 '("foo" 2 "bar")))

; detaching by name destroys a parasite
(assert `(clay-detach-parasite "foo"))
; effective, name no longer valid
(assert-error '(clay-get-parasite "foo")
              "Procedure execution of")


