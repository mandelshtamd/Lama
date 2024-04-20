# include "common.h"

static void vfailure (char *s, va_list args) {
  fprintf(stderr, "*** FAILURE: ");
  vfprintf(stderr, s, args);   // vprintf (char *, va_list) <-> printf (char *, ...)
  exit(255);
}

void failure(char *s, ...) {
  va_list args;

  va_start(args, s);
  vfailure(s, args);
}

/* Gets a string from a string table by an index */
char* get_string (bytefile *f, int pos) {
  return &f->string_ptr[pos];
}

/* Gets a name for a public symbol */
char* get_public_name (bytefile *f, int i) {
  return get_string (f, f->public_ptr[i*2]);
}

/* Gets an offset for a publie symbol */
int get_public_offset (bytefile *f, int i) {
  return f->public_ptr[i*2+1];
}

bytefile* read_file(char* fname) {
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

    size_t fileSize = sizeof(bytefile) + size;
    bytefile* file = (bytefile*)malloc(fileSize);
    if (!file) {
        failure("ERROR: unable to allocate memory.\n");
    }

    if (file == 0) {
        failure("ERROR: unable to allocate memory.\n");
    }

    rewind(f);

    if (size != fread(&file->stringtab_size, 1, size, f)) {
        failure("%s\n", strerror(errno));
    }

    fclose(f);

    file->string_ptr =
        &file->buffer[file->public_symbols_number * 2 * sizeof(int)];
    file->public_ptr = (int*)file->buffer;
    file->code_ptr = (const uint8_t*)&file->string_ptr[file->stringtab_size];
    return file;
}

void read_h1_operation(uint8_t operation, uint8_t *ip, FILE* output, bytefile* bf, ByteCodeHandler handler) {
    switch (operation) {
        case CONST:
            handler(output, "CONST\t%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
            break;

        case BSTRING:
            handler(output, "STRING\t%s", get_string(bf, (ip += sizeof (int), *(int*)(ip - sizeof (int)))));
            break;

        case BSEXP:
            handler(output, "SEXP\t%s ", get_string(bf, (ip += sizeof (int), *(int*)(ip - sizeof (int)))));
            handler(output, "%d", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
            break;

        case STI:
            handler(output, "STI");
            break;

        case STA:
            handler(output, "STA");
            break;

        case JMP:
            handler(output, "JMP\t0x%.8x", (ip += sizeof (int), *(int*)(ip - sizeof (int))));
            break;

        case END:
            handler(output, "END");
            break;

        case RET:
            handler(output, "RET");
            break;

        case DROP:
            handler(output, "DROP");
            break;

        case DUP:
            handler(output, "DUP");
            break;

        case SWAP:
            handler(output, "SWAP");
            break;

        case ELEM:
            handler(output, "ELEM");
            break;

        default:
            failure("ERROR: invalid opcode\n");
    }
}