;; -*-scheme-*-

(define (script-fu-guides-remove image drawables)
  (let* ((guide-id 0))
    (clay-image-undo-group-start image)

    (set! guide-id (car (clay-image-find-next-guide image 0)))
    (while (> guide-id 0)
      (clay-image-delete-guide image guide-id)
      (set! guide-id (car (clay-image-find-next-guide image 0)))
    )

    (clay-image-undo-group-end image)
    (clay-displays-flush)
  )
)

(script-fu-register-filter "script-fu-guides-remove"
  _"_Remove all Guides"
  _"Remove all horizontal and vertical guides"
  "Alan Horkan"
  "Alan Horkan, 2004. Public Domain."
  "April 2004"
  "*"
  SF-ONE-OR-MORE-DRAWABLE
)

(script-fu-menu-register "script-fu-guides-remove"
                         "<Image>/Image/Guides")
