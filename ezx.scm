;;;;; ezxdisp.scm          guile interface to ezxdisp 2020.11.5
;;;;; gcc -I../../include -lX11 -shared -o libezx.so -fPIC ezxdisp.c

(define-module (ezxdisp ezxdisp)
  #:use-module (system foreign)
  #:export (ezx-quit))

;(define ezxdisp (dynamic-link "/home/fuzz/Downloads/ezxdisp-0.1.4/src/x11/libezx.so"))

(define ezx-init
  (pointer->procedure '*
                      (dynamic-func "ezx_init" ezxdisp)
                      (list int int '*)))

(define ezx-quit
  (pointer->procedure void
                      (dynamic-func "ezx_quit" ezxdisp)
                      (list '*)))

(define ezx-set-background
  (pointer->procedure void
                      (dynamic-func "ezx_set_background" ezxdisp)
                      (list '* '*)))

(define ezx-redraw
  (pointer->procedure void
                      (dynamic-func "ezx_redraw" ezxdisp)
                      (list '*)))

(define ezx-line-2d
  (pointer->procedure void
                      (dynamic-func "ezx_line_2d" ezxdisp)
                      (list '* int int int int '* int)))

(define ezx-line
  (pointer->procedure void
                      (dynamic-func "ezx_line" ezxdisp)
                      (list '* int int int int '* int)))

(define ezx-lines-2d-lolevel
  (pointer->procedure void
                      (dynamic-func "ezx_lines_2d_lolevel" ezxdisp)
                      (list '* '* int '* int)))

(define (ezx-lines-2d ezx points color width)
  (ezx-lines-2d-lolevel ezx points (s32vector-length points) color width))

(define (ezx-poly-2d ezx points color)
  (ezx-poly-2d-lolevel ezx points (s32vector-length points) color))
  
(define (ezx-poly-3d ezx points hx hy hz color)
  (ezx-poly-3d-lolevel ezx points hx hy hz (s32vector-length points) color))

(use-modules (rnrs bytevectors))

(define (make-ezx-color r g b) (f64vector r g b) )  ; SRFI-4

;;(define ezx-next-event get-next-event)
;;(ezx-set-background ezx (make-ezx-color 1 1 1))
;;(ezx-lines-2d ezxp (scm->pointer points) (scm->pointer (make-ezx-color 1 0 0)) 4)
;;(ezx-lines-2d-lolevel ezxp (scm->pointer points) 10 (scm->pointer (make-ezx-color 1 0 0)) 4)
;;  (define str (bytevector->pointer (string->utf8 "hello\0")))
;;  (define points (s32vector 2 2 42 2 42 42 2 42 2 2))
;;  (bytevector->pointer (s32-list->bytevector '(2 2 42 2 42 42 2 42 2 2))))
;;  (bytevector->pointer (u8-list->bytevector '(255 0 255))))

(define ezxp (ezx-init 200 100 (string->pointer "goodbye")))

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
