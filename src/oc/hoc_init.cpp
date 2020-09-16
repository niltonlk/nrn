#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/hoc_init.cpp,v 1.25 1999/11/08 17:48:58 hines Exp */

#include "hoc.h"
#include "parse.h"
#include <math.h>
#include "equation.h"

#include "ocfunc.h"

//#if defined(__cplusplus)
//extern "C" {
//#endif

extern void hoc_nrnmpi_init();

#if PVM
extern int      numprocs(), myproc(), psync();
#endif
#if 0
extern int	hoc_co();
#endif
#if	DOS || defined(WIN32) /*|| defined(MAC)*/
extern double	erf(), erfc();	/* supplied by unix */
#endif
#if defined(WIN32)
extern void hoc_winio_show(int b);
#endif

#if MAC
static double Fabs(x) double x; { return (x>0.) ? x : -x; }
static double Erf(x) double x; { return erf(x); }
static double Erfc(x) double x; { return erfc(x); }
#endif

static struct { /* Keywords */
	char	*name;
	int	kval;
} keywords[] = {
    (char*)"proc",		parsePROC,
    (char*)"func",		FUNC,
    (char*)"obfunc",	HOCOBJFUNC,
    (char*)"return",	RETURN,
    (char*)"break",	BREAK,
    (char*)"continue",	CONTINUE,
    (char*)"stop",		STOPSTMT,
    (char*)"if",		IF,
    (char*)"else",		ELSE,
    (char*)"while",	WHILE,
    (char*)"for",		FOR,
    (char*)"print",	PRINT,
    (char*)"delete",	parseDELETE,
    (char*)"read",		READ,
    (char*)"debug",	DEBUG,
    (char*)"double",	parseDOUBLE,
    (char*)"depvar",	DEPENDENT,
    (char*)"eqn",		EQUATION,
    (char*)"local",	LOCAL,
    (char*)"localobj",	LOCALOBJ,
    (char*)"strdef",	STRDEF,
    (char*)"parallel",	PARALLEL,
    (char*)"help",		HELP,
    (char*)"iterator",	ITERKEYWORD,
    (char*)"iterator_statement", ITERSTMT,
#if CABLE
    (char*)"create",	SECTIONKEYWORD,
    (char*)"connect",	CONNECTKEYWORD,
    (char*)"setpointer",	SETPOINTERKEYWORD,
    (char*)"access",	ACCESSKEYWORD,
    (char*)"insert",	INSERTKEYWORD,
    (char*)"uninsert",	UNINSERTKEYWORD,
    (char*)"forall",	FORALL,
    (char*)"ifsec",	IFSEC,
    (char*)"forsec",	FORSEC,
#endif /*CABLE*/
#if OOP
    (char*)"begintemplate", BEGINTEMPLATE,
    (char*)"endtemplate", ENDTEMPLATE,
    (char*)"objectvar",	OBJVARDECL,
    (char*)"objref",	OBJVARDECL,
    (char*)"public",	PUBLICDECL,
    (char*)"external",	EXTERNALDECL,
    (char*)"new",		NEW,
#endif
	0,		0
};
static struct {	/* Constants */
	char	*name;
	double cval;
} consts[] = {
    (char*)"PI",	3.14159265358979323846,
    (char*)"E",	2.71828182845904523536,
    (char*)"GAMMA",0.57721566490153286060,	/* Euler */
    (char*)"DEG", 57.29577951308232087680,	/* deg/radian */
    (char*)"PHI",	1.61803398874989484820,	/* golden ratio */
#if defined(LegacyFR) && LegacyFR == 1
    (char*)"FARADAY", 96485.309,	/*coulombs/mole*/
    (char*)"R", 8.31441,		/*molar gas constant, joules/mole/deg-K*/
#else
	/* Nov, 2017, from https://physics.nist.gov/cuu/Constants/index.html */
	/* also see FARADAY and gasconstant in ../nrnoc/eion.cpp */
    (char*)"FARADAY", 96485.33289,	/*coulombs/mole*/
    (char*)"R", 8.3144598,		/*molar gas constant, joules/mole/deg-K*/
#endif
	0,	0
};

