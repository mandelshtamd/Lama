#include <stddef.h>
#include <stdint.h>

#include "../runtime/gc.h"
#include "../runtime/runtime.h"
#include "../runtime/runtime_common.h"
#include "interpreter.h"

static const uint8_t* eof;

void* __stop_custom_data = 0;
void* __start_custom_data = 0;

static const int32_t EMPTY_BOX = BOX(0);

static int32_t gc_handled_memory[MEM_SIZE];
static int32_t call_stack[STACK_SIZE];
const uint8_t* ip;
static int32_t* fp;
static const int32_t* call_stack_bottom = call_stack + STACK_SIZE;
static int32_t* call_stack_top;
extern size_t __gc_stack_top;
extern size_t __gc_stack_bottom;
static int32_t* globals;
static bytefile* bf;

static int n_args = 0;
static int n_locals = 0;
static bool is_closure = false;

static bytefile* read_file(char* fname) {
    FILE* f = fopen(fname, "rb");

    if (!f) {
        failure("ERROR: unable to open file %s\n", strerror(errno));
    }

    if (fseek(f, 0, SEEK_END) == -1) {
        failure("ERROR: unable to seek file end %s\n", strerror(errno));
    }

    long size = ftell(f);
    if (size == -1) {
        failure("ERROR: unable to get file size %s\n", strerror(errno));
    }

    size_t file_size = sizeof(bytefile) + size;
    bytefile* file = malloc(file_size);
    if (!file) {
        failure("ERROR: unable to allocate memory.\n");
    }
    eof = (unsigned char*)file + file_size;

    if (file == 0) {
        failure("ERROR: unable to allocate memory.\n");
    }

    rewind(f);

    if (size != fread(&file->string_table_size, 1, size, f)) {
        failure("%s\n", strerror(errno));
    }

    fclose(f);

    file->string_ptr =
        &file->buffer[file->public_symbols_number * 2 * sizeof(int)];
    file->public_ptr = (int*)file->buffer;
    file->code_ptr = (const uint8_t*)&file->string_ptr[file->string_table_size];
    return file;
}

static inline int32_t* sp() { return (int32_t*)__gc_stack_top; }

static inline void move_sp(int delta) {
    __gc_stack_top = (size_t)(sp() + delta);
}

static inline void push_op(int32_t value) {
    *sp() = value;
    move_sp(-1);
    if (sp() == gc_handled_memory) {
        failure("ERROR: operands stack overflow\n");
    }
}

static inline void push_call(int32_t value) {
    *call_stack_top = value;
    call_stack_top--;
    if (call_stack_top == call_stack) {
        failure("ERROR: call stack overflow\n");
    }
}

static inline int32_t pop_op(void) {
    if (sp() == (int32_t*)__gc_stack_bottom - 1) {
        failure("ERROR: try to access empty operands stack\n");
    }
    move_sp(1);
    return *sp();
}

static inline int32_t peek_op(void) { return *(sp() + 1); }

static inline int32_t pop_call() {
    if (call_stack_top == call_stack_bottom - 1) {
        failure("ERROR: try to access empty call stack\n");
    }
    call_stack_top++;
    return *call_stack_top;
}

static void init_interpreter(int32_t global_area_size) {
    __gc_init();
    __gc_stack_bottom = (size_t)gc_handled_memory + MEM_SIZE;
    globals = (int32_t*)__gc_stack_bottom - global_area_size;
    __gc_stack_top = (size_t)(globals - 1);
    call_stack_top = (int32_t*)call_stack_bottom - 1;

    for (int i = 0; i < global_area_size; i++) {
        globals[i] = EMPTY_BOX;
    }
}

static inline void update_ip(const uint8_t* new_ip) {
    if (new_ip < bf->code_ptr || new_ip >= eof) {
        failure("ERROR: instruction pointer points out of bytecode area.");
    }
    ip = new_ip;
}

static inline unsigned char next_byte() {
    if (ip + 1 >= eof) {
        failure("ERROR: instruction pointer points out of bytecode area.\n");
    }
    return *ip++;
}

