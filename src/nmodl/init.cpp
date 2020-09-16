#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/nmodl/init.c,v 4.5 1998/03/25 14:33:42 hines Exp */

#include "modl.h"
#include "parse1.h"

extern List    *firstlist;
extern List    *syminorder;
Symbol         *semi, *beginblk, *endblk;
List           *intoken;
char		buf[NRN_BUFSIZE];	/* volatile temporary buffer */

static struct {			/* Keywords */
	char           *name;
	short           kval;
}               keywords[] = {
                        (char*)"VERBATIM", VERBATIM,
                        (char*)"COMMENT", COMMENT,
                        (char*)"TITLE", MODEL,
                        (char*)"CONSTANT", CONSTANT,
                        (char*)"PARAMETER", PARAMETER,
                        (char*)"INDEPENDENT", INDEPENDENT,
                        (char*)"ASSIGNED", DEPENDENT,
                        (char*)"INITIAL", INITIAL1,
                        (char*)"TERMINAL", TERMINAL,
                        (char*)"DERIVATIVE", DERIVATIVE,
                        (char*)"EQUATION", EQUATION,
                        (char*)"BREAKPOINT", BREAKPOINT,
                        (char*)"CONDUCTANCE", CONDUCTANCE,
                        (char*)"SOLVE", SOLVE,
                        (char*)"STATE", STATE,
                        (char*)"STEPPED", STEPPED,
                        (char*)"LINEAR", LINEAR,
                        (char*)"NONLINEAR", NONLINEAR,
                        (char*)"DISCRETE", DISCRETE,
                        (char*)"FUNCTION", FUNCTION1,
                        (char*)"FUNCTION_TABLE", FUNCTION_TABLE,
                        (char*)"PROCEDURE", PROCEDURE,
                        (char*)"PARTIAL", PARTIAL,
                        (char*)"INT", INT,
                        (char*)"DEL2", DEL2,
                        (char*)"DEL", DEL,
                        (char*)"LOCAL", LOCAL,
                        (char*)"METHOD", USING,
                        (char*)"STEADYSTATE", USING,
                        (char*)"SENS", SENS,
                        (char*)"STEP", STEP,
                        (char*)"WITH", WITH,
                        (char*)"FROM", FROM,
                        (char*)"FORALL", FORALL1,
                        (char*)"TO", TO,
                        (char*)"BY", BY,
                        (char*)"if", IF,
                        (char*)"else", ELSE,
                        (char*)"while", WHILE,
                        (char*)"START", START1,
                        (char*)"DEFINE", DEFINE1,
                        (char*)"KINETIC", KINETIC,
                        (char*)"CONSERVE", CONSERVE,
                        (char*)"PLOT", PLOT,
                        (char*)"VS", VS,
                        (char*)"LAG", LAG,
                        (char*)"RESET", RESET,
                        (char*)"MATCH", MATCH,
                        (char*)"MODEL_LEVEL", MODEL_LEVEL,	/* inserted by merge */
                        (char*)"SWEEP", SWEEP,
                        (char*)"FIRST", FIRST,
                        (char*)"LAST", LAST,
                        (char*)"COMPARTMENT", COMPARTMENT,
                        (char*)"LONGITUDINAL_DIFFUSION", LONGDIFUS,
                        (char*)"PUTQ", PUTQ,
                        (char*)"GETQ", GETQ,
                        (char*)"IFERROR", IFERROR,
                        (char*)"SOLVEFOR", SOLVEFOR,
                        (char*)"UNITS", UNITS,
                        (char*)"UNITSON", UNITSON,
                        (char*)"UNITSOFF", UNITSOFF,
                        (char*)"TABLE", TABLE,
                        (char*)"DEPEND", DEPEND,
                        (char*)"NEURON", NEURON,
                        (char*)"SUFFIX", SUFFIX,
                        (char*)"POINT_PROCESS", SUFFIX,
                        (char*)"ARTIFICIAL_CELL", SUFFIX,
                        (char*)"NONSPECIFIC_CURRENT", NONSPECIFIC,
                        (char*)"ELECTRODE_CURRENT", ELECTRODE_CURRENT,
                        (char*)"SECTION", SECTION,
                        (char*)"RANGE", RANGE,
                        (char*)"USEION", USEION,
                        (char*)"READ", READ,
                        (char*)"WRITE", WRITE,
                        (char*)"VALENCE", VALENCE,
                        (char*)"CHARGE", VALENCE,
                        (char*)"GLOBAL", GLOBAL,
                        (char*)"POINTER", POINTER,
                        (char*)"BBCOREPOINTER", BBCOREPOINTER,
                        (char*)"EXTERNAL", EXTERNAL,
                        (char*)"INCLUDE", INCLUDE1,
                        (char*)"CONSTRUCTOR", CONSTRUCTOR,
                        (char*)"DESTRUCTOR", DESTRUCTOR,
                        (char*)"NET_RECEIVE", NETRECEIVE,
                        (char*)"BEFORE", BEFORE, /* before NEURON sets up cy' = f(y,t) */
                        (char*)"AFTER", AFTER, /* after NEURON solves cy' = f(y, t) */
                        (char*)"WATCH", WATCH,
                        (char*)"FOR_NETCONS", FOR_NETCONS,
                        (char*)"THREADSAFE", THREADSAFE,
                        (char*)"PROTECT", PROTECT,
                        (char*)"MUTEXLOCK", NRNMUTEXLOCK,
                        (char*)"MUTEXUNLOCK", NRNMUTEXUNLOCK,
                        (char*)"REPRESENTS", REPRESENTS,
	                0, 0
};

