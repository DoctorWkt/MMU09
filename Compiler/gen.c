#include "defs.h"
#include "data.h"
#include "decl.h"
#include "gen.h"

// Code generator for a generic two
// operand machine, i.e. an intermediate
// machine code representation.
// Copyright (c) 2019-2022 Warren Toomey, GPL3

#define NUMFREEREGS 16
static int freeregs[NUMFREEREGS];
static int regtype[NUMFREEREGS];

// Free all registers, except the one
// given in the argument
static void irfreeregs(int reg) {
  int i;
  for (i=0; i< NUMFREEREGS; i++) {
    if (i== reg) continue;
    freeregs[i]= 1;
    regtype[i]= P_NONE;
  }
}

// Free one register
static void freereg(int r) {
  if (freeregs[r] != 0)
    fatald("Error trying to free register", r);
  freeregs[r]= 1;
  regtype[r]= P_NONE;
}

// Allocate a free register. Return the number of
// the register. Die if no available registers.
int allocreg(void) {
  int reg;

  for (reg = 0; reg < NUMFREEREGS; reg++) {
    if (freeregs[reg]) {
      freeregs[reg] = 0;
      return (reg);
    }
  }
  fatal("Out of IR registers");
  return(0);		// Keep -Wall happy
}

// Generate and return a new label number
static int labelid = 1;
int genlabel(void) {
  return (labelid++);
}

// List of IR instructions for one AST tree.
// Position of next empty slot.
// Total number of elements in the list.
struct irlist *IRlist=NULL;
int irposn=0;
int irsize=0;

// Add an instruction to the IR list.
// Return the destination register value.
int add_instruction(int opcode, int type, int dest,
                    long src, char *dstname, int label) {
  struct irlist *this;

  // The list is full and either needs to be created or expanded
  if (irposn==irsize) {
    if (IRlist==NULL)
      irsize= 40;	// An arbitrary number of elements to start with,
    else		// and double each time we run out of elements
      irsize= irsize + irsize;
    IRlist= (struct irlist *)realloc((struct irlist *)IRlist,
                                irsize * sizeof(struct irlist));
    if (IRlist==NULL) fatal("out of IR memory");
  }

  // Save the values
  this= &IRlist[irposn];
  this->opcode=     opcode;
  this->type=       type;
  this->dest=       dest;
  this->src=        src;
  this->dstname=    dstname;
  this->label=      label;

  // If this instruction alters a destination register, then
  // record the type of the register.
  if ((opcode >= IR_MOV) || ((opcode >= IR_CEQ) && (label==0)))
    regtype[dest]= type;
  irposn++;
  return(dest);
}

static void update_line(struct ASTnode *n) {
  // Output the line into the IR list if we've
  // changed the line number in the AST node
  if (n->linenum != 0 && Line != n->linenum) {
    Line = n->linenum;
    add_instruction(IR_LINENUM, 0, Line, 0, NULL, 0);
  }
}

// List of inverted comparison instructions
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static int invcmplist[] = { IR_CNE, IR_CEQ, IR_CGE, IR_CLE, IR_CGT, IR_CLT };

// Compare two registers and jump if false.
static void ircompare_and_jump(int ASTop, int r1, int r2, int label, int type) {

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in compare_and_jump()");

  add_instruction(invcmplist[ASTop - A_EQ], type, r1, r2, NULL, label);
  freereg(r1);
  freereg(r2);
}

// List of comparison instructions,
// in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static int cmplist[] = { IR_CEQ, IR_CNE, IR_CLT, IR_CGT, IR_CLE, IR_CGE };

// Compare two temporaries and set if true.
int ircompare_and_set(int ASTop, int r1, int r2, int type) {

  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in ircompare_and_set()");

  add_instruction(cmplist[ASTop - A_EQ], type, r1, r2, NULL, 0);
  freereg(r2);
  return (r1);
}

// Convert an integer value to a boolean value. Jump if
// it's non-zero on an IF, WHILE, LOGAND or LOGOR operation
static int irboolean(int r, int op, int label, int type) {
  switch (op) {
  case A_IF:
  case A_WHILE:
  case A_LOGAND:
    add_instruction(IR_CNEI, type, r, 0, NULL, label);
    break;
  case A_LOGOR:
    add_instruction(IR_CEQI, type, r, 0, NULL, label);
    break;
  default:
    add_instruction(IR_CNEI, type, r, 0, NULL, 0);
  }
  return (r);
}

