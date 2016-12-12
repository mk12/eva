(load "prelude")

(define a 42)
(define f (lambda (x) x))
(f a)

(define (g a b c)
  (+ a (* b c)))
(g 1 1 1)
(g 10 3 8)
