(define bessel-lib (dynamic-link "./libbessel.so"))
(dynamic-call "init_math_bessel" bessel-lib)
(j0 2)

;;(apropos "j0")

(use-modules (system foreign))
(define p (dynamic-pointer "v" bessel-lib))
(define h (dynamic-pointer "h" bessel-lib))

(use-modules (rnrs bytevectors))

;; (pointer->bytevector numptob (sizeof long)) 0 (native-endianness)(sizeof long))
;; (pointer->bytevector p (sizeof uint8) 12)

(define pbv (pointer->bytevector p 40))
(define hbv (pointer->bytevector h 40))

(bytevector->u8-list pbv)
(bytevector->u8-list hbv)
(utf8->string hbv)
(define lbv (string->utf8 "λ"))
(bytevector-copy! lbv 0 hbv 1 2)

(bytevector-u8-set! pbv 0 9)

;(parse-c-struct (make-c-struct (list int64 uint8)
;			       (list 300 43))
;		(list int64 uint8))
; (300 43)

(parse-c-struct p (list uint32 uint32))
(pointer->bytevector p 8 0 'u32)
;; google hangout microwank meet


;; (f64vector 1.1 2 3)  ; SRFI-4
;; (f64vector->list (f64vector 1.1 2 3))
;; (define strp (bytevector->pointer (string->utf8 "hello\0")))
;; (utf8->string (pointer->bytevector strp 40))
;; (bytevector->pointer (u8-list->bytevector '(255 0 255)))

(utf8->string (u8-list->bytevector '(99 97 102 101)))
(define strp (string->pointer "gozila"))
(utf8->string (pointer->bytevector strp 4))
(use-modules (ice-9 iconv))
(bytevector->string (pointer->bytevector strp 40) "ISO-8859-1")

#|
(define mybv (make-bytevector 24))
(bytevector-ieee-double-native-set! mybv 0 1.0)
(define mybvp (bytevector->pointer mybv))

(define (fill-bv bv l n)
  (match l
	 (() bv)
	 (((x y) . r)
	  (s32vector-set! bv n x)
	  (s32vector-set! bv (+ n 1) y)
	  (fill-bv bv r (+ 2 n)))))

     (fill-bv (make-s32vector (* count 2)) (list 1 2 3 4 5 6) 0)

(make-shared-array #2((a b c) (d e f) (g h i))
		   (lambda (i) (list i 2))
		   '(0 2))

(make-f32vector 3)
(define a #f32(0.0 2.0 3.14 21 22 23 31 32 33))
(define a #u8(11 12 13 21 22 23 31 32 33))

(define b 
  (make-shared-array a
		     (lambda (i j) (list (+ (* i 3) j)))
		     3 3))

(define b1
(make-shared-array b
		   (lambda (i) (list i 2))
		   3))


 |# 
