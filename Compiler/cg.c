#include "defs.h"
#include "data.h"
#include "decl.h"
#include "gen.h"

// Code generator for the 6809.
// Copyright (c) 2022 Warren Toomey, GPL3

// Are we in the text or data section
static int in_text= 1;

// Given a scalar type value, return the
// size of the QBE type in bytes.
int cgprimsize(int type) {
  if (ptrtype(type))
    return (2);
  switch (type) {
    case P_VOID:
    case P_NONE:
      return (0);		// XXX, could be a problem?
    case P_CHAR:
      return (1);
    case P_INT:
      return (2);
    case P_LONG:
      return (4);
    default:
      fatald("Bad type in cgprimsize:", type);
  }
  return (0);			// Keep -Wall happy
}

// Given a scalar type, an existing memory offset
// (which hasn't been allocated to anything yet)
// and a direction (1 is up, -1 is down), calculate
// and return a suitably aligned memory offset
// for this scalar type. This could be the original
// offset, or it could be above/below the original
int cgalign(int type, int offset, int direction) {

  // We don't need to do this on the 6809.
  return (offset);
}

// Print out the assembly preamble
// for one output file
void cgpreamble(char *filename) {
  int i;
  for (i=0; i<=16; i++)
    fprintf(Outfile, ".r%d\textern\n", i);
  fprintf(Outfile, "printint\textern\n");
  fprintf(Outfile, "\tsection text\n");
  in_text= 1;
}

void cgpostamble(void) {
}

// Convert the IR instructions into 6809 assembly
void cgenerate(void) {
  int i, j, k, size;
  struct irlist *n;

  // Walk the list
  n= IRlist;
  for (i=0; i< irposn; i++, n++) {

    // Do data handling opcodes in the data section
    if ((n->opcode <= IR_DVAL) && in_text) {
      fprintf(Outfile, "\tsection data\n"); in_text= 0;
    }
    if ((n->opcode > IR_DVAL) && !in_text) {
      fprintf(Outfile, "\tsection text\n"); in_text= 1;
    }

    // Do some opcodes regardless of size
    switch (n->opcode) {
      case IR_LINENUM: continue;		// Skip line numbers
      case IR_GDATA: fprintf(Outfile,"\texport %s\n", n->dstname);
      case IR_DATA:  fprintf(Outfile, "%s", n->dstname); continue;
      case IR_PVAL: fprintf(Outfile, "\tFDB 0x%lx\n", n->src); continue;
      case IR_BVAL: fprintf(Outfile, "\tFCB %ld\n", n->src); continue;
      case IR_WVAL: fprintf(Outfile, "\tFDB %ld\n", n->src); continue;
      case IR_DVAL: fprintf(Outfile, "\tFQB %ld\n", n->src); continue;
    }

    // Work out the data size for the instruction
    size= cgprimsize(n->type);

    // Convert the IR instruction into 6809 assembly.
    // We do this in size groups
    switch (size) {
      case 0: switch(n->opcode) {
        case IR_FUNCTION:  fprintf(Outfile,"%s\n", n->dstname); break;
        case IR_GFUNCTION: fprintf(Outfile,"%s\n\texport %s\n", n->dstname, n->dstname); break;
	case IR_CALL:
		// Find the index position of the last argument
		for (j=i+1; j<irposn; j++)
		  if (IRlist[j].opcode != IR_ARG) break;

		// Now work backwards and push each register onto the stack
		for (k=j-1; k!=i; k--) {
    		  size= cgprimsize(IRlist[k].type);
    		  switch (size) {
		    case 1: fprintf(Outfile,"\tlda <.r%ld\n", IRlist[k].src);
		    	    fprintf(Outfile,"\tpshs a\n"); break;
		    case 2: fprintf(Outfile,"\tldd <.r%ld\n", IRlist[k].src);
		    	    fprintf(Outfile,"\tpshs d\n"); break;
		    default: fatald("no push for size", size);
		  }
		}

		// Do the jsr
		fprintf(Outfile,"\tjsr %s\n", n->dstname); 

		// Pop the arguments from the stack. Could be more optimal!!
		for (k=j-1; k!=i; k--) {
    		  size= cgprimsize(IRlist[k].type);
    		  switch (size) {
		    case 1: fprintf(Outfile,"\tpuls a\n"); break;
		    case 2: fprintf(Outfile,"\tpuls d\n"); break;
		    default: fatald("no pull for size", size);
		  }
		}
		break;
	case IR_ARG: break;
	case IR_RETURN: fprintf(Outfile,"\trts\n"); break;
	
	default: printf("cg to handle IR opcode %d size 0\n", n->opcode);
        }
	break;





      case 1: switch(n->opcode) {
        case IR_LDI:  fprintf(Outfile,"\tlda #%ld\n", n->src);
        	      fprintf(Outfile,"\tsta <.r%d\n", n->dest); break;
        case IR_LD:   fprintf(Outfile,"\tlda %s\n", n->dstname);
        	      fprintf(Outfile,"\tsta <.r%d\n", n->dest); break;
        case IR_ST:   fprintf(Outfile,"\tlda <.r%ld\n", n->src);
		      fprintf(Outfile,"\tsta %s\n", n->dstname); break;
	case IR_ARG: break;
	default: printf("cg to handle IR opcode %d size 1\n", n->opcode);
      }
      break;

      case 2: switch(n->opcode) {
        case IR_LDI:  fprintf(Outfile,"\tldd #%ld\n", n->src);
        	      fprintf(Outfile,"\tstd <.r%d\n", n->dest); break;
        case IR_LD:   fprintf(Outfile,"\tldd %s\n", n->dstname);
        	      fprintf(Outfile,"\tstd <.r%d\n", n->dest); break;
        case IR_ST:   fprintf(Outfile,"\tldd <.r%ld\n", n->src);
		      fprintf(Outfile,"\tstd %s\n", n->dstname); break;
	case IR_CAST:
		// Get the size of the old type
    		size= cgprimsize(n->label);
		if (size!=1) fatal("Can only cast from size 1");
		fprintf(Outfile,"\tldb <.r%ld\n", n->src);
		fprintf(Outfile,"\tsex\n");
        	fprintf(Outfile,"\tstd <.r%d\n", n->dest);
		break;
	case IR_ARG: break;
	default: printf("cg to handle IR opcode %d size 2\n", n->opcode);
      }
    }
  }
}
