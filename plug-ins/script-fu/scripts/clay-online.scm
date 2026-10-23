; xyz.kainzmayer.ClayDesign
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; clay-online.scm
; Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.

(define (clay-help-main)
  (clay-help "" "clay-main")
)

(define (clay-help-concepts-usage)
  (clay-help "" "clay-concepts-usage")
)

(define (clay-help-using-docks)
  (clay-help "" "clay-concepts-docks")
)

(define (clay-help-using-simpleobjects)
  (clay-help "" "clay-using-simpleobjects")
)

(define (clay-help-using-selections)
  (clay-help "" "clay-using-selections")
)

(define (clay-help-using-fileformats)
  (clay-help "" "clay-using-fileformats")
)

(define (clay-help-using-photography)
  (clay-help "" "clay-using-photography")
)

(define (clay-help-using-web)
  (clay-help "" "clay-using-web")
)

(define (clay-help-concepts-paths)
  (clay-help "" "clay-concepts-paths")
)


; shortcuts to help topics
(script-fu-register-procedure "clay-help-concepts-paths"
   _"_Using Paths"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-concepts-paths"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-web"
   _"_Preparing your Images for the Web"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-web"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-photography"
   _"_Working with Digital Camera Photos"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-photography"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-fileformats"
   _"Create, Open and Save _Files"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-fileformats"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-concepts-usage"
   _"_Basic Concepts"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-concepts-usage"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-docks"
   _"How to Use _Dialogs"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-docks"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-simpleobjects"
   _"Drawing _Simple Objects"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-simpleobjects"
			 "<Image>/Help/User Manual")


(script-fu-register-procedure "clay-help-using-selections"
   _"_Create and Use Selections"
   _"Bookmark to the user manual"
    "Roman Joost <romanofski@gimp.org>"
    "2006"
)

(script-fu-menu-register "clay-help-using-selections"
			 "<Image>/Help/User Manual")

(script-fu-register-procedure "clay-help-main"
   _"_Table of Contents"
   _"Bookmark to the user manual"
    "Alx Sa"
    "2023"
)

(script-fu-menu-register "clay-help-main"
			 "<Image>/Help/User Manual/[Table of Contents]")
