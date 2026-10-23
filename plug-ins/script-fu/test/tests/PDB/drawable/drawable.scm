; test methods of drawable


; set/get tested elsewhere
; kernel operations on drawable as set of pixels tested elsewhere

; setup
(define testImage (testing:load-test-image "clay-logo.png"))
; Wilber has one layer
(define testDrawable (vector-ref (car (clay-image-get-layers testImage)) 0))



; get bounding box of intersection of selection mask with drawable
; bounding box in x, y, width, height format
(assert `(clay-drawable-mask-bounds ,testDrawable))
; bounding box in upper left x,y lower right x,y
(assert `(clay-drawable-mask-intersect ,testDrawable))



(assert `(clay-drawable-free-shadow ,testDrawable))
(assert `(clay-drawable-merge-filters ,testDrawable))

; update a region (AKA invalidate)
; forces redraw of any display
(assert `(clay-drawable-update
            ,testDrawable
            -1 -1 ; origin of region
            2147483648 2147483648 ; width height of region
          ))

; FIXME: throws CRITICAL and sometimes crashes
;(assert `(clay-drawable-merge-shadow
;            ,testDrawable
;            1 ; push merge to undo stack
;          ))

; TODO document that signature changed in v3
(assert `(clay-drawable-offset
            ,testDrawable
            1 ; wrap around or fill
            OFFSET-WRAP-AROUND ; OffsetType
            "white"            ; color to fill background
            -2147483648 -2147483648 ; x, y
            ))



; thumbnails
; Since 3.0 rc2 these are private to  libclay
;(assert `(clay-drawable-thumbnail
;            ,testDrawable
;            1 1 ; thumbnail width height
;          ))

;(assert `(clay-drawable-sub-thumbnail
;            ,testDrawable
;            1 1 ; origin
;            2 2 ; width, height
;            2 2 ; thumbnail width height
;          ))


;clay-drawable-extract-component is tested drawable-ops

;clay-layer-new-from-drawable is tested w layer