// Load a value from a variable into a register.
// Return the number of the register. If the
// operation is pre- or post-increment/decrement,
// also perform this action.
static int irloadvar(struct symtable *sym, int op) {
  int r, r2, offset = 1;

  // Get a register for the result
  r = allocreg();

  // If the symbol is a pointer, use the size
  // of the type that it points to as any
  // increment or decrement. If not, it's one.
  if (ptrtype(sym->type))
    offset = typesize(value_at(sym->type), sym->ctype);

  // Negate the offset for decrements
  if (op == A_PREDEC || op == A_POSTDEC)
    offset = -offset;

  // Get the value into the register
  add_instruction(IR_LD, sym->type, r, 0, sym->name, 0);

  // If we have a pre-operation, 
  // increment it and save it back
  if (op == A_PREINC || op == A_PREDEC) {
    add_instruction(IR_ADDI, sym->type, r, offset, NULL, 0);
    add_instruction(IR_ST, sym->type, 0, r, sym->name, 0);
  }

  // If we have a post-operation, 
  // copy the value to a second register,
  // increment it and save it back
  if (op == A_PREINC || op == A_PREDEC) {
    r2 = allocreg();
    add_instruction(IR_MOV, sym->type, r2, r, NULL, 0);
    add_instruction(IR_ADDI, sym->type, r2, offset, NULL, 0);
    add_instruction(IR_ST, sym->type, 0, r2, sym->name, 0);
  }

  return(r);
}

// Push any in-use registers on the stack
static void irpushregs(void) {
  int reg;

  for (reg = 0; reg < NUMFREEREGS; reg++) {
    if (freeregs[reg]==0)
      add_instruction(IR_PUSH, regtype[reg], 0, reg, NULL, 0);
  }
}

// Pop any in-use registers from the stack
// Don't pop the named register as we just
// allocated it
static void irpopregs(int notreg) {
  int reg;

  for (reg = NUMFREEREGS-1; reg >= 0; reg--) {
    if ((freeregs[reg]==0) && (reg != notreg))
      add_instruction(IR_POP, regtype[reg], reg, 0, NULL, 0);
  }
}

// Generate a two-operand instruction.
// Free the second register and return
// the identity of the first register
static int irbinop(int op, int leftreg, int rightreg, int type) {
   add_instruction(op, type, leftreg, rightreg, NULL, 0);
   freereg(rightreg);
   return(leftreg);
}

// Generate a function's preamble
static void irfuncpreamble(struct symtable *sym) {
  int op= IR_FUNCTION;
  struct symtable *parm, *locvar;

  // Output the function's name and return type
  if (sym->class == C_GLOBAL)
    op= IR_GFUNCTION;
  add_instruction(op, sym->type, 0, 0, sym->name, 0);

  // Output the parameter names and types
  for (parm = sym->member; parm != NULL; parm = parm->next)
    add_instruction(IR_PARAM, parm->type, 0, 0, parm->name, 0);

  // Output the local names and types
  for (locvar = Loclhead; locvar != NULL; locvar = locvar->next)
    add_instruction(IR_LOCAL, locvar->type, 0,
 		    locvar->size * locvar->nelems, locvar->name, 0);

  // Ensure all registers are free
  irfreeregs(NOREG);
}

// Generate a functions' postamble
static void irfuncpostamble(struct symtable *sym) {

  // Output the return instruction
  add_instruction(IR_RETURN, 0, 0, 0, NULL, 0);
}

