# Bootstrap Forth in C

( Original: https://forth.neocities.org/bootstrap/ )

The goal is to create a small C program that implements the primitives which
are usually written in assembler and just enough of an interpreter and compiler
so that the rest of the system can be written in Forth.

The resulting system can either be used stand-alone, or to bring up a more
traditional Forth system by cross-compiling.

Some design decisions:

  • No immediate words
  • No literal numbers
  • No strings
  • Enough code to write the bulk of the kernel in Forth itself
  • Only one word list (vocabulary)
  • Saves length and up to 4 characters of a name
  • Cells are 32-bit wide
  • Memory is addressed by cells, not bytes

Stage 0 — The C Code

Creates a small dictionary; defines layout for headers; starts last word
defined; defines 2 variables at known addresses; order of first 4 or so
primitives known in Forth code.

The C code creates a small dictionary containing all primitives needed to
implement a full Forth system. These are usually implemented in assembler; here
they are written in "portable assembler" aka C.

The last word defined is automatically started. It implements a simple
interpreter loop that parses a name terminated by the next blank, looks it up
in the dictionary, and then executes it. If it is not found, the system simply
exits with an error message. This means that there is no support for numbers.

Some of the primitives defined (e.g. lit or branch) are only meant for being
compiled into other words and cannot be interpreted. Don't use them from the
interpreter.

A handful of words allow the system to be extended by writing Forth code that
obeys a couple of simple rules. These words are for parsing the input, creating
a new entry in the dictionary, searching the dictionary and a small compiler
similar to the interpreter that compiles words instead of executing them. There
is no way to execute words while compiling; everything gets compiled into the
dictionary, even control-flow words.

This requires special control-flow words as the usual Forth words rely on
execution during compilation; they are all immediate words. Instead, we have

  • -exit to conditionally leave a colon definition, and
  • goto to continue execution at the beginning of a colon definition.

Since names are not "hidden" in the dictionary during compilation, the latter
can be used to form loops.

Stage 1 — Forth Code

Glossary
```
c          character
n, n1, n2… signed number
nt         name token
a          address
x, x1, x2… anything
xt         execution token

xt ,
    Add xt to the dictionary.
boot
    The interpreter loop.
compile:
    The compiler loop.
nt find xt | 0
    Lookup nt in dictionry. If found, leave its execution token xt; otherwise
    leave 0 as a flag.
head
    Creates header for a new dictionary entry. XXX
c parse n1 n2
    Parse a string ending at next delimiter c. Does not skip leading
    delimiters.
token n
    Parse a name delimited by space characters and return an indication of its
    length.
word n nt
    Parse a name with token and convert it to a name token nt. XXX
```

Files

Source code in forth.c and boot.fth.

A simple Makefile for POSIX systems.
