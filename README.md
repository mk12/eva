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

These rules make it possible for macros to be first-class values in Eva. Although this may seem like a strange feature, it actually results in a simpler implementation. There are no special cases: if you really want to, you can `(define d define)` to save 5 characters. You could then `(d (define x y) (error "Use d"))` if you really wanted to.

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
procedure? eval apply map force delay
read write load
```

[1]: https://groups.csail.mit.edu/mac/ftpdir/scheme-reports/r5rs-html/r5rs_6.html
[2]: https://groups.csail.mit.edu/mac/ftpdir/scheme-reports/r5rs-html/r5rs_8.html

## License

© 2016 Mitchell Kember

Eva is available under the MIT License; see [LICENSE](LICENSE.md) for details.
