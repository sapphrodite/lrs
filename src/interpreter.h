#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <array>
#include <cstring>
#include <cassert>

#include <engine/engine.h>

struct interpreter {

    const char* exec(const char* cmdin);
    engine* e;


private:
    int get_varidx(const char* ref);
    char* decodearg(char* arg);

    char* getarg(size_t i);
    size_t argtoi(size_t argi);
    size_t argtof(size_t argi);

    // as it stands, this function works, although it doesn't preserve the '='...
    // returns argc
    size_t parse(const char* cmd);
 

    int argc;
    int argoffset = 0; // to account for $1 being the first string
    std::array<char[256], 256> argv;
    std::array<char[256], 10> vars;
    char output[256];
};



#endif //INTERPRETER_H