static inline int32_t next_int() {
    if (ip + sizeof(int32_t) >= eof) {
        failure("ERROR: instruction pointer points out of bytecode area.\n");
    }
    return (ip += sizeof(int32_t), *(int32_t*)(ip - sizeof(int32_t)));
}

static inline void call(void) {
    int32_t func_label = next_int();
    int32_t n_args = next_int();
    is_closure = false;

    push_call((int32_t)ip);
    update_ip(bf->code_ptr + func_label);
}

static inline void begin(int local_variables_count, int args_count) {
    push_call((int32_t)fp);
    push_call(n_args);
    push_call(n_locals);
    push_call(is_closure);

    fp = sp();

    n_args = args_count;
    n_locals = local_variables_count;
    for (int i = 0; i < local_variables_count; i++) {
        push_op(EMPTY_BOX);
    }
}

static inline const char* get_string_by_position(bytefile* f, uint32_t pos) {
    if (pos >= f->string_table_size) {
        failure("ERROR: no such index in string pool.\n");
    }
    return &f->string_ptr[pos];
}

static inline void tag(void) {
    const char* tag = get_string_by_position(bf, next_int());
    int32_t n_field = next_int();
    int32_t sexp = pop_op();
    int32_t tag_hash = LtagHash((char*)tag);
    push_op(Btag((void*)sexp, tag_hash, BOX(n_field)));
}

static inline char* get_closure_content(int32_t* p) {
    data* closure_obj = TO_DATA(p);
    int t = TAG(closure_obj->data_header);
    if (t != CLOSURE_TAG) {
        failure("ERROR: pointer to not-closure object as closure argument.\n");
    }
    return closure_obj->contents;
}

static inline int32_t get_closure_addr(int32_t* p) {
    return ((int32_t*)get_closure_content(p))[0];
}

static inline int32_t* get_addr(int32_t location, int32_t idx) {
    if (idx < 0) {
        failure("ERROR: index less than zero.");
    }
    switch (location) {
        case LOCATION_GLOBAL:
            if (globals + idx >= (int32_t*)__gc_stack_bottom) {
                failure("ERROR: out of memory (global %d).\n", idx);
            }
            return globals + idx;
        case LOCATION_LOCAL:
            if (idx >= n_locals) {
                failure("ERROR: operands stack overflow.\n");
            }
            return fp - idx;
        case LOCATION_ARGUMENT:
            if (idx >= n_args) {
                failure("ERROR: arguments overflow.\n");
            }
            return fp + n_args - idx;
        case LOCATION_CLOSURE: {
            int32_t* closure_addr =
                (int32_t*)get_closure_content((int32_t*)fp[n_args + 1]);
            return (closure_addr + idx + 1);
        }
        default:
            failure("ERROR: unknown location %d.\n", location);
    }
}

static inline void ld(int32_t location_type, int index) {
    int32_t* location = get_addr(location_type, index);
    push_op(*location);
}

static inline void lda(int32_t location_type, int index) {
    int32_t* location = get_addr(location_type, index);
    push_op((int32_t)location);
}

static inline void st(int32_t location_type, int index) {
    int32_t value = peek_op();
    int32_t* location = get_addr(location_type, index);
    *location = value;
}

static inline void sta() {
    int32_t value = pop_op();
    int32_t dest = pop_op();
    if (UNBOXED(dest)) {
        int32_t array = pop_op();
        Bsta((void*)value, dest, (void*)array);
    } else {
        *(int32_t*)dest = value;
    }
    push_op(value);
}

static inline void closure(void) {
    void* closure_addr = (void*)next_int();
    int32_t num_values = next_int();

    data* closure_data = (data*)alloc_closure(num_values + 1);

    push_extra_root((void**)&closure_data);

    ((void**)closure_data->contents)[0] = closure_addr;

    for (int i = 0; i < num_values; i++) {
        unsigned char location_type = next_byte();
        int32_t idx = next_int();
        int32_t* location = get_addr(location_type, idx);
        ((int*) closure_data->contents)[i + 1] = *location;
    }

    pop_extra_root((void**)&closure_data);
    push_op((int32_t)closure_data->contents);
}

