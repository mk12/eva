;;; Copyright 2016 Mitchell Kember. Subject to the MIT License.

;;;;; R5RS standard procedures

;;; Equivalence predicates

(define (eqv? x y)
  (or (eq? x y)
      (and (number? x) (number? y) (= x y))
      (and (char? x) (char? y) (char=? x y))
      (and (string? x) (string? y) (string=? x y))))

(define (equal? x y)
  (if (and (pair? x) (pair? y))
    (and (equal? (car x) (car y))
         (equal? (cdr x) (cdr y)))
    (eq? x y)))

;;; Numerical operations

(define integer? number?)
(define (exact? x) #t)
(define (exact? x) #f)

(define (zero? x) (= x 0))
(define (positive? x) (> x 0))
(define (negative? x) (< x 0))
(define (even? x) (= (remainder x 2) 0))
(define (odd? x) (= (remainder x 2) 1))

(define (min x . xs)
  (cond ((null? xs) x)
        ((< x (car xs)) (apply min x (cdr xs)))
        (else (min (cdr xs)))))

(define (max x . xs)
  (cond ((null? xs) x)
        ((> x (car xs)) (apply max x (cdr xs)))
        (else (max (cdr xs)))))

(define (abs x) (if (negative? x) (- x) x))
(define quotient /)

(define (gcd x y)
  (if (zero? y)
    x
    (gcd y (remainder x y))))

(define (lcm x y)
  (/ (* x y) (gcd x y)))

;;; Lists and pairs

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (list? x) (or (null? x) (pair? x)))
(define (list . xs) xs)

(define (length xs)
  (if (null? xs)
    0
    (+ 1 (length (cdr xs)))))

(define (append . lists)
  ())

(define (reverse xs)
  (define (go fwd bwd)
    (if (null? fwd)
      bwd
      (go (cdr fwd) (cons (car fwd) bwd))))
  (go xs ()))

(define (list-tail xs k)
  (if (zero? k)
    xs
    (list-tail (cdr xs) (- k 1))))

(define (list-ref xs k)
  (if (zero? k)
    (car xs)
    (list-ref (cdr xs) (- k 1))))

(define (memq obj xs)
  (cond ((null? xs) #f)
        ((eq? (car xs) obj) xs)
        (else (memq obj (cdr xs)))))

(define (memv obj xs)
  (cond ((null? xs) #f)
        ((eqv? (car xs) obj) xs)
        (else (memq obj (cdr xs)))))

(define (member obj xs)
  (cond ((null? xs) #f)
        ((equal? (car xs) obj) xs)
        (else (memq obj (cdr xs)))))

(define (assq obj xs)
  (cond ((null? xs) #f)
        ((eq? (caar xs) obj) (car xs))
        (else (assq obj (cdr xs)))))

(define (assv obj xs)
  (cond ((null? xs) #f)
        ((eqv? (caar xs) obj) (car xs))
        (else (assv obj (cdr xs)))))

(define (assoc obj xs)
  (cond ((null? xs) #f)
        ((equal? (caar xs) obj) (car xs))
        (else (assoc obj (cdr xs)))))

;;; Characters

(define (char-alphabetic? c)
  (or (and (char>=? c #\a) (char<=? c #\z))
      (and (char>=? c #\A) (char<=? c #\Z))))

(define (char-numeric? c)
  (and (char>=? c #\0) (char<=? c #\9)))

(define (char-whitespace? c)
  (or (char=? c #\space)
      (char=? c #\newline)
      (char=? c #\return)
      (char=? c #\tab)))

(define (char-lower-case? c)
  (and (char>=? c #\a) (char<=? c #\z)))

(define (char-upper-case? c)
  (and (char>=? c #\A) (char<=? c #\Z)))

(define (char-downcase c)
  (if (char-upper-case? c)
    (integer->char (+ 32 (char->integer c)))
    c))

(define (char-upcase c)
  (if (char-lower-case? c)
    (integer->char (- 32 (char->integer c)))
    c))

;;; Strings

(define (string->list s)
  (define (go chars i)
    (if (zero? i)
      chars
      (go (cons (string-ref s (- i 1)) chars) (- i 1))))
  (go () (string-length s)))

(define (list->string chars)
  (define result (make-string (length chars) #\0))
  (define (go chars i)
    (if (null? chars)
      result
      (begin
        (string-set! result i (car chars))
        (go (cdr chars) (+ i 1)))))
  (go chars 0))

;;; Control features

(define (map f first . rest)
  (define (map1 g xs)
    (if (null? xs)
      ()
      (cons (g (car xs))
            (map1 g (cdr xs)))))
  (define (go lists)
    (if (apply or (map1 null? lists))
      ()
      (cons (apply f (map1 car lists))
            (go (map1 cdr lists)))))
  (if (null? rest)
    (map1 f first)
    (go (cons first rest))))

(define delay
  (macro
    (lambda (x)
      `(,let ((cache ())
              (evaluated #f))
             (,lambda ()
                      (,if evaluated
                           cache
                           (,begin
                             (,set! cache ,x)
                             (,set! evaluated #t)
                             cache)))))))

(define (force promise) (promise))

;;;;; Other procedures

;;; Macros

(define defmacro
  (macro
    (lambda (name-params . body)
      `(,define ,(car name-params)
                (,macro
                  (,lambda ,(cdr name-params) ,@body))))))

(defmacro (when condition . then)
  (list if condition
        (cons begin then)
        (begin)))

;;; Functional

(define (compose f g)
  (lambda args
    (f (apply g args))))

(define (filter pred xs)
  (cond ((null? xs) ())
        ((pred (car xs))
         (cons (car xs)
               (filter pred (cdr xs))))))
