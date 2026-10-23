; test methods of PDB about PDBProcedure

; Prefix of methods is "clay-pdb-proc"

; Tests in alphabetic order of attribute name

; Asymmetry:
; - some attributes can be get, but not set
; - some attributes can be set but not get
; Such attributes are set only in libclay, not by a PDB procedure

; Dependent on ProcedureType
; ProcedureType:Internal procedures lack some attributes e.g. menu_label
; ProcedureType:Plugin procedures have those attributes

; There is no PDB API to create a procedure
; and no setters of the arguments and return values.
; There are setters of other attributes.

; Setters like clay-pdb-set-file-proc- are not tested.



; Getters
;
; Getters of argument and return value types, useful for introspection.
; Returns GParamSpec
; FIXME crashes, ScriptFu does not marshall return type GParamSpec
; (assert `(clay-pdb-get-proc-argument "clay-pdb-get-proc-argument" 0)
; (assert `(clay-pdb-get-proc-return-value "clay-pdb-get-proc-argument" 0)

; Other getters

; Getters that return lists of strings of no real significance
; Every procedure of any type has the attribute and it is generally not empty string.
(assert `(clay-pdb-get-proc-attribution "clay-pdb-get-proc-argument"))
(assert `(clay-pdb-get-proc-documentation "clay-pdb-get-proc-argument"))


; image types is a string with known encoding
; typically RGB, GRAY, RGBA, * (any), "" (none), or combinations

; Error to ask for image-types of ProcedureType:Internal
(assert-error `(clay-pdb-get-proc-image-types "clay-pdb-get-proc-argument")
              "Procedure execution of clay-pdb-get-proc-image-types failed")
; Works for ProcedureType:Plugin
(assert `(clay-pdb-get-proc-image-types "plug-in-script-fu-console"))
; Empty string means ScriptFu Console can be invoked when no images open
(assert `(string=? (car (clay-pdb-get-proc-image-types "plug-in-script-fu-console"))
                    ""))


; first return value is integer from enumeration ClayPDBProcType
(assert `(number? (car (clay-pdb-get-proc-info "clay-pdb-get-proc-argument"))))
; clay-pdb-get-proc-argument is ProcedureType:Internal
(assert `(= (car (clay-pdb-get-proc-info "clay-pdb-get-proc-argument"))
            PDB-PROC-TYPE-INTERNAL))
; plug-in-script-fu-console is ProcedureType:Plugin
(assert `(= (car (clay-pdb-get-proc-info "plug-in-script-fu-console"))
            PDB-PROC-TYPE-PLUGIN))


; Error to ask for the menu_label of ProcedureType:Internal
(assert-error `(clay-pdb-get-proc-menu-label "clay-pdb-get-proc-argument")
              "Procedure execution of clay-pdb-get-proc-menu-label failed")
; Error to ask for the menu-paths of ProcedureType:Internal
(assert-error `(clay-pdb-get-proc-menu-paths "clay-pdb-get-proc-argument")
              "Procedure execution of clay-pdb-get-proc-menu-paths failed")


; Not an error for ProcedureType:Plugin.
; Returns single string.
; The string may be encoded with tag "_" for keyboard shortcuts
(assert `(string=? (car (clay-pdb-get-proc-menu-label "plug-in-script-fu-console"))
                    "Script-Fu _Console"))
(assert `(clay-pdb-get-proc-menu-paths "plug-in-script-fu-console"))
; Returns possibly long list of strings.
; a procedure can be installed at many menu paths.
; !!! FUTURE: returns a doubly wrapped list
(assert `(string=? (caar (clay-pdb-get-proc-menu-paths "plug-in-script-fu-console"))
                  "<Image>/Filters/Development/Script-Fu"))


; Setters cannot be called on a procedure that is not owned by current plugin.
; Current plugin during test is e.g. ScriptFu Console
; and it owns/installed the procedure: plug-in-script-fu-console
(assert-error `(clay-pdb-set-proc-attribution "clay-pdb-get-proc-argument" "author" "foo" "bar")
              "Procedure execution of clay-pdb-set-proc-attribution failed on invalid input arguments: ")
              ; ... It has however not installed that procedure. This is not allowed.

; Setters work if the procedure is owned/installed by the current procedure
; i.e. ProcedureType:Temporary installed by extension-script-fu.
; Unable to test, since the current procedure during test varies
; and doesn't own any temporary procedures.
; This doesn't work:
; (assert `(clay-pdb-set-proc-attribution "plug-in-script-fu-console" "author" "copyright" "date"))



; Signatures of other setters, all return void
;clay-pdb-set-proc-attribution ( gchararray gchararray gchararray gchararray ) =>
;clay-pdb-set-proc-documentation ( gchararray gchararray gchararray gchararray ) =>
;clay-pdb-set-proc-icon ( gchararray ClayIconType gint ClayUint8Array ) =>
;clay-pdb-set-proc-image-types ( gchararray gchararray ) =>
;clay-pdb-set-proc-menu-label ( gchararray gchararray ) =>
; Set but not get
;clay-pdb-set-proc-sensitivity-mask ( gchararray gint ) =>