static inline void execute_closure(void) {
    int32_t args_count = next_int();
    int32_t closure_label = get_closure_addr((int32_t*)sp()[args_count + 1]);
    is_closure = true;

    push_call((int32_t)ip);
    update_ip(bf->code_ptr + (int32_t)closure_label);
}

static inline void initialize_closure_context() {
    int arg_count = next_int();
    int local_variables_count = next_int();
    begin(local_variables_count, arg_count);
}

static inline void finalize_function() {
    int32_t returned_value = pop_op();

    move_sp(n_args + n_locals);

    bool is_closure = pop_call();
    if (is_closure) {
        pop_op();
    }

    push_op(returned_value);

    n_locals = pop_call();
    n_args = pop_call(); 
    fp = (int32_t*)pop_call();

    if (call_stack_top != call_stack_bottom - 1) {
        ip = (unsigned char*)pop_call();
    }
}

static inline void binary_operation(int32_t operator_code) {
    int32_t b = UNBOX(pop_op());
    int32_t a = UNBOX(pop_op());
    int32_t result = 0;
    switch (operator_code) {
#define IMPLEMENT_BINOP(n, op) \
    case n:                    \
        result = a op b;       \
        break;

        BINOPS(IMPLEMENT_BINOP)

#undef IMPLEMENT_BINOP
        default:
            failure("ERROR: unknown operand code: %d.\n", operator_code);
            return;
    }
    result = BOX(result);
    push_op(result);
}

static inline void call_barray(void) {
    int num_elements = next_int();
    data* array_data = (data*)alloc_array(num_elements);

    for (int i = num_elements - 1; i >= 0; i--) {
        int value = pop_op();
        ((int*)array_data->contents)[i] = value;
    }
    push_op((int32_t)array_data->contents);
}

static inline void call_bsexp(void) {
    const char* tag = get_string_by_position(bf, next_int());
    int num_elements = next_int();
    data* exp = (data*) alloc_sexp(num_elements);
    ((sexp*)exp)->tag = 0;

    for (int i = num_elements; i >= 1; i--) {
        int value = pop_op();
        ((int*)exp->contents)[i] = value;
    }

    ((sexp*)exp)->tag = UNBOX(LtagHash((char *)tag));
    push_op((int32_t)exp->contents);
}

static inline bool check_tag(int32_t obj, int32_t tag) {
    if (UNBOXED(obj)) {
        return false;
    }

    int32_t actual_tag = TAG(TO_DATA(obj)->data_header);

    switch (tag) {
        case ref_type:
            return true;
        case str_literal: {
            int32_t other_str = pop_op();
            return UNBOX(Bstring_patt((void*)other_str, (void*)obj));
        }
        case string_type:
            return actual_tag == STRING_TAG;
        case array_type:
            return actual_tag == ARRAY_TAG;
        case sexp_type:
            return actual_tag == SEXP_TAG;
        case (closure_type):
            return actual_tag == CLOSURE_TAG;
        default:
            failure("ERROR: unknown tag %d.\n", tag);
    }
}

static inline void match_pattern(int32_t patt_type) {
    bool result = false;
    int32_t obj = pop_op();

    if (patt_type == val_type) {
        result = UNBOXED(obj);
    } else {
        result = check_tag(obj, patt_type);
    }
    push_op(BOX(result));
}

static inline void verify_array(void) {
    int32_t requested_size = BOX(next_int());
    int32_t actual_obj = pop_op();
    push_op(Barray_patt((void*)actual_obj, requested_size));
}

