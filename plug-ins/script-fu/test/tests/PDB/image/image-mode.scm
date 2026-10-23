; test combinations of mode conversion
; COLOR=>GRAY=>INDEXED=>GRAY


; Since 3.0rc3, getters of color profile are private to libclay


; setup
(define testImage (testing:load-test-image "clay-logo.png"))

; test image is RGB
(assert `(= (car (clay-image-get-base-type ,testImage))
            RGB))


; convert RGB=>GRAY
(assert `(clay-image-convert-grayscale ,testImage))
; mode conversion was effective
(assert `(= (car (clay-image-get-base-type ,testImage))
                 GRAY))
; effective color profile is the built-in one for GRAY
;(assert `(= (vector-length (car (clay-image-get-effective-color-profile ,testImage)))
;            544))

; convert GRAY=>INDEXED
(assert `(clay-image-convert-indexed
            ,testImage
            CONVERT-DITHER-NONE
            CONVERT-PALETTE-GENERATE
            25  ; color count
            1  ; alpha-dither.  FUTURE: #t
            1  ; remove-unused. FUTURE: #t
            "myPalette" ; ignored
            ))
; mode conversion was effective
(assert `(= (car (clay-image-get-base-type ,testImage))
                 INDEXED))
; effective color profile is the built-in one for COLOR
;(assert `(= (vector-length (car (clay-image-get-effective-color-profile ,testImage)))
;            672))

; convert INDEXED=>GRAY
(assert `(clay-image-convert-grayscale ,testImage))
; mode conversion was effective
(assert `(= (car (clay-image-get-base-type ,testImage))
                 GRAY))
; effective color profile is the built-in one for GRAY
;(assert `(= (vector-length (car (clay-image-get-effective-color-profile ,testImage)))
;            544))

; convert GRAY=>COLOR
(assert `(clay-image-convert-rgb ,testImage))
; mode conversion was effective
(assert `(= (car (clay-image-get-base-type ,testImage))
                 RGB))
; effective color profile is the built-in one for RGB
;(assert `(= (vector-length (car (clay-image-get-effective-color-profile ,testImage)))
;            672))



