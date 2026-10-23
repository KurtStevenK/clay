; "Rippling Image" animation generator (ripply-anim.scm)
; Adam D. Moss (adam@foxbox.org)
; 97/05/18
; Revised by Saul Goode April 2015.
;
; Designed to be used in conjunction with a plugin capable
; of saving animations (i.e. the GIF plugin).
;

(define (script-fu-ripply-anim image drawables displacement num-frames edge-type)
  (let* ((drawable (vector-ref (car (clay-image-get-selected-drawables image)) 0))
         (width (car (clay-drawable-get-width drawable)))
         (height (car (clay-drawable-get-height drawable)))
         (work-image (car (clay-image-new width
                                          height
                                          (quotient (car (clay-drawable-type drawable))
                                                    2))))
         (map-layer (car (clay-layer-new work-image
                                         "Ripple Map"
                                         width
                                         height
                                         (car (clay-drawable-type drawable))
                                         100
                                         LAYER-MODE-NORMAL))))
    (clay-context-push)
    (clay-context-set-paint-mode LAYER-MODE-NORMAL)
    (clay-context-set-opacity 100.0)
    (clay-image-undo-disable work-image)

    ; Create a tile-able displacement map in the first layer
    (clay-context-set-background '(127 127 127))
    (clay-image-insert-layer work-image map-layer 0 0)
    (clay-drawable-edit-fill map-layer FILL-BACKGROUND)
    (clay-drawable-merge-new-filter map-layer "gegl:noise-rgb" 0 LAYER-MODE-REPLACE 1.0
                                    "independent" FALSE "red" 1.0 "alpha" 0.0
                                    "correlated" FALSE "seed" (msrg-rand) "linear" TRUE)
    (plug-in-tile #:run-mode RUN-NONINTERACTIVE #:image work-image #:drawables (vector map-layer) #:new-width (* width 3) #:new-height (* height 3) #:new-image FALSE)
    (clay-drawable-merge-new-filter map-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 11.2 "std-dev-y" 11.2 "filter" "auto")
    (clay-drawable-equalize map-layer TRUE)
    (clay-drawable-merge-new-filter map-layer "gegl:gaussian-blur" 0 LAYER-MODE-REPLACE 1.0 "std-dev-x" 1.6 "std-dev-y" 1.6 "filter" "auto")
    (clay-drawable-equalize map-layer TRUE)
    (clay-image-crop work-image width height width height)

    ; Create the frame layers
    (let loop ((remaining-frames num-frames))
      (unless (zero? remaining-frames)
        (let ((frame-layer (car (clay-layer-new-from-drawable drawable work-image))))
          (clay-image-insert-layer work-image frame-layer 0 0)
          (clay-item-set-name frame-layer
                              (string-append "Frame "
                                             (number->string (+ 1 (- num-frames
                                                                     remaining-frames)))
                                             " (replace)"))
          (let* ((abyss "black")
                 (filter (car (clay-drawable-filter-new frame-layer "gegl:displace" ""))))

            (if (= edge-type 0) (set! abyss "loop"))
            (if (= edge-type 1) (set! abyss "clamp"))

            (clay-drawable-filter-configure filter LAYER-MODE-REPLACE 1.0
                                            "amount-x" displacement "amount-x" displacement "abyss-policy" abyss
                                            "sampler-type" "cubic" "displace-mode" "cartesian")
            (clay-drawable-filter-set-aux-input filter "aux" map-layer)
            (clay-drawable-filter-set-aux-input filter "aux2" map-layer)
            (clay-drawable-merge-filter frame-layer filter)
          )
          (clay-item-set-visible frame-layer TRUE))
        (clay-drawable-offset map-layer
                              TRUE
                              OFFSET-COLOR
                              (car (clay-context-get-background))
                              (/ width num-frames)
                              (/ height num-frames))
        (loop (- remaining-frames 1))))

    (clay-image-remove-layer work-image map-layer)
    (clay-image-undo-enable work-image)
    (clay-display-new work-image)

    (clay-context-pop)))

(script-fu-register-filter "script-fu-ripply-anim"
  _"_Rippling..."
  _"Create a multi-layer image by adding a ripple effect to the current layer"
  "Adam D. Moss (adam@foxbox.org), Saul Goode"
  "Adam D. Moss, Saul Goode"
  "1997, 2015"
  "RGB* GRAY*"
  SF-ONE-DRAWABLE
  SF-ADJUSTMENT _"Ri_ppling strength"  '(3 0 256 1 10 1 0)
  SF-ADJUSTMENT _"_Number of frames"   '(15 0 256 1 10 0 1)
  SF-OPTION     _"Edge _behavior"      '(_"Wrap" _"Smear" _"Black")
  )

(script-fu-menu-register "script-fu-ripply-anim"
                         "<Image>/Filters/Animation/")
