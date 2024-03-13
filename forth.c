/* simple forth */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DP if (0) printf

typedef int cell;

/* all sizes are in cells */
#define DSTACKSIZE 200
#define RSTACKSIZE 200
#define MEMSIZE 10000

#define SP0 0 /*(MEMSIZE - DSTACKSIZE)*/
#define RP0 0 /*(SP0 - RSTACKSIZE)*/

static cell dstack[DSTACKSIZE];
static cell rstack[RSTACKSIZE];
static cell memory[MEMSIZE];

/* registers */
static int sp;
static int rp;
static int ip;
static int w;

/* initial part of memory has well-known layout: */
#define START 0  /* entry word */
#define HERE 1
#define CURRENT 2
#define CONTEXT 3
#define LATEST 4  /* latest dictionary entry */

/* word headers */
#define LINK 0
#define NAME 1
#define CODE 2

#define here memory[HERE]
#define latest memory[LATEST]

#define TOP dstack[sp]  /* top element on stack */
#define SND dstack[sp-1]  /* second element on stack */
#define DROP sp--
#define DROPn(n) sp -= n

#define POP dstack[DROP]
#define PUSH(x) dstack[++sp] = x

#define RPUSH(x) rstack[++rp] = x
#define RPOP rstack[rp--]
#define RTOP rstack[rp]

#define unary(op) { TOP = op TOP; }
#define binary(op) { SND = SND op TOP; DROP; }
#define compare(op) { SND = -(SND op TOP); DROP; }
#define comp0(op) { TOP = -(TOP op 0); }

#define WW(name, sym) static void name()
#define W(name) WW(name, #name)

static void run(cell cfa);

W(doconst) { PUSH(memory[w + 1]); }  /* this MUST be the first primitive */
W(nest) { RPUSH(ip); ip = w + 1; }  /* this MUST be the second primitive */
W(dovar) { PUSH(w + 1); }  /* this MUST be the third primitive */
W(unnest) { ip = RPOP; }
W(lit) { PUSH(memory[ip++]); }
W(execute) /* cfa */ { run(POP); }
W(branch) { ip += memory[ip]; }
WW(zbranch, "-branch") { ip += (!POP) ? memory[ip] : 1; }

WW(jump, "goto") { ip = memory[ip] + 1; }  /* only for colon defs! */
WW(exitunless, "-exit") { if (!POP) ip = RPOP; }
/*WW(skipif, "?skip") { if (POP) ip++; }*/


W(drop) { DROP; }
W(dup) { cell a = TOP; PUSH(a); }
W(over) { cell a = SND; PUSH(a); }
W(swap) { cell a = SND; SND = TOP; TOP = a; }
// W(rot) { cell c = POP; cell a = SND; SND = TOP; TOP = c; PUSH(a); }
// W(nip) { SND = TOP; DROP; }  /* swap drop */
// W(tuck) { cell a = TOP; TOP = SND; SND = a; PUSH(a); }  /* swap over */
// WW(qdup, "?dup") { cell a = TOP; if (a) PUSH(a); }

WW(rpush, ">r") { RPUSH(POP); }
WW(rpop, "r>") { PUSH(RPOP); }
WW(rtop, "r@") { PUSH(RTOP); }

WW(spstore, "sp0!") { sp = SP0; }
WW(rpstore, "rp0!") { rp = RP0; }

WW(add, "+") binary(+)
WW(sub, "-") binary(-)
WW(times, "*") binary(*)
WW(slashmod, "/mod") { div_t x = div(SND, TOP); SND = x.rem; TOP = x.quot; }
WW(divide, "/") binary(/)
W(mod) binary(%)

W(negate) unary(-)
W(invert) unary(~)

W(and) binary(&)
W(or) binary(|)
W(xor) binary(^)

WW(zero, "0=") comp0(==)
WW(negative, "0<") comp0(<)
WW(positive, "0>") comp0(>)

// WW(eq, "=") compare(==)
// WW(ne, "<>") compare(!=)
// WW(lt, "<") compare(<)
// WW(le, "<=") compare(<=)

WW(add1, "1+") { TOP = TOP + 1; }
WW(sub1, "1-") { TOP = TOP - 1; }
WW(mul2, "2*") { TOP = TOP << 1; }
WW(div2, "2/") { TOP = TOP >> 1; }