static struct {	/* Built-ins */
	char	*name;
	double	(*func)(double);
} builtins[] = {
    (char*)"sin",	sin,
    (char*)"cos",	cos,
    (char*)"atan",	atan,
    (char*)"tanh",  tanh,
    (char*)"log",	Log,	/* checks argument */
    (char*)"log10", Log10,	/* checks argument */
    (char*)"exp",	hoc1_Exp,	/* checks argument */
    (char*)"sqrt",	Sqrt,	/* checks argument */
    (char*)"int",	integer,
#if MAC
    (char*)"abs",	Fabs,
    (char*)"erf",	Erf,
    (char*)"erfc",  Erfc,
#else
    (char*)"abs",	fabs,
    (char*)"erf",	erf,
    (char*)"erfc",  erfc,
#endif
	0,	0
};
static struct { /* Builtin functions with multiple or variable args */
	char 	*name;
	void	(*fun_blt)(void);
} fun_bltin[] = {
    (char*)"atan2",	    hoc_atan2,
    (char*)"system",	hoc_System,
    (char*)"prmat",	    hoc_Prmat,
    (char*)"solve",	    hoc_solve,
    (char*)"eqinit",	hoc_eqinit,
    (char*)"plt",		hoc_Plt,
    (char*)"axis",		hoc_axis,
    (char*)"plot",		hoc_Plot,
    (char*)"plotx",	    hoc_plotx,
    (char*)"ploty",	    hoc_ploty,
    (char*)"regraph",	hoc_regraph,
    (char*)"symbols",	hoc_symbols,
    (char*)"printf",	hoc_PRintf,
    (char*)"xred",		hoc_Xred,
    (char*)"sred",		hoc_Sred,
    (char*)"ropen",	    hoc_ropen,
    (char*)"wopen",	    hoc_wopen,
    (char*)"xopen",	    hoc_xopen,
    (char*)"hoc_stdout",hoc_stdout,
    (char*)"chdir",	    hoc_Chdir,
    (char*)"fprint",	hoc_Fprint,
    (char*)"fscan",	    hoc_Fscan,
    (char*)"sscanf",    hoc_sscanf,
    (char*)"sprint",	hoc_Sprint,
    (char*)"graph",	    hoc_Graph,
    (char*)"graphmode",	hoc_Graphmode,
    (char*)"fmenu",	    hoc_fmenu,
    (char*)"lw",		hoc_Lw,
    (char*)"getstr",	hoc_Getstr,
    (char*)"strcmp",	hoc_Strcmp,
    (char*)"setcolor",	hoc_Setcolor,
    (char*)"startsw",	hoc_startsw,
    (char*)"stopsw",	hoc_stopsw,
    (char*)"object_id",	hoc_object_id,
    (char*)"allobjectvars", hoc_allobjectvars,
    (char*)"allobjects",	hoc_allobjects,
    (char*)"xpanel",	hoc_xpanel,
    (char*)"xbutton",	hoc_xbutton,
    (char*)"xcheckbox",    hoc_xcheckbox,
    (char*)"xstatebutton", hoc_xstatebutton,
    (char*)"xlabel",	hoc_xlabel,
    (char*)"xmenu",	hoc_xmenu,
    (char*)"xvalue",	hoc_xvalue,
    (char*)"xpvalue",	hoc_xpvalue,
    (char*)"xradiobutton",	hoc_xradiobutton,
    (char*)"xfixedvalue",	hoc_xfixedvalue,
    (char*)"xvarlabel",	hoc_xvarlabel,
    (char*)"xslider",	hoc_xslider,
    (char*)"boolean_dialog",	hoc_boolean_dialog,
    (char*)"continue_dialog",	hoc_continue_dialog,
    (char*)"string_dialog",	hoc_string_dialog,
    (char*)"doEvents",	hoc_single_event_run,
    (char*)"doNotify",	hoc_notify_iv,
    (char*)"nrniv_bind_thread",	nrniv_bind_thread,
    (char*)"ivoc_style",	ivoc_style,
    (char*)"numarg",	hoc_Numarg,
    (char*)"argtype",	hoc_Argtype,
    (char*)"hoc_pointer_",	hoc_pointer,		/* for internal use */
    (char*)"nrn_mallinfo", hoc_mallinfo,
    (char*)"execute",	hoc_exec_cmd,
    (char*)"execute1",	hoc_execute1,
    (char*)"load_proc",	hoc_load_proc,
    (char*)"load_func",	hoc_load_func,
    (char*)"load_template", hoc_load_template,
    (char*)"load_file", hoc_load_file,
    (char*)"load_java", hoc_load_java,
    (char*)"unix_mac_pc", hoc_unix_mac_pc,
    (char*)"show_winio", hoc_show_winio,
    (char*)"nrn_load_dll", hoc_nrn_load_dll,
    (char*)"machine_name", hoc_machine_name,
    (char*)"saveaudit", hoc_Saveaudit,
    (char*)"retrieveaudit", hoc_Retrieveaudit,
    (char*)"coredump_on_error", hoc_coredump_on_error,
    (char*)"checkpoint", hoc_checkpoint,
    (char*)"quit",		 hoc_quit,
    (char*)"object_push",	hoc_object_push,
    (char*)"object_pop",	hoc_object_pop,
    (char*)"pwman_place",	hoc_pwman_place,
    (char*)"save_session", hoc_save_session,
    (char*)"print_session", hoc_print_session,
    (char*)"show_errmess_always", hoc_show_errmess_always,
    (char*)"execerror", hoc_Execerror,
    (char*)"variable_domain", hoc_Symbol_limits,
    (char*)"name_declared", hoc_name_declared,
    (char*)"use_mcell_ran4", hoc_usemcran4,
    (char*)"mcell_ran4", hoc_mcran4,
    (char*)"mcell_ran4_init", hoc_mcran4init,
    (char*)"nrn_feenableexcept", nrn_feenableexcept,
    (char*)"nrnmpi_init", hoc_nrnmpi_init,
#if PVM
        (char*)"numprocs", numprocs,
        (char*)"myproc", myproc,
        (char*)"psync", psync,
#endif
#if	DOS
    (char*)"settext",	hoc_settext,
#endif
#if defined(WIN32)
   (char*)"WinExec", hoc_win_exec,
#endif
	0,	0
};

