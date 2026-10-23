; Test clay-file-<foo> PDB procedures
;
; clay-file-load is tested elsewhere
; (clay-file-load RUN-NONINTERACTIVE (path-to-test-images "clay-logo.png")))

; clay-image-thumbnail is private to libclay

; clay-<foo>-export is tested elsewhere


(script-fu-use-v3)


; setup

; This also tests clay-file-load
(define testImage (testing:load-test-image-basic-v3))

(assert `(clay-image-id-is-valid ,testImage))



(test! "get temp file name")

; this is a "file" method but name is not clay-file- ?

(define tempFilename (clay-temp-file "xyz"))

; The named file does not exist.
(assert `(not (file-exists? ,tempFilename)))

; The name is unique among repeated calls, but we don't test that.






(test! "load thumbnail of file")

; It is not clear whether this loads the file and creates a thumbnail,
; or finds a thumbnail stored separately, for the desktop.

; We don't do this, because the framework doesn't handle quoted lists.
; FIXME This fails with illegal function, but works in the SF Console
; (define (testThumbnail) (clay-file-load-thumbnail (path-to-test-images "clay-logo.png")))
; (assert `(list? ,testThumbnail))

; testThumbnail is list
(assert `(list? (clay-file-load-thumbnail (,path-to-test-images "clay-logo.png"))))

; first element is width, 128
; third element is 50k vector of bytes, RGB data




(test! "load layer from file")

; the image in the file becomes one layer in the image

; !!! This is rare: a PDB internal function that takes a run mode
(define testLayer (clay-file-load-layer
                     RUN-NONINTERACTIVE
                     testImage
                     (path-to-test-images "clay-logo.png")))

; testLayer is-a layer
(assert `(clay-item-id-is-layer ,testLayer))

; it is not insert into any image yet



(test! "load layers from file")

; All the layers in the image in the file
; become multiple layers in the image.

; Note testImage only has one layer.

(define testLayers (clay-file-load-layers
                     RUN-NONINTERACTIVE
                     testImage
                     (path-to-test-images "clay-logo.png")))

; testLayers is-a vector of one
(assert `(vector? ,testLayers))

; the one layer in the vector is-a layer
(assert `(clay-item-id-is-layer
            (vector-ref ,testLayers 0)))

; it is not insert into any image yet



(test! "create thumbnail for existing file")

; FIXME this fails for unknown reasons

; This file already has a thumbnail.
;;(assert `(clay-file-create-thumbnail
;;            ,testImage
;;            (path-to-test-images "clay-logo.png")))



(test! "create thumbnail for newly saved image")

; Thumbnails don't exist for newly created files unless you create them


(define testNewImage (clay-image-new 21 22 RGB))

(assert `(clay-file-save
            RUN-NONINTERACTIVE
            ,testNewImage
            "/tmp/testSaveNewImage.xcf"
            -1 )) ; NULL export options

; The file does not have a thumbnail yet???

; FIXME shouldn't this call succeed instead of error?
(assert-error `(clay-file-create-thumbnail
                  ,testNewImage
                  "/tmp/testSaveNewImage.xcf")
              "Procedure execution of clay-file-create-thumbnail failed")



(test! "file save")

; The image is not dirty, but saving under a different suffix
; does write the file.

(assert `(clay-file-save
            RUN-NONINTERACTIVE
            ,testImage
            "/tmp/testSaveImage.xcf"
            -1 )) ; NULL export options

; The file exists
(assert `(file-exists? "/tmp/testSaveImage.xcf"))


; TODO test clay-file-save on a non-dirty image
; does or does not write the file?


; Is error to omit a suffix indicating image format
(assert-error
     `(clay-file-save
            RUN-NONINTERACTIVE
            ,testImage
            "/tmp/testSaveImage"
            -1 ) ; NULL export options
      "Procedure execution of clay-file-save failed: Unknown file type")



; cleanup
; Delete image but not the created file.
(clay-image-delete testImage)




(script-fu-use-v2)