WW(fetch, "@") { int a = TOP; TOP = memory[a]; }  /* XXX word address */
WW(store, "!") { int a = POP; memory[a] = POP; }  /* XXX word address */

WW(comma, ",") { memory[here++] = POP; }

W(digit) /* c - n */ {
    int d = TOP;
    TOP = ('0' <= d && d <= '9') ? d - '0' :
          ('A' <= d && d <= 'Z') ? d - 'A' + 10 : 255;
}

static void fatal(const char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	abort();
}

W(error) /* n -- */ {
    fprintf(stderr, "error %d\n", (int) TOP);
    exit(EXIT_FAILURE);
}

static cell abbrev(const char *name, int l) {
	/* XXX magic numbers for 32-bit cells */
	int i;
	if (l >= 32) fatal("name too long");  /* name too long */
	cell nf = l;
	for (i=0; i<4 && i<l; i++) {
		int c = name[i];
		if ('a' <= c && c <= 'z')
			c = c - 'a' + 'A';
		if (c < ' ') c = ' ';
		c = c - ' ';
		if (c >= 64) fatal("unsupported name char");  /* unsupported name char */
		nf = (nf << 6) + c;
	}
	for (; i<4; i++)
		nf = nf << 6;
	return (nf << 2);
}

static cell lookup(cell name) {
	cell a = memory[CONTEXT];
	for (a = memory[a]; a; a = memory[a + LINK])
		if ((memory[a + NAME] & ~ 3) == name)
			return a + CODE;
	return 0;
}

/* input buffer */
static const char *input;
static int inputlen;
static int pos;

W(parse) /* c - n1 n2 */ {
	int delimiter = TOP;
	TOP = pos;
	while (pos < inputlen && input[pos] != delimiter) pos++;
	PUSH(pos);
DP("after parse: [%d...%d]\n", pos, inputlen);
	if (pos < inputlen) pos++;
}
W(token) /* - n */ {
	/* skip leading blanks */
	while (pos < inputlen && input[pos] <= ' ') pos++;
    int p0 = pos;
	while (pos < inputlen && !(input[pos] <= ' ')) pos++;
	PUSH(p0 - pos);
	if (pos < inputlen) pos++;
}

WW(infetch, "in@") /* n - c */ {
	int n = TOP;
	if (n >= 0) fatal("cannot look ahead");
	if (pos < -n) fatal("no source");
	TOP = input[pos - 1 + n];  /* "- 1" is wrong if input ends with number */
}

/* ca n - x */
W(compact) { cell l = POP; TOP = abbrev((char*)memory + TOP, l); }
W(find) /* name - a|0 */ { TOP = lookup(TOP); }
W(word) /* - n name */ {
	token();
DP("@@ sp=%d rp=%d (... %d %d %d %d)\n", sp, rp, dstack[sp-3], dstack[sp-2], SND, TOP);
    int len = TOP;
	PUSH(abbrev(&input[pos - 1 + len], -len));
}
W(head) /* create a new head */ {
	word();
    cell current = memory[CURRENT];
	memory[here + LINK] = memory[current];
    memory[here + NAME] = TOP;
    memory[current] = here;
	here += CODE;
	DROPn(2);
DP("head: %d %08x  xt=%d\n", latest, memory[latest + NAME], latest+CODE);
}
static void lay(const char *word) {
	PUSH(lookup(abbrev(word, strlen(word))));
	DP("lay: %d @ %d\n", TOP, here);
	comma();
}
static void notfound() {
	DROP;
    /*
	int p1 = SND, p2 = TOP;
	for (; p1 < p2; p1++)
		fputc(input[p1], stdout);
    */
    fputs("\e[31;1m", stderr);
    for (int len = TOP; len < 0; len++)
        fputc(input[pos - 1 + len], stderr);
    fputs(" ?\e[0m\n", stderr);
	PUSH(0); /*XXX*/
}
WW(compiler, "compile:") /* stops at ';' - only good for bootstrapping */ {
	// head(); PUSH(1); comma();
	PUSH(';'); parse();
	int oldlen = inputlen, oldpos = pos;
	for (inputlen = POP, pos = POP; ;) {
		word();
		if (TOP == 0) /* end of input */ break;
		find();
		if (TOP == 0) {
			notfound();
			break;
		}
		comma();
		DROP;
	}
	DROPn(2);
	inputlen = oldlen;
	pos = oldpos;
	lay("unnest");
}