// Generate the code for an IF statement
// and an optional ELSE clause.
static int genIF(struct ASTnode *n, int looptoplabel, int loopendlabel) {
  int Lfalse, Lend = 0;
  int r, r2;

  // Generate two labels: one for the
  // false compound statement, and one
  // for the end of the overall IF statement.
  // When there is no ELSE clause, Lfalse _is_
  // the ending label!
  Lfalse = genlabel();
  if (n->right)
    Lend = genlabel();

  // Generate the condition code
  r = genAST(n->left, Lfalse, NOLABEL, NOLABEL, n->op);
  // Set r2 to 1
  // Test to see if the condition is true. If not, jump to the false label
  r2 = add_instruction(IR_LDI, P_INT, allocreg(), 1, NULL, 0);
  ircompare_and_jump(A_EQ, r, r2, Lfalse, P_INT);
  irfreeregs(NOREG);

  // Generate the true compound statement
  genAST(n->mid, NOLABEL, looptoplabel, loopendlabel, n->op);
  irfreeregs(NOREG);

  // If there is an optional ELSE clause,
  // generate the jump to skip to the end
  if (n->right) {
    // QBE doesn't like two jump instructions in a row, and
    // a break at the end of a true IF section causes this. The
    // solution is to insert a label before the IF jump.
    add_instruction(IR_LABEL, 0, 0, 0, NULL, genlabel());
    add_instruction(IR_JMP, 0, 0, 0, NULL, Lend);
  }
  // Now the false label
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lfalse);

  // Optional ELSE clause: generate the
  // false compound statement and the
  // end label
  if (n->right) {
    genAST(n->right, NOLABEL, NOLABEL, loopendlabel, n->op);
    irfreeregs(NOREG);
    add_instruction(IR_LABEL, 0, 0, 0, NULL, Lend);
  }

  return (NOREG);
}

// Generate the code for a WHILE statement
static int genWHILE(struct ASTnode *n) {
  int Lstart, Lend;
  int r, r2;

  // Generate the start and end labels
  // and output the start label
  Lstart = genlabel();
  Lend = genlabel();
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lstart);

  // Generate the condition code
  r = genAST(n->left, Lend, Lstart, Lend, n->op);
  // Test to see if the condition is true. If not, jump to the end label
  r2 = add_instruction(IR_LDI, P_INT, allocreg(), 1, NULL, 0);
  ircompare_and_jump(A_EQ, r, r2, Lend, P_INT);
  irfreeregs(NOREG);

  // Generate the compound statement for the body
  genAST(n->right, NOLABEL, Lstart, Lend, n->op);
  irfreeregs(NOREG);

  // Finally output the jump back to the condition,
  // and the end label
  add_instruction(IR_JMP, 0, 0, 0, NULL, Lstart);
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lend);
  return (NOREG);
}

// Generate the code for a SWITCH statement
static int genSWITCH(struct ASTnode *n) {
  int *caselabel;
  int Lend;
  int Lcode = 0;
  int i, reg, r2, type;
  struct ASTnode *c;

  // Create an array for the case labels
  caselabel = (int *) malloc((n->a_intvalue + 1) * sizeof(int));
  if (caselabel == NULL)
    fatal("malloc failed in genSWITCH");

  // For now
  // we simply evaluate the switch condition and
  // then do successive comparisons and jumps,
  // just like we were doing successive if/elses

  // Generate a label for the end of the switch statement.
  Lend = genlabel();

  // Generate labels for each case. Put the end label in
  // as the entry after all the cases
  for (i = 0, c = n->right; c != NULL; i++, c = c->right)
    caselabel[i] = genlabel();
  caselabel[i] = Lend;

  // Output the code to calculate the switch condition
  reg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->left->type;
  irfreeregs(NOREG);

  // Walk the right-child linked list to
  // generate the code for each case
  for (i = 0, c = n->right; c != NULL; i++, c = c->right) {

    // Generate a label for the actual code that the cases will fall down to
    if (Lcode == 0)
      Lcode = genlabel();

    // Output the label for this case's test
    add_instruction(IR_LABEL, 0, 0, 0, NULL, caselabel[i]);

    // Do the comparison and jump, but not if it's the default case
    if (c->op != A_DEFAULT) {
      // Jump to the next case if the value doesn't match the case value
      r2 = add_instruction(IR_LDI, type, allocreg(), c->a_intvalue, NULL, 0);
      ircompare_and_jump(A_EQ, reg, r2, caselabel[i + 1], type);

      // Otherwise, jump to the code to handle this case
      add_instruction(IR_JMP, 0, 0, 0, NULL, Lcode);
    }
    // Generate the case code. Pass in the end label for the breaks.
    // If case has no body, we will fall into the following body.
    // Reset Lcode so we will create a new code label on the next loop.
    if (c->left) {
      add_instruction(IR_LABEL, 0, 0, 0, NULL, Lcode);
      genAST(c->left, NOLABEL, NOLABEL, Lend, 0);
      irfreeregs(NOREG);
      Lcode = 0;
    }
  }

  // Now output the end label.
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lend);
  free(caselabel);
  return (NOREG);
}

