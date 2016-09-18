(define fib
  (lambda (n)
	(if (<= n 1)
	  n
	  (+ (fib (- n 1)) (fib (- n 2))))))

(write (fib 1))
(write (fib 5))
(write (fib 10))
(write (fib 15))
