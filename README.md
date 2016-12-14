# Eva

Eva is a Scheme interpreter written in C.

## Build

Just run `make`.

## Usage

There are three ways to use the program `bin/eva`:

- `eva`: Start a new REPL session.
- `eva -e expression`: Evaluate expressions and print their results.
- `eva file1 file2 ...`: Execute one or more Scheme files.

In addition, you can pass the `-n` or `--no-prelude` flag to disable automatic loading of the [prelude](src/prelude.scm).

## Language

All Schemes are different. The Eva dialect is fairly minimal. It supports some cool things, like first-class macros, but it lacks other things I didn't feel like implementing, such as floating-point numbers and tail-call optimization.

### Data types

Eva has 9 types:

1. **Null**. There is only one null value, written `()`. Unlike in most Schemes, `()` does not need to be quoted.
2. **Symbol**. Symbols are implemented as interned strings. The quoted expression `'foo` evaluates to the symbol `foo`. (Another round of evaluation would look up a variable called "foo.")
3. **Number**. Numbers in Eva are signed integers. Their width is whatever Clang decides `long` is on your machine.
4. **Boolean**. There are two boolean constants: `#t` and `#f`. Everything in Eva is truthy (considered true in a boolean context) except for `#f`. 
5. **Character**. These are just single-byte ASCII characters. They are written like `#\A`, and then there are the special character `#\space`, `#\newline`, `#\return`, and `#\tab`.
6. **String**. A string of characters. Unlike symbols, these are not interned, and they are mutable. They are written with double quotes, like `"Hello, World!"`.
7. **Pair**. You can't have Lisp without pairs. These are your standard cons cells. For example, `(cons 1 2)` evaluates to the pair `(1 . 2)`, and `(cons 1 (cons 2 ()))` evaluates to `(1 2)`.
8. **Procedure**. Procedures are created by lambda abstractions. A procedure `f` can be called like `(f a b c)`.
9. **Macro**. Macros are just procedures that follow different evaluation rules. They allow the syntax of Eva to be extended.

There is also a void type for the result of operations with side effects such as `define` and `set!`.

### Evaluation

When Eva sees `(f a b c)`, it evaluates as follows:

1. Evaluate the operator, `f`.
	1. If it evaluated to a procedure:
		1. Evaluate the operands `a`, `b`, and `c`.
		2. Substitute them into the function body.
		3. Evaluate the resulting body.
	2. If it evaluated to a macro:
		1. Substitute the operands `a`, `b`, and `c` unevaluated into the macro body.
		2. Evaluate the resulting body to get the code.
		3. Evaluate the resulting code at the call site.

These rules make it possible for macros to be first-class values in Eva. Although this may seem like a strange feature, it actually results in a simpler implementation. There are no special cases: if you really want to, you can `(define d define)` to save 5 characters. You could then write `(d (define x y) (error "Use d"))`.

### Macros

In Eva, any function `f` can be turned into a macro simply by writing `(macro f)`. For this new object, `macro?` will return `#t` and `procedure?` will return false.

For example, consider the following function which converts infix arithmetic to prefix form:

```scheme
(define (infix->prefix code)
  (define operators
    '(+ - * / = < > <= >=))
  (if (not (pair? code))
    code
    (let ((c (map infix->prefix code)))
      (if (and (= (length c) 3)
               (memq (cadr c) operators))
        (list (cadr c) (car c) (car (cddr c)))
        c))))
        
(infix->prefix '(1 + ((4 * (5 - 1)) / 3)))
;; => (+ 1 (/ (* 4 (- 5 1)) 3))
```

Now, let's turn it into a macro:

```scheme
(define with-infix (macro infix->prefix))

(macro? with-infix)
;; => #t

(with-infix
  (let ((x (1 + ((4 * (5 - 1)) / 3))))
    (x + x)))
;; => 12
```

Another benefit of having first-class macros is that reducing a list with a macro like `and` or `or` works. First-class macros are very cool and powerful, but I'm sure they'd be a nightmare if anyone actually used them in a large project.

## Input/output

Eva has seven IO procedures worth mentioning:

1. `(load str)`: Loads an Eva file. If `str` is "prelude," then it loads the prelude. Otherwise, it tries to open a file.
2. `(error expr1 ...)`: Creates an error. This can be used anywhere. The arguments will be printed when the error is reported.
3. `(read)`: Reads an expression using the same parser as for code.
4. `(write expr)`: Writes an expression in a format that `read` would accept.
5. `(display expr)`: Displays an expression to standard output without a trailing newline. Strings are displayed without double quotes or escaped characters.
6. `(newline)`: Prints a newline to standard output.
7. `(print expr)`: Like `display`, except it adds a trailing newline and it recursively enters lists to print each item individually.

### R5RS conformity

Eva implements the following standard macros (also called special forms) from [R5RS][1]:

```
define set!
lambda begin
if cond and or
let let*
quote quasiquote unquote unquote-splicing
```

The quotation special forms can be used via the usual `'`, `` ` ``, and `,` syntax. Due to the way environments are implemented, there is no need for `letrec`. You can write mutually recursive definitions using `let` bindings.

Eva implements the following standard procedures from [R5RS][2]:

```
eq? eqv? equal?
number? integer?
+ - * / quotient remainder modulo
= < > <= >=
zero? positive? negative? even? odd?
min max abs gcd lcm expt
number->string string->number
boolean? not
pair? cons car cdr set-car! set-cdr!
caar cadr cdar cddr
null? list? list length append reverse
list-tail list-ref
memq memv member assq assv assoc
symbol? symbol->string string->symbol
char? char=? char<? char>? char<=? char>=?
char-alphabetic? char-numeric? char-whitespace?
char-lower-case? char-upper-case?
char->integer integer->char
char-upcase char-downcase
string? string make-string string-length string-ref string-set!
string=? string<? string>? string<=? string>=?
substring string-append string->list list->string
string-copy string-fill!
procedure? eval apply map for-each force delay
read write load
```

[1]: https://groups.csail.mit.edu/mac/ftpdir/scheme-reports/r5rs-html/r5rs_6.html
[2]: https://groups.csail.mit.edu/mac/ftpdir/scheme-reports/r5rs-html/r5rs_8.html

## Implementation

Eva is implemented in 15 parts:

1. `main.c`: Implements the main function. Handles command-line arguments.
2. `util.c`: Utilities for reading files, allocating memory, etc.
3. `repl.c`: Implements the REPL and a routine for executing files.
4. `parse.c`: Parser for the language.
5. `expr.c`: Defines the Expression struct and related functions.
6. `type.c`: Typechecking for applications of standard procedures and macros.
7. `eval.c`: Implements the core of the interpreter (eval and apply).
8. `proc.c`: Implementation functions for standard procedures.
9. `macro.c`: Implementation functions for standard macros.
10. `env.c`: Data structure for environment frames.
11. `intern.c`: Table for interning strings.
12. `list.c`: Helper functions for dealing with linked lists.
13. `set.c`: Set data structure for detecting duplicates.
14. `error.c`: Creating and printing error messages.
15. `prelude.c`: Auto-generated from `prelude.scm`, the prelude.

## License

© 2016 Mitchell Kember

Eva is available under the MIT License; see [LICENSE](LICENSE.md) for details.