// Generate the code for an
// A_LOGAND or A_LOGOR operation
static int gen_logandor(struct ASTnode *n) {
  // Generate two labels
  int Lfalse = genlabel();
  int Lend = genlabel();
  int reg;
  int type;

  // Generate the code for the left expression
  // followed by the jump to the false label
  reg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->left->type;
  irboolean(reg, n->op, Lfalse, type);

  // Generate the code for the right expression
  // followed by the jump to the false label
  reg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, 0);
  type = n->right->type;
  irboolean(reg, n->op, Lfalse, type);
  irfreeregs(NOREG);

  // We didn't jump so set the right boolean value
  if (n->op == A_LOGAND) {
    add_instruction(IR_LDI, type, reg, 1, NULL, 0);
    add_instruction(IR_JMP, 0, 0, 0, NULL, Lend);
    add_instruction(IR_LABEL, 0, 0, 0, NULL, Lfalse);
    add_instruction(IR_LDI, type, reg, 0, NULL, 0);
  } else {
    add_instruction(IR_LDI, type, reg, 0, NULL, 0);
    add_instruction(IR_JMP, 0, 0, 0, NULL, Lend);
    add_instruction(IR_LABEL, 0, 0, 0, NULL, Lfalse);
    add_instruction(IR_LDI, type, reg, 1, NULL, 0);
  }
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lend);
  return (reg);
}

// Generate the code to calculate the arguments of a
// function call, then call the function with these
// arguments. Return the temoprary that holds
// the function's return value.
static int gen_funccall(struct ASTnode *n) {
  struct ASTnode *gluetree = n->left;
  struct symtable **paramlist;
  struct symtable *p;
  int i,reg;
  int numargs = 0;
  int *arglist = NULL;

  // Determine the actual number of arguments
  for (gluetree = n->left; gluetree != NULL; gluetree = gluetree->left) {
    numargs++;
  }

  // Check if the argument count matches the param count
  if (numargs != n->sym->nelems)
    fatald("Incorrect number of arguments in function call", numargs);

  // Allocate an array to record which registers hold the arguments
  if (numargs != 0) {
    arglist = (int *) malloc(numargs * sizeof(int));
    if (arglist == NULL)
      fatal("arglist malloc failed in gen_funccall");

    // Also allocate an array of parameter symbols. We do this because
    // the param linked list is left to right, but the argument list
    // is right to left.
    paramlist = (struct symtable **) malloc(numargs * sizeof(struct symtable *));
    if (paramlist == NULL)
      fatal("paramlist malloc failed in gen_funccall");

    // Copy the parameter pointers into the array
    for (i=0, p= n->sym->member; i< numargs; i++, p= p->next)
      paramlist[i]= p;
  }

  // Push all the in-use registers on the stack
  irpushregs();

  // Calculate the expression's value for any arguments
  for (i=numargs-1, gluetree= n->left; gluetree != NULL; gluetree= gluetree->left, i--) {

    // Slightly dirty hack: change any INTLIT type to be the same type
    // as the function's parameter before we evaluate it
    if (gluetree->right->op == A_INTLIT) {
      gluetree->right->type= paramlist[i]->type;
    }
    reg = genAST(gluetree->right, NOLABEL, NOLABEL, NOLABEL, gluetree->op);
    arglist[i]= reg;
  }

  // Call the function
  add_instruction(IR_CALL, 0, 0, 0, n->sym->name, 0);

  // List the arguments to the call in order
  for (i= 0; i< numargs; i++) {

    // Store the parameter type in the IR label 
    add_instruction(IR_ARG, regtype[arglist[i]], 0, arglist[i], NULL,
	paramlist[i]->type);
    // Mark the register argument as not in use
    freereg(arglist[i]);
  }

  // If non-void, allocate a register and copy the return value from r0
  if (n->sym->type != P_VOID) {
    // If r0 is free, just mark it as in-use
    if (freeregs[0]==1) {
      reg= 0;
      freeregs[0] = 0;
      regtype[0]= n->sym->type;
    } else {
      reg= allocreg();
      add_instruction(IR_MOV, n->sym->type, reg, 0, NULL, 0);
    }
  }
  
  // Pop the in-use registers from the stack and return the result
  irpopregs(reg);
  if (arglist) free(arglist);
  if (paramlist) free(paramlist);
  return(reg);
}

