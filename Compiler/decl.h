// Function prototypes for all compiler files
// Copyright (c) 2019 Warren Toomey, GPL3

// scan.c
void reject_token(struct token *t);
int scan(struct token *t);

// tree.c
struct ASTnode *mkastnode(int op, int type,
			  struct symtable *ctype,
			  struct ASTnode *left,
			  struct ASTnode *mid,
			  struct ASTnode *right,
			  struct symtable *sym, int intvalue);
struct ASTnode *mkastleaf(int op, int type,
			  struct symtable *ctype,
			  struct symtable *sym, int intvalue);
struct ASTnode *mkastunary(int op, int type,
			   struct symtable *ctype,
			   struct ASTnode *left,
			   struct symtable *sym, int intvalue);
void dumpAST(struct ASTnode *n, int label, int level);

// gen.c
int genlabel(void);
int genAST(struct ASTnode *n, int iflabel, int looptoplabel,
	   int loopendlabel, int parentASTop);
void genpreamble(char *filename);
void genpostamble();
void genglobsym(struct symtable *node);
int genglobstr(char *strvalue, int append);
void genglobstrend(void);
int genprimsize(int type);
int genalign(int type, int offset, int direction);
void genreturn(int reg, int id);
void print_irlist(void);

// cg.c
int cgprimsize(int type);
int cgalign(int type, int offset, int direction);
void cgpreamble(char *filename);
void cgpostamble(void);
void cgenerate(void);

// expr.c
struct ASTnode *expression_list(int endtoken);
struct ASTnode *binexpr(int ptp);

// stmt.c
struct ASTnode *compound_statement(int inswitch);

// misc.c
void match(int t, char *what);
void semi(void);
void lbrace(void);
void rbrace(void);
void lparen(void);
void rparen(void);
void ident(void);
void comma(void);
void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

// sym.c
void appendsym(struct symtable **head, struct symtable **tail,
	       struct symtable *node);
struct symtable *newsym(char *name, int type, struct symtable *ctype,
			int stype, int class, int nelems, int posn);
struct symtable *addglob(char *name, int type, struct symtable *ctype,
			 int stype, int class, int nelems, int posn);
struct symtable *addlocl(char *name, int type, struct symtable *ctype,
			 int stype, int nelems);
struct symtable *addparm(char *name, int type, struct symtable *ctype,
			 int stype);
struct symtable *addstruct(char *name);
struct symtable *addunion(char *name);
struct symtable *addmemb(char *name, int type, struct symtable *ctype,
			 int stype, int nelems);
struct symtable *addenum(char *name, int class, int value);
struct symtable *addtypedef(char *name, int type, struct symtable *ctype);
struct symtable *findglob(char *s);
struct symtable *findlocl(char *s);
struct symtable *findsymbol(char *s);
struct symtable *findmember(char *s);
struct symtable *findstruct(char *s);
struct symtable *findunion(char *s);
struct symtable *findenumtype(char *s);
struct symtable *findenumval(char *s);
struct symtable *findtypedef(char *s);
void clear_symtable(void);
void freeloclsyms(void);
void freestaticsyms(void);
void dumptable(struct symtable *head, char *name, int indent);
void dumpsymtables(void);

// decl.c
int parse_type(struct symtable **ctype, int *class);
int parse_stars(int type);
int parse_cast(struct symtable **ctype);
int declaration_list(struct symtable **ctype, int class, int et1, int et2,
		     struct ASTnode **gluetree);
void global_declarations(void);

// types.c
int inttype(int type);
int ptrtype(int type);
int pointer_to(int type);
int value_at(int type);
int typesize(int type, struct symtable *ctype);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype,
			    struct symtable *rctype, int op);

// opt.c
struct ASTnode *optimise(struct ASTnode *n);
