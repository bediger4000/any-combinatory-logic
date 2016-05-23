#acl: Generalized Combinatory Logic Interpreter

## version 1.2

[Bruce Ediger](mailto:bediger@stratigery.com)

# Table of contents

1.  [Introduction](#introduction)
2.  [Starting](#starting-the-interpreter)
3.  [Using](#using-the-interpreter)
4.  [Defining Primitives](#defining-primitives)
5.  [Expressing Bracket Abstraction Algorithms](#expressing-bracket-abstraction-algorithms)
6.  [Interpreter Commands](#interpreter-commands)
7.  [Examples](#examples)
8.  [Building and Installing](#building-and-installing)

# Introduction

This document describes how to build and use `acl` v1.2.

`acl` (*A*ny *C*ombinatory *L*ogic) interprets programming languages with
similarities to various "Combinatory Logic" (CL) formal systems. It doesn't
interpret any "Combinatory Logic" in that it runs on computers with finite CPU
speed and a finite memory. Most or all formal systems fail to take these limits
into account.

`acl` differs from other interpreters in that the user must [specify any
primitives](#defining-primitives) to the interpreter. It has no built-in atomic
primitive combinators.

`acl` does not have any built-in bracket abstraction algorithms. The user must
[specify a bracket abstraction algorithm](#expressing-bracket-abstraction-algorithms) before doing bracket
abstraction on a term. The user must specify primitives before using them in a
bracket abstraction rule definition.

Without specifying any primitives, the interpreter checks the syntax of
"applicative structures", where every atomic term constitutes a free variable.

#Starting the interpreter

After [building the interpreter's executable](#building-and-installing),
you can start it from the command line:

    $ ./acl
    ACL>

The interpreter uses `ACL>` as its prompt for user input. `acl` has a strict
grammar, so you must type in either a term for reduction, or an
[interpreter command](#interpreter-commands), or a command to [examine a term](#information-about-expressions).

A keyboard interrupt (almost always control-C) can interrupt whatever
long-running reduction currently takes place. A keyboard interrupt at the
`ACL>` prompt will cause the interpreter to exit.

You have to use keyboard end-of-file (usually control-D) to exit `acl`.

Giving the interpreter a CL term causes it to execute the usual functional
language interpreter's read-eval-print loop. The interpreter prints the input
in a minimally-parenthesized representation, reduces to normal form, and prints
a text representation of the normal form. It prints a prompt, and waits for
another user input.

`acl` does standard Combinatory Logic "normal order" evaluation. The leftmost
outermost contraction gets evaluated first.

## Command line options

    -c               enable reduction cycle detection
    -d               debug contractions
    -e               elaborate output
    -L <filename>    Interpret a file named <filename> before reading user input
    -N <number>      perform up to <number> contractions on each input expression.
    -p               Don't print any prompt.
    -s               single-step reductions
    -T <number>      evaluate an expression for up to <number> seconds
    -t               trace reductions

The `-e` or `-s` options have no use without the `-t` option, but `-t` alone might have some use.

`-L <filename>` can occur more than one time. `acl` will interpret the files in
the order they appear on the command line. After interpreting the last (or
only) file, it prints the `ACL>` prompt, then waits for interactive user input.
This command line flag pre-loads files. To interpret files during an
interactive session, use the [`load`](#load) command.

#Using the interpreter

## Interactive input

I designed `acl` for use as an interactive system, with a user typing CL
expressions at a prompt. The interpreter reduces the expression to a normal
form (if it has one), or hits some other limit, like a pre-set timeout, count
of allowed contractions or the user's patience.

After entering an entire expression, the user types "return" or "enter" to
trigger evaluation.

The built-in prompt for input is the string `ACL>`. It appears when the
interpreter starts up, or has finished reducing whatever expression the user
gave it last, or it has executed an interpreter command.

You have to type an end-of-file character (almost always control-D) to quit, as
it has no built-in "exit" or "quit" command.

A keyboard interrupt (almost always control-C) can interrupt whatever
long-running reduction currently takes place, returning the user to the `ACL>`
prompt. A keyboard interrupt at the `ACL>` prompt will cause the interpreter to
exit.

### Non-interactive input

The `-p` command line option causes the interpreter to not print a prompt. This
only has use for non-interactive input. The interpreter does read from stdin
and write to stdout. You can use it as a non-interactive "filter", with input
and output redirection.

###Grammar, briefly

Expressions consist of either a single term, or two (perhaps implicitly
parenthesized) terms. Terms consist of either a [user-defined
primitive](#defining-primitives) or a variable, or a parenthesized expression.
The [`reduce`](#reduce) command, and [bracket abstraction](#expressing-bracket-abstraction-algorithms) each
produce an expression.

Variables (which can also serve as [abbreviations](#defining-abbreviations) or names of
[user-defined primitives](#defining-primitives)) look like C or Java style
identifiers. An identifier consists of a letter, followed by zero or more
letters or underscores. Variables, abbreviations and user-defined primitives
share the same name space. You can't have an abbreviation with the same name as
a primitive.

The interpreter treats combinators and variables as left associative, the
standard in the Combinatory Logic literature. That means that an expression
like this: `I a b c d` ends up getting treated as though it had parentheses
like this: `((((I a) b) c) d)`

To apply one complex term to another, the user must parenthesize terms.
Applying `W (W K)` to `C W` would look like this: `(W (W K)) (C W)`.

###Parentheses

Users can parenthesize input expressions as much or as little as they desire,
up to the limits of left-association and the meaning they wish to convey to the
interpreter. The grammar used by `acl` does not allow single terms inside
paired parentheses. It considers strings like `(I)` as syntax errors. You have
to put at least two terms inside a pair of parentheses. Parentheses must have a
match.

The interpreter prints out normal forms in minimal-parentheses style. Users
have the opportunity to cut-n-paste output back into the input, as output has
valid syntax. No keyboard shortcuts exist to take advantage of any previous
output.

#Defining Primitives

`acl` does not implement any built-in primitives. The user must describe
desired primitives to the interpreter.

## Rules specifying primitives

Defining a primitive means getting the interpreter to read a line having this form:

    rule: Name n<sub>i</sub> ... ->  m<sub>i</sub> ...

The `Name` becomes the primitive's name when used in a CL expression.
`Name` has the same format as a C or Java language identifier: a letter
followed by zero or more letters, digits or underscores.

The n<sub>i</sub> symbols represent ascending-value digits, beginning with 1.
These consitute the required arguments of the primitive under definition.

The m<sub>i</sub symbols also represent digits, which must also appear as
n<sub>i</sub> arguments. They represent the positions of arguments after
the primitive reduces. You can delete, rearrange, duplicate and compose
arguments, but you can't introduce other non-argument results. You can only
define "proper" combinators. You can define "regular" or "irregular"
combinators.

### Examples

*   The famous **S** combinator: `rule: S 1 2 3 -> 1 3 (2 3)`
*   The obscure **J** combinator: `rule: J 1 2 3 4 -> 1 3 (2 4 3)`
*   Smullyan's Mockingbird: `rule: M 1 -> 1 1`
*   Half an Ogre: `rule: HalfOgre 1 2 -> 1 1`. An Ogre (`HalfOgre HalfOgre`), will "eat" all arguments.
*   The **K** combinator: `rule: K 1 2 -> 1`
*   The identity combinator: `rule: I 1 -> 1`

The interpreter imposes no limits on number of primitives the user defines, or
on the number of arguments a given primitive uses or produces. Defining large
numbers of primitives will slow it down, as will reducing a primitive that has
a large number of arguments, or produces a large number of results in a reduced
term.

You can redefine a primitive with another `rule:` input which uses the name to
be redefined. You cannot delete a primitive, once you have defined it. You have
to exit the interpreter to "delete" pimitives.

### Primitives versus abbreviations

At first glance, a user defined primitive and an [abbreviation](#defining-abbreviations)
appear redundant. Differences exist, both subtle and gross.

One gross difference: primitives only rearrange, delete, duplicate or regroup
their arguments. An abbreviation (in the form of a bracket abstracted
expression) can insert primitives. No way to construct an "improper" primitive
exists, but abbreviations can do this. Abbreviations can refer to a fixed point
combinator. You cannot define a fixed point combinator as a primitive.

`acl` abbreviations work in a way that encourages the user to build up complex
terms by abbreviating smaller, simpler terms, then invoking all the
abbreviations together. For example, you could build up a fixed-point
combinator like this:

<pre>ACL> def subexpr (x y x)
ACL> def Y ([x,y] subexpr)([y,x] (y subexpr))
</pre>

This contrived example illustrates the use of abbreviating a common
sub-expression, as well as demonstrating that an abbreviation isn't "atomic".
The bracket abstractions capture what the user defines as free variables.

A more subtle distinction exists in that primitives reduce atomically:
normal-order evaluation doesn't take place "inside" a primitive. An example
follows to clarify.

    ACL> rule: S 1 2 3 -> 1 3 (2 3)
    ACL> rule: I 1 -> 1
    ACL> def M (S I I)
    ACL> trace on     #show expression after each contraction
    ACL> detect on    #mark reducible primitives with '*'
    ACL> step on      #single-step, pause after each contraction
    ACL> M M
    S I I (S I I)
    continue? <return>
    [2] I* (S I I) (I* (S I I))   *even out-of-order contractible primitives marked with '*'*
    continue? <return>
    [2] S* I I (I* (S I I))
    continue? <return>
    [4] I* (I* (S I I)) (I* (I* (S I I)))
    continue? <return>
    [3] I* (S I I) (I* (I* (S I I)))
    continue? <return>
    [3] S* I I (I* (I* (S I I)))
    continue? <return>
    [6] I* (I* (I* (S I I))) (I* (I* (I* (S I I))))
    continue? q <return>
    Terminated
    ACL>

Each abbreviation `M` gets expanded into a term `S I I`. The `S I I` terms get
evaluated in normal order, leftmost-outermost contraction happens first. This
leads to an infinitely expanding term, not the `M M` → `M M` cycle that one
might naively hope for.

To achieve an `M M` → `M M` cycle, the user could define `M` as a primitive.

    ACL> rule: M 1 -> 1 1
    ACL> cycles on    #detect cyclical reductions
    ACL> trace on     #show expression after each contraction
    ACL> detect on    #mark reducible primitives with '*'
    ACL> M M
    M M
    [1] M* M
    [1] M* M
    Found a pure cycle of length 1, 1 terms evaluated, ends with ".M M"
    [1] M* M

#Expressing Bracket Abstraction Algorithms

"Bracket abstraction" names the process of creating a CL expression without
specified variables, that when evaluated with appropriate arguments, ends up
giving you the original expression with the specified variables.

The `acl` interpreter uses the conventional square-bracket notation. For
example, to create an expression that will duplicate its single argument, one
would type:

    ACL> [x] x x

You can use more than one variable inside square brackets, separated with commas:

    ACL> [a, b, c] a (b c)

The above square-bracketed expression ends up performing three bracket
abstractions, abstracting `c` from `a (b c)`, `b` from the resulting
expression, and `a` from that expression.

A bracket abstraction makes an expression. You can use abstractions where you
might use any other simple or complex expression, defining an abbreviation, a
sub-expression of a much larger expression, as an expression to evaluate
immediately, or inside another bracket abstraction. For example, you could
create Turing's fixed-point combinator like this:

    ACL> def U [x][y] (x(y y x))
    ACL> def Yturing (U U)

Note the use of nested bracket abstractions. The abstraction of `y` occurs
first, then `x` gets abstracted from the resulting expression.

You could express `[x][y] (x(y y x))` with the alternate form `[x,y] (x(y y
x))`. The same nested abstraction occurs.

`acl` allows you to express even complicated bracket abstraction algorithms.
You can write rules that match specific terms, or that match general
sub-expressions.

## Bracket Abstraction Rules

You input the abstraction algorithm in the form of rules, one per line, in
decreasing order of priority. Each line has this format:

    abstraction: [_] lhs -> rhs

The lexical token `[_]` denotes the abstraction of whatever variable the user
chooses. It must appear after the `abstraction:` token. It can also appear in
the right-hand-side of an abstraction rule, where it will cause recursive
abstraction(s).

#### Left Hand Side

The left-hand-side (`lhs`) looks like a valid CL term, except that it can
contain one or more special symbols, as well as names of primitives and/or
variables:

*   `_` (underscore) - a "wildcard" for the name of the variable abstracted.
*   `*` (asterisk) - a wildcard meaning "any term, possibly containing the abstracted variable".
*   `*-` (asterisk minus) - a wildcard meaning "any term not containing the abstracted variable".
*   `*!` (asterisk bang) - a wildcard meaning "any term without variables".
*   `*+` (asterisk plus) - a wildcard meaning "any term containing the abstracted variable".
*   `*^` (asterisk caret) - a wildcard meaning "a lexically identical string", if it appears twice in one rule.

#### Right Hand Side

The right-hand-side (rhs) also looks like a valid CL term, except that it can
contain one or more special symbols, as well as the names of primitives and/or
variables:

*   `[_]` - the abstraction-of-whatever-variable again. Triggers a "re-abstraction" of the term constructed by the right-hand-side.
*   `_` (plain old underscore), replaced by the abstracted variable.
*   Digits, `1, 2, 3 …` - these stand for terms (including wildcard symbols (`*`, `*-`, `*!`, `*+`)) that appear in the left-hand-side of the rule under consideration. The matching left-hand term gets used in the righ-hand-side.

#### Rule Precedence

The order in which the user enters rules amounts to specifying precedence. The
first rule entered at the `ACL>` prompt has the highest precedence. The last
rule entered has the lowest precedence.

The user must [define primitives](#defining-primitives) before using them in a
right-hand-side. Otherwise, what appears as a "primitive" will actually
constitute a free variable with a confusingly identical name.

### Bracket Abstraction Rule Examples

The rule format above allows expression of the standard SKI-basis 4-rule
algorithm like this:

1.  `abstraction: [_] *- -> K 1`
2.  `abstraction: [_] _ -> I`
3.  `abstraction: [_] *- _ -> 1`
4.  `abstraction: [_] * * -> S ([_] 1) ([_] 2)`

In rule 1, the `*-` on lhs gets into the resulting expression as the '1' on the right-hand-side.

Similarly, in rule 4, the two expressions on the lhs, each denoted by an '*',
have the variable abstracted ('[_]') in the rhs.

The 9-rule bracket abstraction algorithm from John Tromp's [Binary Lambda
Calculus and Combinatory Logic](https://tromp.github.io/cl/LC.pdf) looks like
this:

1.  `abstraction: [_] S K * -> S K`
2.  `abstraction: [_] *- -> K 1`
3.  `abstraction: [_] _ -> S K K`
4.  `abstraction: [_] *- _ -> 1`
5.  `abstraction: [_] _ * _ -> [_] S S K _ 2`
6.  `abstraction: [_] (*! (*! *)) -> [_] (S ([_]1) 2 3)`
7.  `abstraction: [_] (*! * *!) -> ([_] S 1 ([_]3) 2)`
8.  `abstraction: [_] *! *^ (*! *^) -> [_] (S 1 3 2)`
9.  `abstraction: [_] * * -> S ([_] 1) ([_] 2)`

Tromp writes rule 5 above as:

    _λ<sup>2</sup>x.(x M x) ≡ λ<sup>2</sup>x.(S S K x M)

The abstracted variable `x` appears in the right- and left-hand-side of the
rule as `_` . Tromp's `M` appears as `*` on lhs and `2` on rhs of the `acl`
abstraction rule.

The right-hand-side of the rule references the abstracted-out-variable. The
digit `2` in the RHS refers to the `*` (match any term) marker in the LHS. The
RHS also builds a term which has that variable re-abstracted.

Rules 7 and 9 above feature the '*!' special symbol, which means "term
containing no variables whatsoever", a.k.a. a combinator. Tromp seeks to
minimize the size of the end result of multi-variable abstractions. Variables
exist only to get abstracted away, so a term containing multiple variables will
undergo multiple abstractions. Applying rules 6 and 7 to terms more than once
ends up making the resulting variable-free terms much larger than they
otherwise would end up.

Rule 8 above features the '*^' special symbol. This rule triggers an
abstraction when it finds two, lexically-identical sub-expressions, in the
positions described. The two expressions in the RHS denoted by '*!' do not get
checked for lexical equality, but must not contain variables for the rule to
trigger an abstraction.

The "*^" special symbol can appear more than once, should you find it
interesting to do so. All sub-trees marked with a '*^' must compare identically
to trigger the abstraction rule. More than one abstraction rule can contain
'*^' symbols, but the lexical identity only gets checked during examination of
a single rule. Lexical identity does not get checked "across rules".

#Interpreter Commands

*   [Defining abbreviations](#defining-abbreviations)
*   [Information about expressions](#information-about-expressions)
*   [Detecting reduction cycles](#detecting-reduction-cycles)
*   [Intermediate output and single-stepping](#intermediate-output-and-single-stepping)
*   [Reduction information and control](#reduction-information-and-control)
*   [Reading in files](#reading-in-files)
*   [Printing primitive and abstraction rules](#printing-primitive-and-abstraction-rules)

## Defining abbreviations

*   `define _name_ _expression_`
*   `def _name_ _expression_`
*   `reduce _expression_`

`define` and `def` allow a user to introduce "abbreviations" to the input later. Using `define` or `def` causes `acl` to keep an internal copy of the expression so abbreviated. When the abbreviation appears in an input expression, `acl` puts a copy of the internal-held expression in the input. No matter how complex the expression, an abbreviation comprises a single term. Effectively, the interpreter puts the expanded abbreviation inside a pair of parentheses.

`def` makes an easy-to-type abbreviation of `define`.

The `reduce` command actually produces an expression, just like a bracket
abstraction. Unlike `define` or `def`, you can use `reduce` anywhere an
expression would fit, as part of a larger expression, as part of an
abbreviation, or as part of a bracket abstraction.

`reduce` causes an out-of-order (normal order, leftmost outermost) reduction to
take place. The expression next to `reduce` gets interpreted, and the result
put back into the original context. `reduce` allows a user to store the normal
form of an expression, rather than just a literal expression, as `def` and
`define` don't cause any contractions to take place.

## Information about expressions

*   `size expression` - print the number of atoms in expression.
*   `length expression` - print the number of atoms plus number of applications in expression.
*   `print expression` - print human-readable representation, with abbreviations expanded, but without evaluation.
*   `printc expression` - print [canonical](#canonical-expression-representation) representation, with abbreviations substituted, but without evaluation.
*   `redexes <expression>` - print a count of possible contractions in expression, regardless of order of evaluation.
*   `<expression> = <expression>` - determine lexical equivalence of any two expressions, after abbreviation substitution, but without evaluation.

`print` lets you see what abbreviations expand to, without evaluation, as does
`printc`. The "=" sign lets you determine *lexical* equality. All combinators,
variables and parentheses have to match as strings, otherwise "=" deems the
expressions not equivalent. You can put in explicit `reduce` commands on both
sides of an "=", otherwise, no evaluation takes place.

`size` and `length` seem redundant, but authorities measure CL expressions
different ways. These two methods should cover the vast majority of cases.

#### Canonical Expression Representation

The `printc` command, and cycle detection output use a canonical
form of representing a CL expression.

In this case, "canonical" means: pre-order, depth-first, left-to-right
traversal of the parse tree, output of a period (".") for each application, and
a space before each combinator or variable.

A simple application (`K I`, for example) looks like this: `.K I`

A more complex application (`P R (Q R)`) looks like this: `..P R.Q R`

The advantage to this sort of notation is that every application appears
explicitly, and variant, semantically-equivalent parentheses don't appear.

## Detecting reduction cycles

*   `cycles on|off`
*   `detect on|off`

Some CL expressions end up creating a cycle. `M M` or `W W W` constitute
examples from common bases. After a certain number of contractions, the
interpreter encounters an expression it has previously created. If you issue
the `cycles on` command, the interpreter keeps track of every expression in the
current reduction, and stops when it detects a cyclical reduction.

`detect on` causes the interpreter to count and mark possible contraction (with
an asterisk), regardless of reduction order. It does "normal order" reduction,
but ignores that for the contraction count. This has use with `trace on`.

Turning cycle detection on will add time to an expression's reduction, as will
possible contraction detection.

## Intermediate output and single-stepping

*   `trace on|off`
*   `elaborate on|off`
*   `debug on|off`
*   `step on|off`

You can issue any of these commands without an "on" or "off" argument to
inquire about the current state of the directive.

`trace`, `debug` and `elaborate` provide increasingly verbose output. `trace`
shows the expression after each contraction, `debug` adds information about
which branch of an application it evaluates, and `elaborate` adds to that
information useful in debugging memory allocation problems.

`detect` causes `trace` to also print a count of possible contractions (not all
of them normal order reductions), and mark contractable primitives with an
asterisk.

`step on` causes the interpreter to stop after each contraction, print the
intermediate expression, and wait, at a `?` prompt for user input. Hitting
return goes to the next contraction, `n` or `q` terminates the reduction, and
`c` causes it to quit single-stepping.

`trace on` will also display steps taken during bracket abstractions.

## Reduction information and control

*   `timer on|off` - turn on/off per-reduction elapsed time output.
*   `timeout 0|N`- stop reducing after `N` seconds.
*   `count 0|N` - stop reducing after `N` contractions.

You can turn time outs off by using a 0 (zero) second timeout. Similarly, you
can turn contraction-count-limited evaluation off with a 0 (zero) count.

`timer on` also times [bracket abstraction](#expressing-bracket-abstraction-algorithms).

## Reading in files

*   `load "filename"`

You have to double-quote filenames with whitespace or non-alphanumeric
characters in them. You can use absolute filenames (beginning with "/") or you
can use filenames relative to the current working directory of the `acl`
process.

## Printing primitive and abstraction rules

*   `rules` - prints out what rules about primitives it has.
*   `abstractions` - prints out what abstraction rules it has.

Rules about primitives and abstraction rules should appear in a format
appropriate for cut-n-paste, that is, in input syntax. Abstraction rules should
appear in order of precedence, highest precedence first.

#Examples

The following examples demonstrate either an interesting facet of Combinatory
Logic (Klein fourgroup, Grzegorzyk bracket abstraction, AMEN basis) or
illustrates part of the interpreter that I found hard to describe (abstraction
rules in Tromp's bracket abstraction, and Scott Numerals example). Your tastes
may vary.

*   [SKI Basis](bases/ski.basis), with "abcf" bracket abstraction.
*   [SKI Basis](bases/tromp.abstraction), with John Tromp's bracket abstraction.
*   [BWCK Basis](bases/bwck.basis), Grzegorzyk bracket abstraction.
*   [IJ Basis (λI basis)](bases/ij.basis), including bracket abstraction.
*   [IBCS Basis (λI basis)](bases/ibcs.basis), with adequate Church Numerals.
*   [BTMK Basis](bases/btmk.basis), with home-grown bracket abstraction.
*   [SNC Basis](bases/snc.basis), with home-grown bracket abstraction.
*   [AMEN Basis](bases/amen.basis), another home-grown bracket abstraction algorithm.
*   [OAME Basis](bases/oame.basis), bracket abstraction similar to AMEN basis.
*   [Scott Numerals](bases/scott.numerals) in an 8-combinator basis, and a 9-rule abstraction algorithm.
*   [D Numerals](bases/d_numerals), where `[x] x` represents 0, `[x]x x` represents 1, …
*   [Klein fourgroup](bases/fourgroup), with application as the operator.
*   [Miscellaneous bases](bases/bases.html)

#Building and installing

I developed this program on Slackware Linux 12.0, and Arch Linux. I used the C
compilers GCC, [LCC](http://www.cs.princeton.edu/software/lcc/),
[PCC](http://pcc.ludd.ltu.se/), [TCC](http://bellard.org/tcc/) and
[Clang](http://clang.llvm.org/). I tried to make it as ANSI-C as possible by
using various compilers, not allowing any compiler warnings when building, and
using as many warning flags for GCC compiles as possible.

I expect it will build on any *BSD-based system, but I haven't done that
formally. For a BSD system, try `make cc`.

## Licensing

Licensed under GNU Public License v2, or later.

## Instructions

1.  Compile source code. From the command line:
    *   `make gnu` - for almost every linux distro.
    *   `make cc` - for BSDs and Solaris.`make cc` uses traditionally-unix-named tools, and may work better on Solaris or the BSDs. Traditional, AT&T derived `lex` will require a minor edit to file `lex.l`. You can see what to uncomment at the very top of the file.
2.  At this point, you can test the newly-compiled executable. From the command
    line: `./runtests`. Most of the tests should run quite rapidly, in under a
    second. At least two of the tests run for 30 seconds or so, and at least one of
    the tests provokes a syntax error message from the interpreter.
3.  Install the interpreter wherever you want, or you can execute it in-place.
    To install, use the `cp` or `mv` commands to move or copy the executable to
 	where ever you want it. It does not care what directory it resides in, and it
    does not look for configuration files anywhere.