// Generate code for a ternary expression
static int gen_ternary(struct ASTnode *n) {
  int Lfalse, Lend;
  int reg, expreg;
  int r, r2;

  // Generate two labels: one for the
  // false expression, and one for the
  // end of the overall expression
  Lfalse = genlabel();
  Lend = genlabel();

  // Generate the condition code
  r = genAST(n->left, Lfalse, NOLABEL, NOLABEL, n->op);
  // Test to see if the condition is true. If not, jump to the false label
  r2 = add_instruction(IR_LDI, P_INT, allocreg(), 1, NULL, 0);
  ircompare_and_jump(A_EQ, r, r2, Lfalse, P_INT);

  // Get a temporary to hold the result of the two expressions
  reg = allocreg();

  // Generate the true expression and the false label.
  // Move the expression result into the known temporary.
  expreg = genAST(n->mid, NOLABEL, NOLABEL, NOLABEL, n->op);
  add_instruction(IR_MOV, n->mid->type, reg, expreg, NULL, 0);
  freereg(expreg);
  add_instruction(IR_JMP, 0, 0, 0, NULL, Lend);
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lfalse);

  // Generate the false expression and the end label.
  // Move the expression result into the known temporary.
  expreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, n->op);
  add_instruction(IR_MOV, n->mid->type, reg, expreg, NULL, 0);
  freereg(expreg);
  add_instruction(IR_LABEL, 0, 0, 0, NULL, Lend);
  return (reg);
}