/*
 * the following special output tokens are used to make the .c file barely
 * readable 
 */
static struct {			/* special output tokens */
	char           *name;
	short           subtype;
	Symbol        **p;
}               special[] = {
                        (char*)";", SEMI, &semi,
                        (char*)"{", BEGINBLK, &beginblk,
                        (char*)"}", ENDBLK, &endblk,
	                0, 0, 0
};

static struct {			/* numerical methods */
	char           *name;
	long           subtype;	/* All the types that will work with this */
	short		varstep;
}               methods[] = {
                        (char*)"adams", DERF | KINF, 0,
                        (char*)"runge", DERF | KINF, 0,
                        (char*)"euler", DERF | KINF, 0,
                        (char*)"adeuler", DERF | KINF, 1,
                        (char*)"heun", DERF | KINF, 0,
                        (char*)"adrunge", DERF | KINF, 1,
                        (char*)"gear", DERF | KINF, 1,
                        (char*)"newton", NLINF, 0,
                        (char*)"simplex", NLINF, 0,
                        (char*)"simeq", LINF, 0,
                        (char*)"seidel", LINF, 0,
                        (char*)"_advance", KINF, 0,
                        (char*)"sparse", KINF, 0,
                        (char*)"derivimplicit", DERF, 0, /* name hard wired in deriv.c */
                        (char*)"cnexp", DERF, 0, /* see solve.c */
                        (char*)"clsoda", DERF | KINF, 1,   /* Tolerance built in to scopgear.c */
                        (char*)"after_cvode", 0, 0,
                        (char*)"cvode_t", 0, 0,
                        (char*)"cvode_t_v", 0, 0,
	                0, 0, 0
};

static char    *extdef[] = {	/* external names that can be used as doubles
				 * without giving an error message */
#include "extdef.h"
			    0
};

static char    *extdef2[] = {	/* external function names that can be used
				 * with array and function name arguments  */
#include "extdef2.h"
			    0
};

static char	*extdef3[] = {	/* function names that get two reset arguments
				 * added */
        (char*)"threshold",
        (char*)"squarewave",
        (char*)"sawtooth",
        (char*)"revsawtooth",
        (char*)"ramp",
        (char*)"pulse",
        (char*)"perpulse",
        (char*)"step",
        (char*)"perstep",
        (char*)"stepforce",
        (char*)"schedule",
	0
};

static char *extdef4[] = { /* functions that need a first arg of NrnThread* */
        (char*)"at_time",
	0
};

static char    *extdef5[] = {	/* the extdef names that are not threadsafe */
#include "extdef5.h"
			    0
};

List *constructorfunc, *destructorfunc;

void init()
{
	int             i;
	Symbol         *s;

	symbol_init();
	for (i = 0; keywords[i].name; i++) {
		s = install(keywords[i].name, keywords[i].kval);
		s->subtype = KEYWORD;
	}
	for (i = 0; methods[i].name; i++) {
		s = install(methods[i].name, METHOD);
		s->subtype = methods[i].subtype;
		s->u.i = methods[i].varstep;
	}
	for (i = 0; special[i].name; i++) {
		s = install(special[i].name, SPECIAL);
		*(special[i].p) = s;
		s->subtype = special[i].subtype;
	}
	for (i = 0; extdef[i]; i++) {
		s = install(extdef[i], NAME);
		s->subtype = EXTDEF;
	}
	for (i = 0; extdef2[i]; i++) {
		s = install(extdef2[i], NAME);
		s->subtype = EXTDEF2;
	}
	for (i = 0; extdef3[i]; i++) {
		s = lookup(extdef3[i]);
		assert(s && (s->subtype & EXTDEF));
		s->subtype |= EXTDEF3;
	}
	for (i = 0; extdef4[i]; i++) {
		s = lookup(extdef4[i]);
		assert(s && (s->subtype & EXTDEF));
		s->subtype |= EXTDEF4;
	}
	for (i = 0; extdef5[i]; i++) {
		s = lookup(extdef5[i]);
		assert(s);
		s->subtype |= EXTDEF5;
	}
	intoken = newlist();
	initfunc = newlist();
	modelfunc = newlist();
	termfunc = newlist();
	procfunc = newlist();
	initlist = newlist();
	firstlist = newlist();
	syminorder = newlist();
	plotlist = newlist();
	constructorfunc = newlist();
	destructorfunc = newlist();
#if NMODL
	nrninit();
#endif
}
