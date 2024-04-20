/* Lama SM Bytecode interpreter */

# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>
# include "../runtime/runtime.h"
# include "common.h"

void *__start_custom_data;
void *__stop_custom_data;

/* Disassembles the bytecode pool */
void disassemble (FILE *f, bytefile *bf) {
  
  uint8_t *ip     = bf->code_ptr;
  do {
    uint8_t x = *ip++,
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

    fprintf (f, "0x%.8x:\t", ip-bf->code_ptr-1);
    
    switch (h) {
    case 15:
      goto stop;

    case BINOP:
      fprintf (f, "BINOP\t%s", ops[l-1]);
      break;
      
    case H1_OPS:
      switch (l) {
      case CONST:
        fprintf (f, "CONST\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case BSTRING:
        fprintf (f, "STRING\t%s", get_string (bf, (ip += sizeof (int), *(int*)(ip - sizeof (int)))));
        break;
          
      case BSEXP:
        fprintf (f, "SEXP\t%s ", get_string (bf, (ip += sizeof (int), *(int*)(ip - sizeof (int)))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case STI:
        fprintf (f, "STI");
        break;
        
      case STA:
        fprintf (f, "STA");
        break;
        
      case JMP:
        fprintf (f, "JMP\t0x%.8x", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case END:
        fprintf (f, "END");
        break;
        
      case RET:
        fprintf (f, "RET");
        break;
        
      case DROP:
        fprintf (f, "DROP");
        break;
        
      case DUP:
        fprintf (f, "DUP");
        break;
        
      case SWAP:
        fprintf (f, "SWAP");
        break;

      case ELEM:
        fprintf (f, "ELEM");
        break;
        
      default:
        failure ("ERROR: invalid opcode %d-%d\n", h, l);
      }
      break;
      
    case LD:
    case LDA:
    case ST:
      fprintf (f, "%s\t", lds[h-2]);
      switch (l) {
      case 0: fprintf (f, "G(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
      case 1: fprintf (f, "L(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
      case 2: fprintf (f, "A(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
      case 3: fprintf (f, "C(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
      default: failure ("ERROR: invalid opcode %d-%d\n", h, l);
      }
      break;
      
    case H5_OPS:
      switch (l) {
      case CJMPZ:
        fprintf (f, "CJMPz\t0x%.8x", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case CJMPNZ:
        fprintf (f, "CJMPnz\t0x%.8x", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case BEGIN:
        fprintf (f, "BEGIN\t%d ", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case CBEGIN:
        fprintf (f, "CBEGIN\t%d ", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case BCLOSURE:
        fprintf (f, "CLOSURE\t0x%.8x", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        {int n = (ip += sizeof (int), *(int*)(ip - sizeof (int)));
         for (int i = 0; i<n; i++) {
         switch (*ip++) {
           case 0: fprintf (f, "G(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
           case 1: fprintf (f, "L(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
           case 2: fprintf (f, "A(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
           case 3: fprintf (f, "C(%d)", (ip += sizeof (int), *(int*)(ip - sizeof (int)))); break;
           default: failure ("ERROR: invalid opcode %d-%d\n", h, l);
         }
         }
        };
        break;
          
      case CALLC:
        fprintf (f, "CALLC\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case CALL:
        fprintf (f, "CALL\t0x%.8x ", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case TAG:
        fprintf (f, "TAG\t%s ", get_string (bf, (ip += sizeof (int), *(int*)(ip - sizeof (int)))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case ARRAY_KEY:
        fprintf (f, "ARRAY\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case  9:
        fprintf (f, "FAIL\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        fprintf (f, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;
        
      case LINE:
        fprintf (f, "LINE\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;

      default:
        failure ("ERROR: invalid opcode %d-%d\n", h, l);
      }
      break;
      
    case PATT:
      fprintf (f, "PATT\t%s", pats[l]);
      break;

    case HI_BUILTIN: {
      switch (l) {
      case BUILTIN_READ:
        fprintf (f, "CALL\tLread");
        break;
        
      case BUILTIN_WRITE:
        fprintf (f, "CALL\tLwrite");
        break;

      case BUILTIN_LENGTH:
        fprintf (f, "CALL\tLlength");
        break;

      case BUILTIN_STRING:
        fprintf (f, "CALL\tLstring");
        break;

      case BUILTIN_ARRAY:
        fprintf (f, "CALL\tBarray\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
        break;

      default:
        failure ("ERROR: invalid opcode %d-%d\n", h, l);
      }
    }
    break;
      
    default:
      failure ("ERROR: invalid opcode %d-%d\n", h, l);
    }

    fprintf (f, "\n");
  }
  while (1);
 stop: fprintf (f, "<end>\n");
}

/* Dumps the contents of the file */
void dump_file (FILE *f, bytefile *bf) {
  int i;
  
  fprintf (f, "String table size       : %d\n", bf->stringtab_size);
  fprintf (f, "Global area size        : %d\n", bf->global_area_size);
  fprintf (f, "Number of public symbols: %d\n", bf->public_symbols_number);
  fprintf (f, "Public symbols          :\n");

  for (i=0; i < bf->public_symbols_number; i++) 
    fprintf (f, "   0x%.8x: %s\n", get_public_offset (bf, i), get_public_name (bf, i));

  fprintf (f, "Code:\n");
  disassemble (f, bf);
}

int main (int argc, char* argv[]) {
  bytefile *f = read_file(argv[1]);
  dump_file (stdout, f);
  return 0;
}