// Given an AST, an optional label, and the AST op
// of the parent, generate assembly code recursively.
// Return the temporary id with the tree's final value.
int genAST(struct ASTnode *n, int iflabel, int looptoplabel,
	   int loopendlabel, int parentASTop) {
  int leftreg = NOREG, rightreg = NOREG;
  int lefttype = P_VOID, type = P_VOID;
  struct symtable *leftsym = NULL;

  // Empty tree, do nothing
  if (n == NULL)
    return (NOREG);

  // Update the line number in the output
  update_line(n);

  // We have some specific AST node handling at the top
  // so that we don't evaluate the child sub-trees immediately
  switch (n->op) {
    case A_IF:
      return (genIF(n, looptoplabel, loopendlabel));
    case A_WHILE:
      return (genWHILE(n));
    case A_SWITCH:
      return (genSWITCH(n));
    case A_FUNCCALL:
      return (gen_funccall(n));
    case A_TERNARY:
      return (gen_ternary(n));
    case A_LOGOR:
      return (gen_logandor(n));
    case A_LOGAND:
      return (gen_logandor(n));
    case A_GLUE:
      // Do each child statement, and free the
      // temporaries after each child
      if (n->left != NULL)
	genAST(n->left, iflabel, looptoplabel, loopendlabel, n->op);
      irfreeregs(NOREG);
      if (n->right != NULL)
	genAST(n->right, iflabel, looptoplabel, loopendlabel, n->op);
      irfreeregs(NOREG);
      return (NOREG);
    case A_FUNCTION:
      // Generate the function's preamble before the code
      // in the child sub-tree
      irfuncpreamble(n->sym);
      genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
      irfuncpostamble(n->sym);
      return (NOREG);
  }

  // General AST node handling below

  // Get the left and right sub-tree values. Also get the type
  if (n->left) {
    lefttype = type = n->left->type;
    leftsym = n->left->sym;
    leftreg = genAST(n->left, NOLABEL, NOLABEL, NOLABEL, n->op);
  }

  if (n->right) {
    type = n->right->type;
    rightreg = genAST(n->right, NOLABEL, NOLABEL, NOLABEL, n->op);
  }

  switch (n->op) {
    case A_ADD:
      return(irbinop(IR_ADD, leftreg, rightreg, type));
    case A_SUBTRACT:
      return(irbinop(IR_SUB, leftreg, rightreg, type));
    case A_MULTIPLY:
      return(irbinop(IR_MUL, leftreg, rightreg, type));
    case A_DIVIDE:
      return(irbinop(IR_DIV, leftreg, rightreg, type));
    case A_MOD:
      return(irbinop(IR_MOD, leftreg, rightreg, type));
    case A_AND:
      return(irbinop(IR_AND, leftreg, rightreg, type));
    case A_OR:
      return(irbinop(IR_OR, leftreg, rightreg, type));
    case A_XOR:
      return(irbinop(IR_XOR, leftreg, rightreg, type));
    case A_LSHIFT:
      return(irbinop(IR_SHL, leftreg, rightreg, type));
    case A_RSHIFT:
      return(irbinop(IR_SHR, leftreg, rightreg, type));
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      return (ircompare_and_set(n->op, leftreg, rightreg, lefttype));
    case A_INTLIT:
      return (add_instruction(IR_LDI, n->type, allocreg(),
			      n->a_intvalue, NULL, 0));
    case A_STRLIT:
      return (add_instruction(IR_LDLAB, n->type, allocreg(),
			      0, NULL, n->a_intvalue));
    case A_IDENT:
      // Load our value if we are an rvalue
      // or we are being dereferenced
      if (n->rvalue || parentASTop == A_DEREF) {
	return (irloadvar(n->sym, n->op));
      } else
	return (NOREG);
    case A_ASPLUS:
    case A_ASMINUS:
    case A_ASSTAR:
    case A_ASSLASH:
    case A_ASMOD:
    case A_ASSIGN:

      // For the '+=' and friends operators, generate suitable code
      // and get the temporary with the result. Then take the left child,
      // make it the right child so that we can fall into the assignment code.
      switch (n->op) {
	case A_ASPLUS:
          irbinop(IR_ADD, leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASMINUS:
          irbinop(IR_SUB, leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASSTAR:
          irbinop(IR_MUL, leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASSLASH:
          irbinop(IR_DIV, leftreg, rightreg, type);
	  n->right = n->left;
	  break;
	case A_ASMOD:
          irbinop(IR_MOD, leftreg, rightreg, type);
	  n->right = n->left;
	  break;
      }

      // Now into the assignment code
      // Are we assigning to an identifier or through a pointer?
      switch (n->right->op) {
	case A_IDENT:
	  if (n->right->sym->class == C_GLOBAL ||
	      n->right->sym->class == C_EXTERN ||
	      n->right->sym->class == C_STATIC)
    	    return(add_instruction(IR_ST, n->right->sym->type, 0,
				   leftreg, n->right->sym->name, 0));
	  else
	    return (add_instruction(IR_ST, n->right->sym->type, 0, 
                                   leftreg, n->right->sym->name, 0));
	case A_DEREF:
	  return (add_instruction(IR_STDR, n->right->sym->type, leftreg,
                                   rightreg, NULL, 0));
	default:
	  fatald("Can't A_ASSIGN in genAST(), op", n->op);
      }
    case A_WIDEN:
      // Widen the child's type to the parent's type
      return (add_instruction(IR_WIDEN, n->type, leftreg, rightreg, NULL, 0));
    case A_RETURN:
      add_instruction(IR_RETURN, n->type, leftreg, 0, NULL, 0);
      return (NOREG);
    case A_ADDR:
      // If we have a symbol, get its address. Otherwise,
      // the left temporary already has the address because
      // it's a member access
      if (n->sym != NULL)
        return(add_instruction(IR_LEA, n->type, leftreg, 0, n->sym->name, 0));
      else
	return (leftreg);
    case A_DEREF:
      // If we are an rvalue, dereference to get the value we point at,
      // otherwise leave it for A_ASSIGN to store through the pointer
      if (n->rvalue)
        return(add_instruction(IR_LEA, lefttype, leftreg, leftreg, NULL, 0));
      else
	return (leftreg);
    case A_SCALE:
      // Small optimisation: use shift if the
      // scale value is a known power of two
      switch (n->a_size) {
	case 2:
          return(irbinop(IR_SHLI, leftreg, 1, type));
	case 4:
          return(irbinop(IR_SHLI, leftreg, 2, type));
	case 8:
          return(irbinop(IR_SHLI, leftreg, 3, type));
	default:
	  // Load a temporary with the size and
	  // multiply the leftreg by this size
  	  rightreg = add_instruction(IR_LDI, P_INT, allocreg(),
				     n->a_size, NULL, 0);
          return(irbinop(IR_MUL, leftreg, rightreg, type));
      }
    case A_POSTINC:
    case A_POSTDEC:
      // Load and decrement the variable's value into a temporary
      // and post increment/decrement it
      return (irloadvar(n->sym, n->op));
    case A_PREINC:
    case A_PREDEC:
      // Load and decrement the variable's value into a temporary
      // and pre increment/decrement it
      return (irloadvar(leftsym, n->op));
    case A_NEGATE:
      return(irbinop(IR_NEG, leftreg, leftreg, type));
    case A_INVERT:
      return(irbinop(IR_INV, leftreg, leftreg, type));
    case A_LOGNOT:
      return(irbinop(IR_NOT, leftreg, leftreg, type));
    case A_TOBOOL:
      // If the parent AST node is an A_IF or A_WHILE, generate
      // a compare followed by a jump. Otherwise, set the temporary
      // to 0 or 1 based on it's zeroeness or non-zeroeness
      return (irboolean(leftreg, parentASTop, iflabel, type));
    case A_BREAK:
      add_instruction(IR_JMP, 0, 0, 0, NULL, loopendlabel);
      return (NOREG);
    case A_CONTINUE:
      add_instruction(IR_JMP, 0, 0, 0, NULL, looptoplabel);
      return (NOREG);
    case A_CAST:
      return(add_instruction(IR_CAST, n->type, leftreg, 0, NULL, lefttype));
    default:
      fatald("Unknown AST operator", n->op);
  }
  return (NOREG);		// Keep -Wall happy
}

// Generate the preamble for an input file
void genpreamble(char *filename) {
  cgpreamble(filename);
}

// Generate the postamble for an input file
void genpostamble() {
  cgpostamble();
}

// Generate a global symbol but not functions
void genglobsym(struct symtable *node) {
  int size, type;
  int initvalue;
  int i;
  int op= IR_DATA;

  if (node == NULL)
    return;
  if (node->stype == S_FUNCTION)
    return;

  // Get the size of the variable (or its elements if an array)
  // and the type of the variable
  if (node->stype == S_ARRAY) {
    size = typesize(value_at(node->type), node->ctype);
    type = value_at(node->type);
  } else {
    size = node->size;
    type = node->type;
  }

  // Output the identity and if it is globally visible.
  // Label holds the size
  if (node->class == C_GLOBAL)
    op= IR_GDATA;
  add_instruction(op, type, 0, 0, node->name, size);

  // Output space for one or more elements
  for (i = 0; i < node->nelems; i++) {

    // Get any initial value
    initvalue = 0;
    if (node->initlist != NULL)
      initvalue = node->initlist[i];

    // Generate the space for this type
    if (ptrtype(type))
      op= IR_PVAL;
    else switch(type) {
      case P_CHAR: op= IR_BVAL; break;
      case P_INT:  op= IR_WVAL; break;
      case P_LONG: op= IR_DVAL; break;
      default: fatal("Bad type in genglobsym");
    }
    add_instruction(op, 0, initvalue, 0, NULL, 0);
  }
}

// Generate a global string.
// If append is true, append to
// previous genglobstr() call.
int genglobstr(char *strvalue, int append) {
  int l=0;
  if (append==0) {
    l= genlabel();
    add_instruction(IR_LABEL, 0, 0, 0, NULL, l);
  }
  add_instruction(IR_ASCII, 0, 0, 0, strvalue, 0);
  return (l);
}

void genglobstrend(void) {
  add_instruction(IR_ENDASCII, 0, 0, 0, NULL, 0);
}

int genprimsize(int type) {
  return (cgprimsize(type));
}

int genalign(int type, int offset, int direction) {
  return (cgalign(type, offset, direction));
}

// List of IR instruction opcodes
static char *iropname[] = {
  "ascii", "endascii", "data", "gdata", "pval", "bval", "wval", "dval",
  "nop", "label", "linenum", "function", "gfunction", "param",
  "local", "jmp", "push", "pop", "call", "arg", "ret",
  "ceq", "cne", "clt", "cgt", "cle", "cge", "ceqi", "cnei",
  "mov", "add", "sub", "mul", "div", "mod", "and", "or",
  "xor", "shl", "shr", "addi", "st", "stdr", "widen", "lddr", "shli",
  "shri", "neg", "inv", "not", "cast", "ldlab", "lea", "ldi", "ld"
};

static char irtype(int type) {
   char c;
    // Work out the instruction's type
    if (ptrtype(type))
      c= 'p';
    else switch(type) {
      case P_NONE: c= '!'; break;
      case P_CHAR: c= 'b'; break;
      case P_INT:  c= 'w'; break;
      case P_LONG: c= 'd'; break;
      default: c= '?';
    }
  return(c);
}

// Print the list of IR instructions
void print_irlist(void) {
  int i;
  char type;
  struct irlist *n;

  // Walk the list
  n= IRlist;
  for (i=0; i< irposn; i++, n++) {

    // Work out the instruction's type
    type= irtype(n->type);

    // Decode and print that instruction
    switch(n->opcode) {
      case IR_LINENUM: break;
      case IR_FUNCTION: printf("function %s\n", n->dstname); break;
      case IR_GFUNCTION: printf("global function %s\n", n->dstname); break;
      case IR_LABEL: printf("L%d\n", n->label); break;
      case IR_LDI: printf("     ldi%c\tr%d, %ld\n", type,n->dest, n->src); break;
      case IR_ST: printf("     st%c\tr%ld, %s\n",type,n->src, n->dstname); break;
      case IR_LD: printf("     ld%c\tr%d, %s\n",type,n->dest, n->dstname); break;
      case IR_RETURN: if (n->type== P_NONE)
			printf("     ret\n");
		      else
			printf("     ret%c\tr%ld\n",type,n->src);
	 	      break;
      case IR_PUSH: printf("     push%c\tr%ld\n",type,n->src); break;
      case IR_POP: printf("     pop%c\tr%d\n",type,n->dest); break;
      case IR_ASCII: printf("     ASCII \"%s\"\n", n->dstname); break;
      case IR_ENDASCII: printf("     ASCIIZ\n"); break;
      case IR_DATA: printf("%s\tdata%c %d\n", n->dstname, type, n->label); break;
      case IR_GDATA: printf("%s\tgdata%c %d\n", n->dstname, type, n->label); break;
      case IR_PVAL: printf("     pval\t%ld\n", n->src); break;
      case IR_BVAL: printf("     bval\t%ld\n", n->src); break;
      case IR_WVAL: printf("     wval\t%ld\n", n->src); break;
      case IR_DVAL: printf("     dval\t%ld\n", n->src); break;
      case IR_LDLAB: printf("     lea\tr%d, L%d\n",n->dest, n->label); break;
      case IR_CALL: printf("     call\t%s\n", n->dstname); break;
      case IR_ARG: printf("     arg%c\tr%ld (%c)\n", type, n->src, irtype(n->label)); break;
      case IR_JMP: printf("     jmp\tL%d\n", n->label); break;

      case IR_CEQ: 
      case IR_CNE: 
      case IR_CLT: 
      case IR_CGT:
      case IR_CLE:
      case IR_CGE: if (n->label)
      		     printf("     %s%c\tr%d, r%ld, L%d\n", iropname[n->opcode], type,
			n->dest, n->src, n->label);
		   else
      		     printf("     %s%c\tr%d, r%ld\n", iropname[n->opcode], type,
			n->dest, n->src);
		   break;

      default: printf("     %s%c\tr%d, r%ld\n", iropname[n->opcode], type,
			n->dest, n->src);
    }
  }
}