static bool execute_h1_operation(uint8_t operation) {
    switch (operation) {
        case CONST:
            push_op(BOX(next_int()));
            break;  

        case BSTRING: {
            const char* string_in_pool = get_string_by_position(bf, next_int());
            push_op((int32_t)Bstring((char *)string_in_pool));
            break;
        }
        case BSEXP:
            call_bsexp();
            break;

        case STA:
            sta();
            break;

        case JMP: {
            int x = next_int();
            update_ip(bf->code_ptr + x);
            break;
        }

        case END:
            finalize_function();
            if (call_stack_top == call_stack_bottom - 1) {
                return true;
            }
            break;

        case DROP:
            pop_op();
            break;

        case DUP:
            push_op(peek_op());
            break;

        case ELEM: {  
            int32_t idx = pop_op();
            int32_t array = pop_op();
            push_op((int32_t)Belem((char*)array, idx));
            break;
        }

        case RET:
            failure("ERROR: bytecode RET is unsupported.\n");

        case SWAP:
            failure("ERROR: bytecode SWAP is unsupported.\n");

        case STI:
            failure("ERROR: bytecode STI is unsupported.\n");

        default:
            failure("ERROR: invalid opcode %d.\n", operation);
    }
    return false;
}

static void handle_conditional_jump(uint8_t operation) {
    int target = next_int();
    bool condition = (operation == CJMPZ) ? !UNBOX(pop_op()) : UNBOX(pop_op());
    if (condition) {
        update_ip(bf->code_ptr + target);
    }
}

static void execute_h5_operation(uint8_t operation) {
    switch (operation) {
        case CJMPZ:
        case CJMPNZ:
            handle_conditional_jump(operation);
            break;

        case BEGIN:
            int args_count = next_int();
            int locals_count = next_int();
            begin(locals_count, args_count);
            break;

        case CBEGIN:
            initialize_closure_context();
            break;

        case BCLOSURE:
            closure();
            break;

        case CALLC:
            execute_closure();
            break;
        case CALL:
            call();
            break;
        case TAG:
            tag();
            break;
        case ARRAY_KEY:
            verify_array();
            break;

        case FAIL:
            int32_t line = next_int();
            int32_t column = next_byte();
            failure("FAIL at \t%d:%d.\n", line, column);

        case LINE:
            next_int();
            break;

        default:
            failure("ERROR: invalid opcode %d.\n", operation);
    }
}

static void execute_builtin_operation(uint8_t operation) {
    int32_t value;

    switch (operation) {
        case BUILTIN_READ:
            value = Lread();
            break;

        case BUILTIN_WRITE:
            value = Lwrite(pop_op());
            break;

        case BUILTIN_LENGTH:
            value = Llength((char*)pop_op());
            break;

        case BUILTIN_STRING:
            value = (int32_t) Lstring((char*)pop_op());
            break;

        case BUILTIN_ARRAY:
            call_barray();
            return;

        default:
            failure("ERROR: invalid opcode %d.\n", operation);
    }
    push_op(value);
}

static void interpret(FILE* f) {
    ip = bf->code_ptr;
    while (true) {
        uint8_t opcode = next_byte();
        uint8_t high_part = (opcode & 0xF0) >> 4;
        uint8_t low_part = opcode & 0x0F;

        if (high_part == 0xFF) {
            break;
        }

        switch (high_part) {
            case BINOP:
                binary_operation(low_part - 1);
                break;
            case H1_OPS: {
                bool exit = execute_h1_operation(low_part);
                if (exit) {
                    return;
                }
                break;
            }
            case LD:
                ld(low_part, next_int());
                break;
            case LDA:
                lda(low_part, next_int());
                break;
            case ST:
                st(low_part, next_int());
                break;
            case H5_OPS: {
                execute_h5_operation(low_part);
                break;
            }
            case PATT:
                match_pattern(low_part);
                break;

            case HI_BUILTIN: {
                execute_builtin_operation(low_part);
                break;
            }
            default:
                failure("ERROR: Invalid opcode %d-%d.\n", high_part, low_part);
                return;
        }
    }
    return;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        failure("ERROR: provide bytecode file.\n");
    }
    bf = read_file(argv[1]);
    init_interpreter(bf->global_area_size);
    interpret(stdout);
    return 0;
}
