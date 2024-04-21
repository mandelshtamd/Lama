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

const uint8_t* disassemble_instruction(FILE *f, bytefile *bf, const uint8_t *ip, int (*print)(FILE *, const char *, ...)) {
# define INT    (ip += sizeof (int32_t), *(int32_t*)(ip - sizeof (int32_t)))
# define BYTE   *(ip++)
# define STRING get_string (bf, INT)
# define FAIL   failure ("ERROR: invalid opcode %d-%d\n", h, l)

    uint8_t x = BYTE,
            h = (x & 0xF0) >> 4,
            l = x & 0x0F;

    switch (h) {
        case 15:
            print(f, "STOP");
            break;
        case 0:
            print(f, "BINOP\t%s", ops[l-1]);
            break;

        case 1:
            switch (l) {
                case  0:
                    print(f, "CONST\t%d", INT);
                    break;

                case  1:
                    print(f, "STRING\t%s", STRING);
                    break;

                case  2:
                    print(f, "SEXP\t%s ", STRING);
                    print(f, "%d", INT);
                    break;

                case  3:
                    print(f, "STI");
                    break;

                case  4:
                    print(f, "STA");
                    break;

                case  5:
                    print(f, "JMP\t0x%.8x", INT);
                    break;

                case  6:
                    print(f, "END");
                    break;

                case  7:
                    print(f, "RET");
                    break;

                case  8:
                    print(f, "DROP");
                    break;

                case  9:
                    print(f, "DUP");
                    break;

                case 10:
                    print(f, "SWAP");
                    break;

                case 11:
                    print(f, "ELEM");
                    break;

                default:
                    FAIL;
            }
            break;

        case 2:
        case 3:
        case 4:
            print(f, "%s\t", lds[h-2]);
            switch (l) {
                case 0: print(f, "G(%d)", INT); break;
                case 1: print(f, "L(%d)", INT); break;
                case 2: print(f, "A(%d)", INT); break;
                case 3: print(f, "C(%d)", INT); break;
                default: FAIL;
            }
            break;

        case 5:
            switch (l) {
                case  0:
                    print(f, "CJMPz\t0x%.8x", INT);
                    break;

                case  1:
                    print(f, "CJMPnz\t0x%.8x", INT);
                    break;

                case  2:
                    print(f, "BEGIN\t%d ", INT);
                    print(f, "%d", INT);
                    break;

                case  3:
                    print(f, "CBEGIN\t%d ", INT);
                    print(f, "%d", INT);
                    break;

                case  4:
                    print(f, "CLOSURE\t0x%.8x", INT);
                    {int n = INT;
                        for (int i = 0; i<n; i++) {
                            switch (BYTE) {
                                case 0: print(f, "G(%d)", INT); break;
                                case 1: print(f, "L(%d)", INT); break;
                                case 2: print(f, "A(%d)", INT); break;
                                case 3: print(f, "C(%d)", INT); break;
                                default: FAIL;
                            }
                        }
                    };
                    break;

                case  5:
                    print(f, "CALLC\t%d", INT);
                    break;

                case  6:
                    print(f, "CALL\t0x%.8x ", INT);
                    print(f,  "%d", INT);
                    break;

                case  7:
                    print(f, "TAG\t%s ", STRING);
                    print(f, "%d", INT);
                    break;

                case  8:
                    print(f, "ARRAY\t%d", INT);
                    break;

                case  9:
                    print(f, "FAIL\t%d", INT);
                    print(f, "%d", INT);
                    break;

                case 10:
                    print(f, "LINE\t%d", INT);
                    break;

                default:
                    FAIL;
            }
            break;

        case 6:
            print(f, "PATT\t%s", pats[l]);
            break;

        case 7: {
            switch (l) {
                case 0:
                    print(f, "CALL\tLread");
                    break;

                case 1:
                    print(f, "CALL\tLwrite");
                    break;

                case 2:
                    print(f, "CALL\tLlength");
                    break;

                case 3:
                    print(f, "CALL\tLstring");
                    break;

                case 4:
                    print(f, "CALL\tBarray\t%d", INT);
                    break;

                default:
                    FAIL;
            }
        }
            break;

        default:
            FAIL;
    }

    return ip;
}