// Structures for the intermediate code generator
// Copyright (c) 2022 Warren Toomey, GPL3

enum {
  IR_NOP,			// No instruction, can be ignored
  IR_LABEL,			// A label, not an instruction
  IR_LINENUM,			// A line number
  IR_FUNCTION,			// A non-global function
  IR_GFUNCTION,			// A global function
  IR_PARAM,			// A function's parameter
  IR_LOCAL,			// A function's local variable
  IR_JMP,			// Jump to a label
  IR_PUSH,			// Push source register on the stack
  IR_POP,			// Pop destination register from the stack
  IR_CALL,			// Call a function
  IR_ARG,			// Argument to a function call
  IR_RETURN,			// Return from a function
  IR_ASCII,			// Define an ASCII string
  IR_ENDASCII,			// End an ASCII string
  IR_DATA,			// Local data with name
  IR_GDATA,			// Global data with name
  IR_PVAL,			// A pointer value for (G)DATA
  IR_BVAL,			// A byte value for (G)DATA
  IR_WVAL,			// A word value for (G)DATA
  IR_DVAL,			// A doubleword value for (G)DATA

				// The following instructions cause the
				// type of the destination register to
				// be recorded if there is no label

  IR_LDI,			// Load register with immediate value
  IR_LD,			// Load register from memory
  IR_CEQ,			// Comparison operations
  IR_CNE,
  IR_CLT,			// Interpretation: if there is a label
  IR_CGT,			// in the instruction, we interpret
  IR_CLE,			// these as a jump if comparison is true
  IR_CGE,
  IR_CEQI,			// Compare against an immediate value
  IR_CNEI,			
  IR_MOV,			// Move between registers
  IR_ADD,
  IR_SUB,
  IR_MUL,
  IR_DIV,
  IR_MOD,
  IR_AND,
  IR_OR,
  IR_XOR,
  IR_SHL,
  IR_SHR,
  IR_ADDI,			// Add an immediate value
  IR_ST,			// Store register to memory
  IR_STDR,			// Store register to memory, dereferenced
  IR_WIDEN,			// Widen value
  IR_LDDR,			// Load register from memory, dereferenced
  IR_SHLI,			// Shift left and right with
  IR_SHRI,			// immediate values
  IR_NEG,			// Negate
  IR_INV,			// Invert
  IR_NOT,			// Logical not

				// The following instructions cause the
				// type of the destination register to
				// be recorded always

  IR_CAST,			// Label holds the old type
  IR_LDLAB,			// Load the address of a label
  IR_LEA,			// Load effective address
};


// Each IR instruction is stored in one of
// these structures. This allows us to walk
// across the list and perform some optimisations
struct irlist {
  int opcode;			// One of the instruction enums above
  int type;			// The type for this instruction
  long src;			// Register source or literal
  int dest;			// Register destination, or
  char *dstname;		// Name of destination variable
  int label;			// Jump label
};
