;; manipulations of scheme byte vectors 2020.11

(define bv #vu8(0 1 2 3))

(array? bv)
⇒ #t

(array-rank bv)
⇒ 1

(array-ref bv 2)
⇒ 2

;; Note the different argument order on array-set!.
(array-set! bv 77 2)
(array-ref bv 2)
⇒ 77

(array-type bv)
⇒ vu8

(parse-c-struct (make-c-struct (list int64 uint8) (list 300 43))
                (list int64 uint8))


(define-module (math bessel)
  #:use-module (system foreign)
  #:export (j0))

(define libm (dynamic-link "libm"))

(define j0
  (pointer->procedure double
                      (dynamic-func "j0" libm)
                      (list double)))

(define memcpy
  (let ((this (dynamic-link)))
    (pointer->procedure '*
                        (dynamic-func "memcpy" this)
                        (list '* '* size_t))))


(use-modules (rnrs bytevectors))

(define src-bits
  (u8-list->bytevector '(0 1 2 3 4 5 6 7)))
(define src
  (bytevector->pointer src-bits))
(define dest
  (bytevector->pointer (make-bytevector 16 0)))

(memcpy dest src (bytevector-length src-bits))

(bytevector->u8-list (pointer->bytevector dest 16))
⇒ (0 1 2 3 4 5 6 7 0 0 0 0 0 0 0 0)

;; struct timeval {
;;      time_t      tv_sec;     /* seconds */
;;      suseconds_t tv_usec;    /* microseconds */
;; };
;; assuming fields are of type "long"

(define gettimeofday
  (let ((f (pointer->procedure
            int
            (dynamic-func "gettimeofday" (dynamic-link))
            (list '* '*)))
        (tv-type (list long long)))
    (lambda ()
      (let* ((timeval (make-c-struct tv-type (list 0 0)))
             (ret (f timeval %null-pointer)))
        (if (zero? ret)
            (apply values (parse-c-struct timeval tv-type))
            (error "gettimeofday returned an error" ret))))))

(gettimeofday)    
⇒ 1270587589
⇒ 499553


(define qsort!
  (let ((qsort (pointer->procedure void
                                   (dynamic-func "qsort"
                                                 (dynamic-link))
                                   (list '* size_t size_t '*))))
    (lambda (bv compare)
      ;; Sort bytevector BV in-place according to comparison
      ;; procedure COMPARE.
      (let ((ptr (procedure->pointer int
                                     (lambda (x y)
                                       ;; X and Y are pointers so,
                                       ;; for convenience, dereference
                                       ;; them before calling COMPARE.
                                       (compare (dereference-uint8* x)
                                                (dereference-uint8* y)))
                                     (list '* '*))))
        (qsort (bytevector->pointer bv)
               (bytevector-length bv) 1 ;; we're sorting bytes
               ptr)))))

(define (dereference-uint8* ptr)
  ;; Helper function: dereference the byte pointed to by PTR.
  (let ((b (pointer->bytevector ptr 1)))
    (bytevector-u8-ref b 0)))

(define bv
  ;; An unsorted array of bytes.
  (u8-list->bytevector '(7 1 127 3 5 4 77 2 9 0)))

;; Sort BV.
(qsort! bv (lambda (x y) (- x y)))

;; Let's see what the sorted array looks like:
(bytevector->u8-list bv)
⇒ (0 1 2 3 4 5 7 9 77 127)

