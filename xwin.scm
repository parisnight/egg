;;;;; xwin.scm
;;;;; gcc -lX11 -shared -o xwin.so -fPIC xwin.c

(define-module (xwin xwin)
  #:use-module (system foreign)
  #:export (xopen))

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

(define xprint
  (pointer->procedure void
                      (dynamic-func "xprint" xwin)
                      (list int int '* int)))

(define xflush
  (pointer->procedure void
                      (dynamic-func "xflush" xwin)
                      (list)))

(define (demo)
  (xopen 0 0 300 200)
  (xprint 30 60 (string->pointer "hello") 3)
  (xsetline 2 4)
  (xline 0 0 150 50)
  (xflush)
  )

 (use-modules (rnrs bytevectors))
;; (f64vector 1.1 2 3)  ; SRFI-4
;; (f64vector->list (f64vector 1.1 2 3))
;; (define strp (bytevector->pointer (string->utf8 "hello\0")))
;; (utf8->string (pointer->bytevector strp 40))
;; (define points (s32vector 2 2 42 2 42 42 2 42 2 3))
;; (bytevector->pointer points)
;; (bytevector->pointer (u8-list->bytevector '(255 0 255)))

(utf8->string (u8-list->bytevector '(99 97 102 101)))
(define strp (string->pointer "gozila"))
(utf8->string (pointer->bytevector strp 4))
(use-modules (ice-9 iconv))
(bytevector->string (pointer->bytevector strp 40) "ISO-8859-1")



#|
;;;;(define ezxp (ezx-init 200 100 (string->pointer "goodbye")))

(define co (make-ezx-color 0 1 1))

(define mybv (make-bytevector 24))
(bytevector-ieee-double-native-set! mybv 0 1.0))
(define mybvp (bytevector->pointer mybv))

(ezx-line-2d ezxp 0 0 100 20 mybvp 2)
(ezx-line ezxp 0 0 100 40 mybvp 2)
(ezx-set-background ezxp (bytevector->pointer co))
(ezx-redraw ezxp)

(define (fill-bv bv l n)
  (match l
	 (() bv)
	 (((x y) . r)
	  (s32vector-set! bv n x)
	  (s32vector-set! bv (+ n 1) y)
	  (fill-bv bv r (+ 2 n)))))

     (fill-bv (make-s32vector (* count 2)) (list 1 2 3 4 5 6) 0)
 |# 
