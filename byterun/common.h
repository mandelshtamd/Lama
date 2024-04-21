# ifndef BYTERUN_COMMON
# define BYTERUN_COMMON

# include <stdarg.h>
# include <string.h>
# include <stdio.h>
# include <errno.h>
# include <malloc.h>
# include <stdlib.h>
# include <stdint.h>


/* The unpacked representation of bytecode file */
typedef struct {
  char *string_ptr;              /* A pointer to the beginning of the string table */
  int  *public_ptr;              /* A pointer to the beginning of publics table    */
  uint8_t *code_ptr;                /* A pointer to the bytecode itself               */
  int  *global_ptr;              /* A pointer to the global area                   */
  int   stringtab_size;          /* The size (in bytes) of the string table        */
  int   global_area_size;        /* The size (in words) of global area             */
  int   public_symbols_number;   /* The number of public symbols                   */
  char  buffer[0];               
} bytefile;

static char *ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
static char *pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
static char *lds [] = {"LD", "LDA", "ST"};

enum {
    str_literal,
    string_type,
    array_type,
    sexp_type,
    ref_type,
    val_type,
    closure_type
};

enum {
    BINOP,
    H1_OPS,
    LD,
    LDA,
    ST,
    H5_OPS,
    PATT,
    HI_BUILTIN
};

enum {
    CONST,
    BSTRING,
    BSEXP,
    STI,
    STA,
    JMP,
    END,
    RET,
    DROP,
    DUP,
    SWAP,
    ELEM
};

enum {
    CJMPZ,
    CJMPNZ,
    BEGIN,
    CBEGIN,
    BCLOSURE,
    CALLC,
    CALL,
    TAG,
    ARRAY_KEY,
    FAIL,
    LINE
};

enum {
    BUILTIN_READ,
    BUILTIN_WRITE,
    BUILTIN_LENGTH,
    BUILTIN_STRING,
    BUILTIN_ARRAY,
};

enum {
    LOCATION_GLOBAL,
    LOCATION_LOCAL,
    LOCATION_ARGUMENT,
    LOCATION_CLOSURE
};

char* get_string (bytefile *f, int pos);

char* get_public_name (bytefile *f, int i);

int get_public_offset (bytefile *f, int i);

void failure (char *s, ...);

#ifdef __cplusplus
   extern "C" {
   #endif

   bytefile* read_file (char *fname);

   #ifdef __cplusplus
   }
#endif

void option_out(FILE *f, const char *format, ...);

#ifdef __cplusplus
   extern "C" {
   #endif

   const uint8_t* disassemble_instruction(FILE *f, bytefile *bf, const uint8_t *ip, int (*printer)(FILE *, const char *, ...));

   #ifdef __cplusplus
   }
#endif

# endif