;;;;; xwin.scm
;;;;; gcc -lX11 -shared -o xwin.so -fPIC xwin.c

(define-module (xwin xwin)
  #:use-module (system foreign)
  #:export (xwin xopen xsetline xline xlines xprint xflush demo))

(define xwin (dynamic-link "xwin.so"))

(define xopen
  (pointer->procedure void
                      (dynamic-func "xopen" xwin)
                      (list int int int int)))

(define xsetline
  (pointer->procedure void
                      (dynamic-func "xsetline" xwin)
                      (list int int)))

(define xline
  (pointer->procedure void
                      (dynamic-func "xline" xwin)
                      (list int int int int)))
(define xlines
  (pointer->procedure void
                      (dynamic-func "xlines" xwin)
                      (list '* int)))

(define xprint
  (pointer->procedure void
                      (dynamic-func "xprint" xwin)
                      (list int int '* int)))

(define xflush
  (pointer->procedure void
                      (dynamic-func "xflush" xwin)
                      (list)))

(use-modules (rnrs bytevectors))

(define (demo)
  (xopen 0 0 300 200)
  (xprint 30 60 (string->pointer "hello") 3)
  (xsetline 2 4)
  (xlines (bytevector->pointer (s16vector 4 4 4 40 40 40 40 4 4 4)) 5)
  (xline 0 0 150 50)
  (xflush)
  )

