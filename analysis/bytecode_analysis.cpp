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
#include "bytecode_analysis.h"
#include "../byterun/common.h"
#include "lib.h"

uint8_t* eof;

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

int empty_output(FILE *f, const char *value, ...) {
    return 0;
}

struct ByteCodeAnalyzer {
    ByteCodeAnalyzer(bytefile* bf) : bf(bf) {}
    void displayFrequency() {
        calculateCodes();
        std::vector<std::pair<Instruction, int32_t>> sortedByteCodes(byteCodes.begin(),
                                                              byteCodes.end());
        std::sort(sortedByteCodes.begin(), sortedByteCodes.end(),
                  [](auto& a, auto& b) { return a.second > b.second; });
        for (auto& [instruction, count] : sortedByteCodes) {
            std::cout << count << " times\t";
            disassemble_instruction(stdout, bf, instruction.getBegin(), &fprintf);
            std::cout << std::endl;
        }
    }

   private:
    bytefile* bf;
    std::map<Instruction, int32_t> byteCodes;

    void calculateCodes() {
        const uint8_t* ip = bf->code_ptr;

        while (ip < eof) {
            const uint8_t* insnEnd = disassemble_instruction(nullptr, bf, ip, &empty_output);
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
    bytefile* bf = read_file(argv[1]);
    eof = (unsigned char*)bf + malloc_usable_size(bf);
    ByteCodeAnalyzer analyzer(bf);
    analyzer.displayFrequency();
    return 0;
}