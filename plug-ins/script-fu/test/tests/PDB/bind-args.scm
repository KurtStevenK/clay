
; Test ScriptFu's binding to all Clay C arg types of the PDB


; The PDB procedures called are arbitrary, chosen for the type of their args.

; The test is only that no error is thrown, not necessarily that the call is effective.

; Test binding in both directions: args passed and args returned.

; Testing is not complete, but illustrative of special cases.

; Testing is not blindly exhaustive of every type declarable for PDB procedures.
; Testing is with knowledge of the code.
; Only testing representatives for cases in switch statement of scheme-wrapper.c.
; For example, the code has a case for GObject that covers most subclasses
; of ClayItem, so we only test once, say for ClayLayer.

; Also, we don't test all primitive types.
; We know they are tested drive-by in other tests,
; so we don't necessarily test them here.
; Int, String, Double, UInt

; Note that no PDB procedure takes or returns:
;    gchar (the type for a single character.)
;    GParam or ClayParam
; There is no case in scheme-wrapper.c.



; int
; float

; Colors (GeglColor) are tested e.g. with Palette
; ClayColorArray is tested e.g.
; from palette-get-colormap
; to is not tested: not an arg to any PDB proc

; GStrv string array
; from brushes-get-list
; to file-gih-export or extension-clay-help
; TODO test GStrv to file-gih-export

; GBytes
; from image-get-colormap
; to image-set-colormap

; FloatArray
; from clay-context-get-line-dash-pattern
; to clay-context-set-line-dash-pattern


;                 ClayResource
; see resource.scm and context.scm


;                 GFile

;                 ClayParasite

; ScriptFu takes and returns a list of attributes of a ClayParasite
; A ClayParasite is a named string having a flags attribute ?
; Also tested elsewhere, many objects can have parasites.
; This tests the global parasites, on the clay instance.

; to
(assert '(clay-attach-parasite (list "foo" 1 "zed")))
; from
(assert `(equal? (car (clay-get-parasite "foo"))
                 '("foo" 1 "zed")))


;                 ClayUnit

; A ClayUnit is both an enum and an object???
; ScriptFu converts to int.  More or less an ID.

; obsolete: ClayUnit is now an object
; to
; unit index 0 is px
;(assert '(string=? (car (clay-unit-get-abbreviation 0))
;                   "px"))

; from
; default line width unit is px
(assert '(= (car (clay-context-get-line-width-unit))
            0))

