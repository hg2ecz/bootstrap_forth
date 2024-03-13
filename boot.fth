HEAD 0  dup dup xor  dup , ,
HEAD 1  0 ,  0 1+ ,

HEAD :  1 ,  compile: head 1 ,  compile: ;

: CONSTANT   head 0 , , ;
: CREATE     head 1 1+ , ;
: VARIABLE   create  0 , ;

1   dup constant DP
1+  dup constant CURRENT
1+      constant CONTEXT

: IMMEDIATE   current @ @ 1 +  dup @ 1 or  swap ! ;

1 2* 2* 1+ 2* 2* 2* 1+  constant PAREN
: (   paren parse  drop drop ; immediate
  ( now we can use comments )

( DP, CURRENT, and CONTEXT are actually variables.)
( head: link, name, cf )

( ASCII codes: paren 41, semicolon 59 )

( HEAD PAREN  0 ,  1 2* 2* 1+ 2* 2* 2* 1+ ,
  HEAD SEMICOLON  0 ,  1 2* 2* 2* 2* 1- 2* 2* 1- , )

: ?DUP   dup -exit  dup ; ( 0 -- 0 | x -- x x)
: ROT    >r swap r> swap ; ( x1 x2 x3 -- x2 x3 x1)
: NIP    swap drop ; ( x1 x2 -- x2)
: TUCK   swap over ; ( x1 x2 -- x2 x1 x2)

: +!   swap  over @ +  swap ! ; ( n a -- )
: C@   @ ; ( ca -- c)  ( note: have only word addressing)
: C!   ! ; ( c ca --)  ( note: have only word addressing)

: =   - 0= ; ( n1 n2 -- f)
: <   - 0< ; ( n1 n2 -- f)

( Numbers)
variable BASE
1 2* 2* 1+ 2*  base !

: ?BAD   0= -exit  drop  1 error ; ( f -- )
: (NUMBER)   ?dup 0< -exit  dup >r
   in@ digit  dup base @ < ?bad  swap base @ * +
   r> 1+ goto (number) ;  ( n1 count -- n2 )

: ?WORD   ?dup -exit  nip execute ;  ( n a|0 -- n | ??)
: ?NUM   dup 0= -exit  swap (number)  0 ; ( n1 a|0 -- n1 a | n2 0)

: INTERPRET   word  dup -exit  find
    ?num ?word  goto interpret ; ( - 0 0)
( now use it )  interpret

( --- from here on, the INTERPRETER understands numbers --- )

: '   word find  nip ; ( XXX need error if not found )

VARIABLE STATE ( non-zero while compiling)
: ;   lit unnest ,  0 state ! ; immediate

create (COMPILER)  ' , ,  ' execute ,
: ?DEFINED   ?dup -exit  nip  dup 1- @ 1 and
   (compiler) + @ execute ;
: ?LITERAL   dup 0= -exit  swap (number)  lit lit , ,  0 ;
: COMPILE   word find  ?literal ?defined  state @ -exit  goto compile ;

: :   head 1 ,  1 state !  compile ;

( --- from here on, the COMPILER understands literals --- )
    ( and immediate words )

: HERE    dp @ ; ( -- a )
: ALLOT   here +  dp ! ; ( n -- )
: ,       here !  1 dp +! ; ( x -- )

: [COMPILE]   ' , ; immediate

( traditional control structures)
: BEGIN   here ; immediate
: END     lit -branch ,  here - , ; immediate
: AGAIN   lit branch ,   here - , ; immediate
: IF      lit -branch ,  here 0 , ; immediate
: THEN    here over -  swap ! ; immediate
: ELSE    lit branch ,  here 0 ,  swap  [compile] then ; immediate

( 1 2* 2* 2* 2* 2* )  32 constant BL
: SPACE   bl emit ;
: SPACES   ?dup -exit  space 1- goto spaces ; ( u -- )
: (TYPE)   ?dup -exit  swap  dup c@ emit  1+  swap 1-  goto (type) ;
: TYPE    (type) drop ;

( : /mod   dup >r  over >r  mod  r> r> / ; )

variable HLD
: PAD    here  bl 2* 2*  + ;
: HOLD   hld @ 1-  dup hld !  c! ; ( c --)
: SIGN   dup 0< -exit  negate  45 hold ; ( n -- u)
: >DIGIT   9 over < 7 and +  48 + ; ( 48 = '0')
: <#     pad hld ! ;
: #>     drop  hld @  pad over - ; ( u -- ca n)
: #      base @ /mod swap  >digit hold ; ( u1 -- u2)
: #S     #  dup -exit  goto #s ; ( u -- 0)
: ..     <# bl hold  dup #s drop sign #>  type ;
