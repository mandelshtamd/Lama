#include <stddef.h>
#include <stdint.h>

enum { PLUS, MINUS, MULTIPLY, DIVIDE, MOD, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL, AND, OR };
#define BINOPS(op)                                                            \
    op(PLUS, +) op(MINUS, -) op(MULTIPLY, *) op(DIVIDE, /) op(MOD, %) op(LESS, <) op(LESS_EQUAL, <=) \
    op(GREATER, >) op(GREATER_EQUAL, >=) op(EQUAL, ==) op(NOT_EQUAL, !=) op(AND, &&) op(OR, ||)

typedef struct {
    const char* string_ptr;
    const int* public_ptr;
    const uint8_t* code_ptr;
    const int* global_ptr;
    unsigned int string_table_size;
    unsigned int global_area_size;
    unsigned int public_symbols_number;
    char buffer[0];
} bytefile;

#define STACK_SIZE (1 << 20)
#define MEM_SIZE (STACK_SIZE)

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