(load "prelude")

(define s "Hello, World!")
s
(define ss (string-copy s))
(string=? s s)
(string=? s ss)

(string-set! s 0 #\Y)
(string-set! s 5 #\w)
s
ss
(string=? s s)
(string=? s ss)

(string->list s)
(list->string '(#\a #\space #\b))