WW(showstack, ".s") {
    printf("rp=%d sp=%d (... %d %d %d)\n", rp, sp,
        sp >= 2 ? dstack[sp-2] : -22222,
        sp >= 1 ? SND : -11111, TOP);
}
W(depth) { int x = sp - SP0; PUSH(x); }
W(emit) { fputc(POP, stdout); }
W(cr) { fputc('\n', stdout); }
WW(dot, ".") { printf("%d ", POP); }

W(hello) { printf("hello, world!\n"); }
W(bye) { exit(EXIT_SUCCESS); }
W(boot) {
    sp = SP0;
	for (;;) {
        rp = RP0;
DP("[%d...%d]\n", pos, inputlen);
		word();
DP("sp=%d rp=%d (... %d %d $%x)\n", sp, rp, dstack[sp-2], SND, TOP);
		if (TOP == 0) {
			fputs("Bye.\n", stdout);
			exit(EXIT_SUCCESS);
		}
		find();
		if (TOP == 0) {
			notfound();
			DROPn(2);
		} else {
			cell xt = TOP;
			DROPn(2);
DP("@x sp=%d rp=%d (... %d %d %d)  xt=%d\n", sp, rp, dstack[sp-2], SND, TOP, xt);
			run(xt);
DP(".. exec done\n");
		}
	}
}

static struct { void (*fn)(); const char *name; } prims[] = {
#undef WW
#define WW(name, sym) { name, sym },
#include "words.h"
	{ 0, 0 }
};

static void run(int cfa) {
#define xCALL(cfa)	(w = cfa, printf("r>%4d w=%4d w@=%2d %s\n", ip, w, memory[w], prims[memory[w]].name), prims[memory[w]].fn())
#define CALL(cfa)	(w = cfa, prims[memory[w]].fn())
	int rp0 = rp;
DP(">>> cfa=%4d  rp=%d\n", cfa, rp);
	CALL(cfa);
	while (rp > rp0) CALL(memory[ip++]);
}

static void init() {
	int i;

	sp = SP0;
	rp = RP0;
	here = 10;
	latest = 0;
	for (i = 0; prims[i].name; i++) {
		memory[here + LINK] = latest; latest = here;
		memory[here + NAME] = abbrev(prims[i].name, strlen(prims[i].name));
		memory[here + CODE] = i;
DP("%d #%2d '%s' %08x\n", latest, i, prims[i].name, memory[here + NAME]);
		here += CODE + 1;
	}
	memory[START] = latest + 2;
    memory[CURRENT] = LATEST;
    memory[CONTEXT] = LATEST;
    TOP = 999; PUSH(888);
	PUSH(memory[START]);
	/*
	memory[START] = here;
	memory[here] = latest + 2;
	*/
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		fputs("need at least one source file\n", stderr);
		exit(EXIT_FAILURE);
	}
	if (argc > FOPEN_MAX) {  /* off by one, but who cares? */
		fputs("too many files to open\n", stderr);
		exit(EXIT_FAILURE);
	}
	FILE *fp[argc];
	long len[argc];
	long pos, total = 0;
	int i;
	for (i = 1; i < argc; i++) {
		fp[i] = fopen(argv[i], "r");
		if (fp[i] == NULL) {
			perror(argv[i]);
			exit(EXIT_FAILURE);
		}
		fseek(fp[i], 0L, SEEK_END);
		len[i] = ftell(fp[i]);
		total += len[i];
	}
DP("need buffer for %ld bytes\n", total);
	char buffer[total];
	pos = 0L;
	for (i = 1; i < argc; i++) {
		rewind(fp[i]);
		if (fread(&buffer[pos], len[i], 1, fp[i]) != 1) {
			perror(argv[i]);
			exit(EXIT_FAILURE);
		}
		fclose(fp[i]);
		pos = pos + len[i];
	}

	input = buffer;
	inputlen = total;
	pos = 0;

	init();
	execute();
	return EXIT_SUCCESS;
}
