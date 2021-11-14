(define and
  (macro
    (lambda xs
      `())))

(define transform
  (lambda (f ls)
    (cond
      ((null? ls) ())
      ((pair? (car ls))
       (f (cons (transform f (car ls))
                (transform f (cdr ls)))))
      (else
        (f (cons (car ls)
                 (transform f (cdr ls))))))))

(define listify
  (macro
    (lambda (body)
      (transform
        (lambda (ls)
          (cons 'list ls))
        body))))

(define let
  (macro
    (lambda (bindings . body)
      (list (cons lambda
                  (cons (map car bindings)
                        body))
            (map cdr bindings)))))


(define let
  (macro
    (lambda (bindings . body)
      `((lambda ,(map car bindings)
          ,@body)
        ,@(map cdr bindings)))))

