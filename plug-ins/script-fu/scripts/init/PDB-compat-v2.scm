; Deprecated and should not be used in new scripts.
; Clay developers strongly recommend you not use this file
; in scripts that you will distribute to other users.

; This file defines aliases.
; To help make old ScriptFu scripts more compatible with Clay PDB version 3.
; The aliases redirect old PDB procedure names to a new name.

; This script might omit less commonly used PDB procedures.
; This might omit PDB procedures where the signature changed.

; This file is NOT automatically loaded by ScriptFu.
; A script can load this file at runtime like this:
;
; (define my-plug-in-run-func
;     (load (string-append
;              script-fu-sys-init-directory
;              DIR-SEPARATOR
;              "PDB-compat-v2.scm"))
;     ...
; )
;
; Note this puts the definitions in execution-scope, not global scope.
; They will affect all functions called.
; They can affect ScriptFu plugin scripts called.
; They will go out of scope when the run-func completes.

; See also the companion file SIOD-compat.scm
; which defines Scheme functions for compatibility with the SIOD dialect.



(define clay-brightness-contrast               clay-drawable-brightness-contrast       )
(define clay-brushes-get-brush                 clay-context-get-brush                  )

(define clay-drawable-is-channel               clay-item-id-is-channel                 )
(define clay-drawable-is-layer                 clay-item-id-is-layer                   )
(define clay-drawable-is-layer-mask            clay-item-id-is-layer-mask              )
(define clay-drawable-is-text-layer            clay-item-id-is-text-layer              )
(define clay-drawable-is-valid                 clay-item-id-is-valid                   )
(define clay-drawable-transform-2d             clay-item-transform-2d                  )
(define clay-drawable-transform-flip           clay-item-transform-flip                )
(define clay-drawable-transform-flip-simple    clay-item-transform-flip-simple         )
(define clay-drawable-transform-matrix         clay-item-transform-matrix              )
(define clay-drawable-transform-perspective    clay-item-transform-perspective         )
(define clay-drawable-transform-rotate         clay-item-transform-rotate              )
(define clay-drawable-transform-rotate-simple  clay-item-transform-rotate-simple       )
(define clay-drawable-transform-scale          clay-item-transform-scale               )
(define clay-drawable-transform-shear          clay-item-transform-shear               )

(define clay-display-is-valid                  clay-display-id-is-valid                )

(define clay-image-is-valid                    clay-image-id-is-valid                  )
(define clay-image-freeze-vectors              clay-image-freeze-paths                 )
(define clay-image-get-vectors                 clay-image-get-paths                    )
(define clay-image-get-selected-vectors        clay-image-get-selected-paths           )
(define clay-image-set-selected-vectors        clay-image-set-selected-paths           )
(define clay-image-thaw-vectors                clay-image-thaw-paths                   )

(define clay-item-is-channel                   clay-item-id-is-channel                 )
(define clay-item-is-drawable                  clay-item-id-is-drawable                )
(define clay-item-is-layer                     clay-item-id-is-layer                   )
(define clay-item-is-layer-mask                clay-item-id-is-layer-mask              )
(define clay-item-is-selection                 clay-item-id-is-selection               )
(define clay-item-is-text-layer                clay-item-id-is-text-layer              )
(define clay-item-is-valid                     clay-item-id-is-valid                   )
(define clay-item-is-vectors                   clay-item-id-is-path                    )
(define clay-item-id-is-vectors                clay-item-id-is-path                    )

(define clay-vectors-new                       clay-path-new                           )
; ? missing others where "vectors" renamed to "path"

(define clay-layer-group-new                   clay-group-layer-new                    )
; ? missing others where "layer-group" renamed to "group-layer"

(define clay-procedural-db-dump                clay-pdb-dump                           )
(define clay-procedural-db-get-data            clay-pdb-get-data                       )
(define clay-procedural-db-set-data            clay-pdb-set-data                       )
; Obsolete: clay-procedural-db-get-data-size
; Just call clay-pdb-get-data and in Scheme find its size
(define clay-procedural-db-proc-arg            clay-pdb-get-proc-argument              )
(define clay-procedural-db-proc-info           clay-pdb-get-proc-info                  )
(define clay-procedural-db-proc-val            clay-pdb-get-proc-return-value          )
(define clay-procedural-db-proc-exists         clay-pdb-proc-exists                    )
(define clay-procedural-db-query               clay-pdb-query                          )
(define clay-procedural-db-temp-name           clay-pdb-temp-name                      )

(define clay-image-get-exported-uri            clay-image-get-exported-file            )
(define clay-image-get-imported-uri            clay-image-get-imported-file            )
(define clay-image-get-xcf-uri                 clay-image-get-xcf-file                 )
(define clay-image-get-filename                clay-image-get-file                     )
(define clay-image-set-filename                clay-image-set-file                     )

(define clay-plugin-menu-register              clay-pdb-add-proc-menu-path             )
(define clay-plugin-get-pdb-error-handler      clay-plug-in-get-pdb-error-handler      )
(define clay-plugin-help-register              clay-plug-in-help-register              )
(define clay-plugin-menu-branch-register       clay-plug-in-menu-branch-register       )
(define clay-plugin-set-pdb-error-handler      clay-plug-in-set-pdb-error-handler      )

(define clay-plugins-query                     clay-plug-ins-query                     )
(define file-gtm-save                          file-html-table-export                  )
(define python-fu-histogram-export             histogram-export                        )
(define python-fu-gradient-save-as-css         gradient-save-as-css                    )