#include <cstdio>
#include <ios>
#include <iostream>


void failure(const char* patt, const char* arg) {
    printf("ERROR!\n");
    printf(patt, arg);
    exit(1);
}

void failure(const char* patt, int l, int r) {
    printf("ERROR!\n");
    printf(patt, l, r);
    exit(1);
}

void failure(const char* msg) {
    std::cout << "ERROR!\n" << msg;
    exit(1);
}