static struct { /* functions that return a string */
	char 	*name;
	void	(*strfun_blt)(void);
} strfun_bltin[] = {
    (char*)"secname",	hoc_secname,
    (char*)"units", hoc_Symbol_units,
    (char*)"neuronhome", hoc_neuronhome,
    (char*)"getcwd",	hoc_getcwd,
    (char*)"nrnversion", hoc_nrnversion,
	0,	0
};

static struct { /* functions that return an object */
	char 	*name;
	void	(*objfun_blt)(void);
} objfun_bltin[] = {
    (char*)"object_pushed", hoc_object_pushed,
	0,	0
};

double hoc_epsilon = 1.e-11;
double hoc_ac_; /*known to the interpreter to evaluate expressions with hoc_oc() */
double* hoc_varpointer; /* executing hoc_pointer(&var) will put the address of
			the variable in this location */

double hoc_cross_x_, hoc_cross_y_; /* For Graph class in ivoc */
double hoc_default_dll_loaded_;

char* neuron_home;
char* nrn_mech_dll; /* but actually only for NEURON mswin and linux */
int nrn_noauto_dlopen_nrnmech; /* 0 except when binary special. */
int use_mcell_ran4_;
int nrn_xopen_broadcast_;

void hoc_init(void)	/* install constants and built-ins table */
{
	int i;
	Symbol *s;

	use_mcell_ran4_ = 0;
	nrn_xopen_broadcast_ = 255;
    extern void hoc_init_space(void);
	hoc_init_space();
	for (i = 0; keywords[i].name; i++)
		IGNORE(install(keywords[i].name, keywords[i].kval, 0.0, &symlist));
	for (i = 0; consts[i].name; i++) {
		s = install(consts[i].name, UNDEF, consts[i].cval, &symlist);
		s->type = VAR;
		s->u.pval = &consts[i].cval;
		s->subtype = USERDOUBLE;
	}
	for (i = 0; builtins[i].name; i++)
	{
		s = install(builtins[i].name, BLTIN, 0.0, &symlist);
		s->u.ptr = builtins[i].func;
	}
	for (i = 0; fun_bltin[i].name; i++)
	{
		s = install(fun_bltin[i].name, FUN_BLTIN, 0.0, &symlist);
		s->u.u_proc->defn.pf = fun_bltin[i].fun_blt;
		s->u.u_proc->nauto = 0;
		s->u.u_proc->nobjauto = 0;
	}
	for (i = 0; strfun_bltin[i].name; i++)
	{
		s = install(strfun_bltin[i].name, FUN_BLTIN, 0.0, &symlist);
		s->type = STRINGFUNC;
		s->u.u_proc->defn.pf = strfun_bltin[i].strfun_blt;
		s->u.u_proc->nauto = 0;
		s->u.u_proc->nobjauto = 0;
	}
	for (i = 0; objfun_bltin[i].name; i++)
	{
		s = install(objfun_bltin[i].name, FUN_BLTIN, 0.0, &symlist);
		s->type = OBJECTFUNC;
		s->u.u_proc->defn.pf = objfun_bltin[i].objfun_blt;
		s->u.u_proc->nauto = 0;
	}
	/* hoc_ac_ is a way to evaluate an expression using the interpreter */
	hoc_install_var("hoc_ac_", &hoc_ac_);
	hoc_install_var("float_epsilon", &hoc_epsilon);
	hoc_install_var("hoc_cross_x_", &hoc_cross_x_);
	hoc_install_var("hoc_cross_y_", &hoc_cross_y_);
	hoc_install_var("default_dll_loaded_", &hoc_default_dll_loaded_);

	s = install("xopen_broadcast_", UNDEF, 0.0, &hoc_symlist);
	s->type = VAR;
	s->subtype = USERINT;
	s->u.pvalint = &nrn_xopen_broadcast_;

	/* initialize pointers ( why doesn't Vax do this?) */
	hoc_access = (int *)0;
	spinit();
#if OOP
	hoc_class_registration();
	 hoc_built_in_symlist = symlist;
	 symlist = (Symlist *)0;
	 /* start symlist and top level the same list */
	 hoc_top_level_symlist = symlist = (Symlist *)emalloc(sizeof(Symlist));
	 symlist->first = symlist->last = (Symbol *)0;
	hoc_install_hoc_obj();
#endif
}

