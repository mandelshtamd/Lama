#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "../interpreter/interpreter.h"
#include "bytecode_analysis.h"
#include "lib.h"

uint8_t* eof;

static bytefile* readFile(char* fname) {
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
    eof = (unsigned char*)file + fileSize;

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

struct ByteReader {
    ByteReader(uint8_t* eof, bytefile* bf)
        : eof(eof), isPrint(false), bf(bf) {}

    ByteReader(uint8_t* eof, bytefile* bf, char* buf, size_t bufSize)
        : eof(eof), isPrint(true), bf(bf), buf(buf), bufSize(bufSize) {}

    const uint8_t* readInstruction(const uint8_t* ip) {
        written = 0;
        this->ip = ip;
        return readInstruction();
    }

    char* getString() { return buf; }

   private:
    bytefile* bf;
    uint8_t* eof;
    size_t bufSize = 0;
    char* buf = nullptr;
    bool isPrint;
    const uint8_t* ip;
    size_t written = 0;

    void collectByteCode(const char* fmt, ...) {
        if (!isPrint) return;
        va_list args;
        va_start(args, fmt);
        written += vsnprintf(buf + written, bufSize - written, fmt, args);
    }

    const char* getStringByPosition(int pos) {
        if (unsigned(pos) >= unsigned(bf->string_table_size)) {
            failure("ERROR: no such index in string pool.\n");
        }
        return &bf->string_ptr[pos];
    }

    int32_t nextInt() {
        if (ip + sizeof(int) >= eof) {
            failure("ERROR: instruction pointer points out of bytecode area.\n");
        }
        return (ip += sizeof(int), *(int*)(ip - sizeof(int)));
    }

    unsigned char nextByte() {
        if (ip >= eof) {
            failure("ERROR: instruction pointer points out of bytecode area.\n");
        }
        return *ip++;
    }

    void readH1Operation(uint8_t operation) {
        switch (operation) {
            case CONST:
                collectByteCode("CONST\t%d", INT);
                break;

            case BSTRING:
                collectByteCode("STRING\t%s", STRING);
                break;

            case BSEXP:
                collectByteCode("SEXP\t%s ", STRING);
                collectByteCode("%d", INT);
                break;

            case STI:
                collectByteCode("STI");
                break;

            case STA:
                collectByteCode("STA");
                break;

            case JMP:
                collectByteCode("JMP\t0x%.8x", INT);
                break;

            case END:
                collectByteCode("END");
                break;

            case RET:
                collectByteCode("RET");
                break;

            case DROP:
                collectByteCode("DROP");
                break;

            case DUP:
                collectByteCode("DUP");
                break;

            case SWAP:
                collectByteCode("SWAP");
                break;

            case ELEM:
                collectByteCode("ELEM");
                break;

            default:
                failure("ERROR: invalid opcode\n");
        }
    }

    void readH5Operation(uint8_t operation) {
        switch (operation) {
            case CJMPZ:
                collectByteCode("CJMPz\t0x%.8x", INT);
                break;

            case CJMPNZ:
                collectByteCode("CJMPnz\t0x%.8x", INT);
                break;

            case BEGIN:
                collectByteCode("BEGIN\t%d ", INT);
                collectByteCode("%d", INT);
                break;

            case CBEGIN:
                collectByteCode("CBEGIN\t%d ", INT);
                collectByteCode("%d", INT);
                break;

            case BCLOSURE:
                collectByteCode("CLOSURE\t0x%.8x", INT);
                {
                    int n = INT;
                    for (int i = 0; i < n; i++) {
                        switch (BYTE) {
                            case 0:
                                collectByteCode("GLOBAL(%d)", INT);
                                break;
                            case 1:
                                collectByteCode("LOCAL(%d)", INT);
                                break;
                            case 2:
                                collectByteCode("ARGUMENT(%d)", INT);
                                break;
                            case 3:
                                collectByteCode("CLOSURE(%d)", INT);
                                break;
                            default:
                                failure("ERROR: invalid opcode\n");
                        }
                    }
                };
                break;

            case CALLC:
                collectByteCode("CALLC\t%d", INT);
                break;

            case CALL:
                collectByteCode("CALL\t0x%.8x ", INT);
                collectByteCode("%d", INT);
                break;

            case TAG:
                collectByteCode("TAG\t%s ", STRING);
                collectByteCode("%d", INT);
                break;

            case ARRAY_KEY:
                collectByteCode("ARRAY\t%d", INT);
                break;

            case FAIL:
                collectByteCode("FAIL\t%d", INT);
                collectByteCode("%d", INT);
                break;

            case LINE:
                collectByteCode("LINE\t%d", INT);
                break;

            default:
                failure("ERROR: invalid opcode\n");
        }
    }

    void readBuiltinOperation(uint8_t operation) {
        switch (operation) {
            case BUILTIN_READ:
                collectByteCode("CALL\tLread");
                break;

            case BUILTIN_WRITE:
                collectByteCode("CALL\tLwrite");
                break;

            case BUILTIN_LENGTH:
                collectByteCode("CALL\tLlength");
                break;

            case BUILTIN_STRING:
                collectByteCode("CALL\tLstring");
                break;

            case BUILTIN_ARRAY:
                collectByteCode("CALL\tBarray\t%d", INT);
                break;

            default:
                failure("ERROR: invalid opcode\n");
        }
    }

    const uint8_t* readInstruction() {

        const char* ops[] = {"+", "-",  "*",  "/",  "%",  "<", "<=",
                             ">", ">=", "==", "!=", "&&", "!!"};
        const char* pats[] = {"=str", "#string", "#array", "#sexp",
                              "#ref", "#val",    "#fun"};
        const char* lds[] = {"LD", "LDA", "ST"};

        uint8_t opcode = nextByte();
        uint8_t highPart = (opcode & 0xF0) >> 4;
        uint8_t lowPart = opcode & 0x0F;

        switch (highPart) {
            case STOP:
                collectByteCode("STOP");
                break;

            case BINOP:
                collectByteCode("BINOP\t%s", ops[lowPart - 1]);
                break;

            case H1_OPS:
                readH1Operation(lowPart);
                break;

            case LD:
            case LDA:
            case ST:
                collectByteCode("%s\t", lds[highPart - 2]);
                switch (lowPart) {
                    case 0:
                        collectByteCode("GLOBAL(%d)", INT);
                        break;
                    case 1:
                        collectByteCode("LOCAL(%d)", INT);
                        break;
                    case 2:
                        collectByteCode("ARGUMENT(%d)", INT);
                        break;
                    case 3:
                        collectByteCode("CLOSURE(%d)", INT);
                        break;
                    default:
                        failure("ERROR: invalid opcode\n");
                }
                break;

            case H5_OPS:
                readH5Operation(lowPart);
                break;

            case PATT:
                collectByteCode("PATT\t%s", pats[lowPart]);
                break;

            case HI_BUILTIN:
                readBuiltinOperation(lowPart);
                break;

            default:
                failure("ERROR: invalid opcode\n");
        }
        return ip;
    }
};

struct Instruction {
    Instruction(const uint8_t* start, const uint8_t* end)
        : begin(start), end(end) {}

    bool operator<(Instruction other) const {
        auto res = memcmp(begin, other.begin, end - begin);
        return res < 0;
    }

    const uint8_t* getBegin() { return begin; }

   private:
    const uint8_t* begin;
    const uint8_t* end;
};

struct ByteCodeAnalyzer {
    ByteCodeAnalyzer(bytefile* bf) : bf(bf) {}
    void displayFrequency() {
        calculateCodes();
        std::vector<std::pair<Instruction, int32_t>> sortedByteCodes(byteCodes.begin(),
                                                              byteCodes.end());
        std::sort(sortedByteCodes.begin(), sortedByteCodes.end(),
                  [](auto& a, auto& b) { return a.second > b.second; });
        const size_t bufSize = 1024;
        std::vector<char> buffer(bufSize);
        ByteReader br(eof, bf, buffer.data(), bufSize);
        for (auto& [instruction, count] : sortedByteCodes) {
            br.readInstruction(instruction.getBegin());
            std::cout << std::left << std::setw(24) << br.getString() << "\t"
                      << count << std::endl;
        }
    }

   private:
    bytefile* bf;
    std::map<Instruction, int32_t> byteCodes;

    void calculateCodes() {
        const uint8_t* ip = bf->code_ptr;

        ByteReader br(eof, bf);
        while (ip < eof) {
            const uint8_t* insnEnd = br.readInstruction(ip);
            Instruction instruction(ip, insnEnd);
            ip = insnEnd;
            byteCodes.try_emplace(instruction, 0).first->second++;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        failure("ERROR: provide bytecode file.\n");
    }
    bytefile* bf = readFile(argv[1]);
    ByteCodeAnalyzer analyzer(bf);
    analyzer.displayFrequency();
    return 0;
}