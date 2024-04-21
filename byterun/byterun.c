/* Lama SM Bytecode interpreter */

# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>
# include "../runtime/runtime.h"
# include "common.h"

void *__start_custom_data;
void *__stop_custom_data;

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

  uint8_t *ip = bf->code_ptr;
  while (disassemble_instruction(NULL, bf, ip) != 0xFF) {
    ip = disassemble_instruction(f, bf, ip);
    fprintf(f, "\n");
  }
}

int main (int argc, char* argv[]) {
  bytefile *f = read_file(argv[1]);
  dump_file (stdout, f);
  return 0;
}
