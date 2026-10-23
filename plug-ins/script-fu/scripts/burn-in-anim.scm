;
;  burn-in-anim.scm V2.1  -  script-fu for Clay 1.1 and higher
;
;  Copyright (C) 9/2000  Roland Berger
;  roland@fuchur.leute.server.de
;  http://fuchur.leute.server.de
;
;  Let text appear and fade out with a "burn-in" like SFX.
;  Works on an image with a text and a background layer
;
;  Copying Policy:  GNU Public License http://www.gnu.org
;

(define (script-fu-burn-in-anim org-img
                                org-layers
                                glow-color
                                fadeout
                                bl-width
                                corona-width
                                after-glow
                                show-glow
                                optimize
                                speed)

  (let* (
        ;--- main variable: "bl-x" runs from 0 to layer-width
        (bl-x 0)
        (frame-nr 0)
        (img 0)
        (source-layer 0)
        (bg-source-layer 0)
        (source-layer-width 0)
        (bg-layer 0)
        (bg-layer-name 0)
        (bl-layer 0)
        (bl-layer-name 0)
        (bl-mask 0)
        (bl-layer-width 0)
        (bl-height 0)
        (bl-x-off 0)
        (bl-y-off 0)
        (nofadeout-bl-x-off 0)
        (nofadeout-bl-width 0)
        (blended-layer 0)
        (img-display 0)
        )

    (if (< speed 1)
        (set! speed (* -1 speed)) )

    ;--- check image and work on a copy
    (if (and (= (vector-length (car (clay-image-get-layers org-img))) 2)
             (= (car (clay-image-get-floating-sel org-img)) -1))

        ;--- main program structure starts here, begin of "if-1"
        (begin
          (clay-context-push)
          (clay-context-set-defaults)

          (set! img (car (clay-image-duplicate org-img)))
          (clay-image-undo-disable img)
          (if (> (car (clay-drawable-type (vector-ref org-layers 0))) 1 )
              (clay-image-convert-rgb img))
          (set! source-layer    (vector-ref (car (clay-image-get-layers img)) 0 ))
          (set! bg-source-layer (vector-ref (car (clay-image-get-layers img)) 1 ))
          (set! source-layer-width (car (clay-drawable-get-width  source-layer)))

          ;--- hide layers, cause we want to "merge visible layers" later
          (clay-item-set-visible source-layer FALSE)
          (clay-item-set-visible bg-source-layer     FALSE)

          ;--- process image horizontal with pixel-speed
          (while (< bl-x (+ source-layer-width bl-width))
              (set! bl-layer (car (clay-layer-copy source-layer)))
              (set! bl-layer-name (string-append "fr-nr"
                                                 (number->string frame-nr 10) ) )

              (clay-layer-add-alpha bl-layer)
              (clay-image-insert-layer img bl-layer 0 -2)
              (clay-item-set-name bl-layer bl-layer-name)
              (clay-item-set-visible bl-layer TRUE)
              (clay-layer-set-lock-alpha bl-layer TRUE)
              (clay-layer-add-alpha bl-layer)

              ;--- add an alpha mask for blending and select it
              (clay-image-select-item img CHANNEL-OP-REPLACE bl-layer)
              (set! bl-mask (car (clay-layer-create-mask bl-layer ADD-MASK-BLACK)))
              (clay-layer-add-mask bl-layer bl-mask)

              ;--- handle layer geometry
              (set! bl-layer-width source-layer-width)
              (set! bl-height      (car (clay-drawable-get-height bl-layer)))
              (set! bl-x-off (- bl-x     bl-width))
              (set! bl-x-off (+ bl-x-off (car  (clay-drawable-get-offsets bl-layer))))
              (set! bl-y-off             (cadr (clay-drawable-get-offsets bl-layer)))

              ;--- select a rectangular area to blend
              (clay-image-select-rectangle img CHANNEL-OP-REPLACE bl-x-off bl-y-off bl-width bl-height)
              ;--- select at least 1 pixel!
              (clay-image-select-rectangle img CHANNEL-OP-ADD bl-x-off bl-y-off (+ bl-width 1) bl-height)

              (if (= fadeout FALSE)
                  (begin
                    (set! nofadeout-bl-x-off (car (clay-drawable-get-offsets bl-layer)))
                    (set! nofadeout-bl-width (+ nofadeout-bl-x-off bl-x))
                    (set! nofadeout-bl-width (max nofadeout-bl-width 1))
                    (clay-image-select-rectangle img CHANNEL-OP-REPLACE
                                                 nofadeout-bl-x-off bl-y-off
                                                 nofadeout-bl-width bl-height)
                  )
              )

              ;--- alpha blending text to trans (fadeout)
              (clay-context-set-foreground '(255 255 255))
              (clay-context-set-background '(  0   0   0))
              (if (= fadeout TRUE)
                  (begin
                    ; blend with 20% offset to get less transparency in the front
		    (clay-context-set-gradient-fg-bg-rgb)
                    (clay-drawable-edit-gradient-fill bl-mask
						      GRADIENT-LINEAR 20
						      FALSE 1 0
						      TRUE
						      (+ bl-x-off bl-width) 0
						      bl-x-off 0)
                  )
              )

              (if (= fadeout FALSE)
                  (begin
                    (clay-context-set-foreground '(255 255 255))
                    (clay-drawable-edit-fill bl-mask FILL-FOREGROUND)
                  )
              )

              (clay-layer-remove-mask bl-layer MASK-APPLY)

              ;--- add bright glow in front
              (if (= show-glow TRUE)
                  (begin
                    ;--- add some brightness to whole text
                    (if (= fadeout TRUE)
                        (clay-drawable-merge-new-filter bl-layer "clay:brightness-contrast" 0 LAYER-MODE-REPLACE 1.0
                            "brightness" 0.787
                            "contrast"   0)
                    )

		    ;--- blend glow color inside the letters
		    (clay-context-set-foreground glow-color)
		    (clay-context-set-gradient-fg-transparent)
		    (clay-drawable-edit-gradient-fill bl-layer
						      GRADIENT-LINEAR 0
						      FALSE 1 0
						      TRUE
						      (+ bl-x-off bl-width) 0
						      (- (+ bl-x-off bl-width) after-glow) 0)

                    ;--- add corona effect
		    (clay-image-select-item img CHANNEL-OP-REPLACE bl-layer)
		    (clay-selection-sharpen img)
		    (clay-selection-grow img corona-width)
		    (clay-layer-set-lock-alpha bl-layer FALSE)
		    (clay-selection-feather img corona-width)
		    (clay-context-set-foreground glow-color)
		    (clay-drawable-edit-gradient-fill bl-layer
						      GRADIENT-LINEAR 0
						      FALSE 1 0
						      TRUE
						      (- (+ bl-x-off bl-width) corona-width) 0
						      (- (+ bl-x-off bl-width) after-glow) 0)
		    )
		  )

              ;--- merge with bg layer
              (set! bg-layer (car (clay-layer-copy bg-source-layer)))
              (clay-image-insert-layer img bg-layer 0 -1)
              (clay-image-lower-item img bg-layer)
              (set! bg-layer-name (string-append "bg-"
                                                 (number->string frame-nr 10)))
              (clay-item-set-name bg-layer bg-layer-name)
              (clay-item-set-visible bg-layer TRUE)
              (set! blended-layer (car (clay-image-merge-visible-layers img
                                        CLIP-TO-IMAGE)))
              ;(set! blended-layer bl-layer)
              (clay-item-set-visible blended-layer FALSE)

              ;--- end of "while" loop
              (set! frame-nr (+ frame-nr 1))
              (set! bl-x     (+ bl-x speed))
          )

          ;--- finalize the job
          (clay-selection-none img)
          (clay-image-remove-layer img    source-layer)
          (clay-image-remove-layer img bg-source-layer)

          (clay-image-set-file img "burn-in.xcf")

          (if (= optimize TRUE)
              (begin
                (clay-image-convert-indexed img CONVERT-DITHER-FS CONVERT-PALETTE-WEB 250 FALSE TRUE "")
                (set! img (car (plug-in-animationoptimize #:run-mode  RUN-NONINTERACTIVE
                                                          #:image     img
                                                          #:drawables (vector blended-layer))))
              )
          )

          (clay-item-set-visible (vector-ref (car (clay-image-get-layers img)) 0)
                                  TRUE)
          (clay-image-undo-enable img)
          (clay-image-clean-all img)
          (set! img-display (car (clay-display-new img)))

          (clay-displays-flush)

          (clay-context-pop)
        )

        ;--- false form of "if-1"
        (clay-message _"The Burn-In script needs two layers in total. A foreground layer with transparency and a background layer.")
    )
  )
)


(script-fu-register-filter "script-fu-burn-in-anim"
    _"B_urn-In..."
    _"Create intermediate layers to produce an animated 'burn-in' transition between two layers"
    "Roland Berger roland@fuchur.leute.server.de"
    "Roland Berger"
    "January 2001"
    "RGBA GRAYA INDEXEDA"
    SF-ONE-OR-MORE-DRAWABLE
    SF-COLOR        _"Glow color"           "white"
    SF-TOGGLE       _"Fadeout"              FALSE
    SF-ADJUSTMENT   _"Fadeout width"        '(100 1 3000 1 10 0 SF-SPINNER)
    SF-ADJUSTMENT   _"Corona width"         '(7 1 2342 1 10 0 SF-SPINNER)
    SF-ADJUSTMENT   _"After glow"           '(50 1 1024 1 10 0 SF-SPINNER)
    SF-TOGGLE       _"Add glowing"          TRUE
    SF-TOGGLE       _"Prepare for GIF"      FALSE
    SF-ADJUSTMENT   _"Speed (pixels/frame)" '(50 1 1024 1 10 0 SF-SPINNER)
)

(script-fu-menu-register "script-fu-burn-in-anim"
                         "<Image>/Filters/Animation/")
