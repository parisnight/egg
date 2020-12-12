;;;;; fbgraph.scm

(define-module (fbgraph fbgraph)
  #:use-module (system foreign)
  #:use-module (rnrs bytevectors)
  #:export (fbopen fblines fbclose demo color))

(define fbwin (dynamic-link "fbgraph.so"))
(define color (pointer->bytevector (dynamic-pointer "color" fbwin) 4))

(define fbopen
  (pointer->procedure void
                      (dynamic-func "fbopen" fbwin)
                      (list)))

(define fblines
  (pointer->procedure void
                      (dynamic-func "fblines" fbwin)
                      (list '* int)))

(define fbclose
  (pointer->procedure void
                      (dynamic-func "fbclose" fbwin)
                      (list)))

(define (demo)
  (fbopen)
  (bytevector-u32-native-set! color 0 #xffff00)
  ;;(xprint 30 60 (string->pointer "hello") 3)
  ;;(xsetline 2 4)
  (fblines (bytevector->pointer (s16vector 4 4 4 40 40 40 40 4 4 4)) 5)
  ;;(xline 0 0 150 50)
  (fbclose)
  )

;;(demo)