void hoc_unix_mac_pc(void) {
	hoc_ret();
#if defined(DARWIN)
	hoc_pushx(4.);
#else
#if MAC
	hoc_pushx(2.);
#else
#if defined(WIN32)
	hoc_pushx(3.);
#else
	hoc_pushx(1.);
#endif
#endif
#endif
}
void hoc_show_winio(void) {
    int b;
    b = (int)chkarg(1, 0., 1.);
#if MAC
    hoc_sioux_show(b);
#endif
#if defined(WIN32)
	hoc_winio_show(b);
#endif
    hoc_ret();
    hoc_pushx(0.);
}

int nrn_main_launch;

void hoc_nrnversion(void) {
	extern char* nrn_version(int);
	static char* p;
	int i;
	i = 1;
	if (ifarg(1)) {
		i = (int)chkarg(1, 0., 20.);
	}
	hoc_ret();
	p = nrn_version(i);
	hoc_pushstr(&p);
}

void hoc_Execerror(void) {
	char* c2 = (char*)0;
	if (ifarg(2)) {
		c2 = gargstr(2);
	}
	if (ifarg(1)) {
		hoc_execerror(gargstr(1), c2);
	}else{
		hoc_execerror_mes(c2, c2, 0);
	}
	/* never get here */
}
//
//#if defined(__cplusplus)
//}
//#endif