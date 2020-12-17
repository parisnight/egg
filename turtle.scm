;;;;; A. V. Hershey font   2020.12.10

(load "fbgraph.scm")
(use-modules (fbgraph fbgraph)
	     (system foreign)
	     (rnrs bytevectors))

(define cursx 100)
(define cursy 100)
(define i 0)
(define final 0)
(define lines (make-s16vector 600))
(define oldcoord 0)

(define (append-coord i x)
  (if (even? i)
      (set! x (+ x cursx))
      (set! x (+ x cursy)))
  (s16vector-set! lines i x))

(define (coord c)
  (- (char->integer c) (char->integer #\R)))

(define (hershey-encoded-out str)
  ;;(string->number (string-trim (substring mys 0 5)))
  ;;(string->number (string-trim (substring mys 5 8)))
  (set! cursx (- cursx (coord (string-ref str 8))))
  (set! final (coord (string-ref str 9)))
  (set! i 0)
  (set! oldcoord 0)
  (string-for-each
   (lambda (c)
     (set! c (- (char->integer c) (char->integer #\R)))
     ;;(display c) (display " ")
     (if (and (= -50 oldcoord) (= 0 c))
	 (begin
	   (fblines (bytevector->pointer lines) (quotient i 2))
	   ;;(display lines)
	   ;;(display (quotient i 2))
	   (set! i -1))  ;; get rid of the appended -50
	 (append-coord i c))
     (set! oldcoord c)
     (set! i (1+ i)))
   (substring str 10))
  (fblines (bytevector->pointer lines) (quotient i 2))
  (set! cursx (+ final cursx))
  ;;(display lines)
  )

(define (gr)
  (fbopen)
  (bytevector-u32-native-set! color 0 #xffff00)
  (fblines (bytevector->pointer (s16vector 4 4 4 40 40 40 40 4 4 4)) 4)
  ;;  (fbclose)
  )

(define rowmans
 "  699  1JZ
  714  9MWRFRT RRYQZR[SZRY
  717  6JZNFNM RVFVM
  733 12H]SBLb RYBRb RLOZO RKUYU
 2246 24F^IUISJPLONOPPTSVTXTZS[Q RISJQLPNPPQTTVUXUZT[Q[O
  718 14KYQFOGNINKOMQNSNUMVKVIUGSFQF" )

(define typeface (string-split rowmans #\newline))
(define mychar (list-ref typeface 3))
;;(gr)
;;(hershey-encoded-out mychar)
(define (hprint s)
  (string-for-each
   (lambda (c)
     (hershey-encoded-out (list-ref typeface (- (char->integer c) 30))))
   s))

;;(hprint "Merry-Christmas")

(define a 0)
(define oldx 100)
(define oldy 100)

(define (moveto x y)
  (fblines (bytevector->pointer lines) (quotient i 2))
  (set! oldx x)
  (set! oldy y)
  (set! i 0))

(define (render)
  (fblines (bytevector->pointer lines) (quotient i 2))
  (set! i 0))

(define (drawto x y)
  (append-coord i x)
  (set! i (1+ i))
  (set! oldx x)
  (append-coord i y)
  (set! i (1+ i))
  (set! oldy y))

(define (rdrawto x y)
  (set! x (+ oldx x))
  (set! y (+ oldy y))
  (append-coord i x)
  (set! i (1+ i))
  (set! oldx x)
  (append-coord i y)
  (set! i (1+ i))
  (set! oldy y))

(define (tortoise-turn angle)
  (set! a (+ a (/ angle 57.3))))

(define (tortoise-move d)
  (let ((sa (sin a))
	(ca (cos a)))
    (rdrawto
     (inexact->exact (round (+ (* d ca) (- 0 (* d sa)))))
     (inexact->exact (round (+ (* d sa) (* d ca)))))))

(define (draw-polygon! circumference vertices)
  (let ((side (/ circumference vertices))
	(angle (/ 360 vertices)))
    (let iterate ((i 1))
      (if (<= i (1+ vertices))
	  (begin
	    (tortoise-move side)
	    (tortoise-turn angle)
	    (iterate (1+ i)))))))
     
;; (draw-polygon! 86 6)
;; (render)
     
;; (tortoise-penup)
;; (tortoise-move 1)
;; (tortoise-turn 30)
;; (tortoise-pendown)
;; (draw-polygon! 12 3)
     
;; (tortoise-penup)
;; (tortoise-move -2)
;; (tortoise-turn -100)
;; (tortoise-pendown)
;; (draw-polygon! 10 64)

(define (koch-line length depth)
  (if (zero? depth)
      (tortoise-move length)
      (let ((sub-length (/ length 3))
	    (sub-depth (1- depth)))
	(for-each (lambda (angle)
		    (koch-line sub-length sub-depth)
		    (tortoise-turn angle))
		  '(60 -120 60 0)))))
     
(define (snowflake length depth sign)
  (let iterate ((i 1))
    (if (<= i 3)
	(begin
	  (koch-line length depth)
	  (tortoise-turn (* sign -120))
	  (iterate (1+ i))))))
     
;; (snowflake 80 3 1)
;; (tortoise-turn 180)
;; (render)
;; (snowflake 80 3 -1)
;; (render)
