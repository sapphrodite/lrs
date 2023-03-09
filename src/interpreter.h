#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <array>
#include <cstring>
#include <cassert>

struct interpreter {

    const char* exec(const char* cmdin) {
	argc = parse(cmdin);

	int varidx = get_varidx(argv[0]);
	const char* cmd = (varidx < 0) ? argv[0] : argv[1];
	argoffset = (varidx < 0) ? 1 : 2;
	if (strcmp(cmd, "run_tick") == 0) {
		run_tick(e);
	} else if (strcmp(cmd, "render") == 0) {
		render(e);
	} else if (strcmp(cmd, "addentity") == 0) { 
	    sprintf(output, "%i", addentity(e));
	} else if (strcmp(cmd, "addsprite") == 0) {
	    sprintf(output, "%i", addsprite(e, argtoi(0), argtoi(1)));
	} else if (strcmp(cmd, "moveto") == 0) {
	    moveto(e, argtoi(0), argtoi(1), argtoi(2));
	} else if (strcmp(cmd, "setsize") == 0) {
	    setsize(e, argtoi(0), argtoi(1), argtoi(2), argtoi(3));
	} else if (strcmp(cmd, "setbounds") == 0) {
	    setbounds(e, argtoi(0), argtoi(1), argtoi(2), argtoi(3), argtoi(4), argtoi(5));
	} else if (strcmp(cmd, "setuv") == 0) {
	    setuv(e, argtoi(0), argtoi(1), argtof(2), argtof(3), argtof(4), argtof(5));
	} else if (strcmp(cmd, "settex") == 0) {
	    settex(e, argtoi(0), argtoi(1));
	} else if (strcmp(cmd, "setattr") == 0) {
	    std::array<char*, 256> ptrarr;
	    int attr_args = argc - argoffset - 1; // first two are the entity and attr name 
	    for (int i = 0; i < attr_args; i++) {
		ptrarr[i] = getarg(i + 2);
	    }
	    setattr(e, argtoi(0), getarg(1), attr_args, ptrarr.data());
	} else if (strcmp(cmd, "addcomponent") == 0) {
	    addcomponent(e, argtoi(0), getarg(1)); 
	} else {
		printf("Unrecognized command: %s\n", cmd);
	}

	if (varidx >= 0) {
	    strcpy(vars[varidx], output);
	    return nullptr;
	} else {
	    return output;
	}
    }
    engine* e;


private:
    int get_varidx(const char* ref) {
	assert(ref != nullptr);
	if (strlen(ref) != 2 || ref[0] != '$' || ref[1] < '0' || ref[1] > '9')
	    return -1;
	return ref[1] - '0';        
    }

    // todo - argument parsing to restrict to alphanumeric characters
    char* decodearg(char* arg) {
	int idx = get_varidx(arg);
	return idx >= 0 ? vars[idx] : arg;
    }

    char* getarg(size_t i) { return decodearg(argv[i + argoffset]); }
    size_t argtoi(size_t argi) { return std::stoi(decodearg(argv[argi + argoffset])); }
    size_t argtof(size_t argi) { return std::stof(decodearg(argv[argi + argoffset])); }

    // as it stands, this function works, although it doesn't preserve the '='...
    // returns argc
    size_t parse(const char* cmd) {
		argc = 0;

		for (int i = 0; i < 256; i++) {
			memset(argv[i], 0, 256);
		}

	size_t read_idx = 0;
	size_t write_idx = 0;
	size_t word_idx = 0;
	bool skipping = false;
	for (;;) {
		switch (cmd[read_idx]) {
		case ' ':
	    case '=':
	    case '\t':
			if (skipping) {
			    break;
			}
			argv[word_idx][write_idx] = 0;
			word_idx++;
			write_idx = 0;
			skipping = true;
			break;
	    case '\n':
	    case '\0':
			return word_idx;
	    default:
			argv[word_idx][write_idx++] = cmd[read_idx];
			skipping = false;
		break;
}
	read_idx++;
	}
    } 
    
    int argc;
    int argoffset = 0; // to account for $1 being the first string
    std::array<char[256], 256> argv;
    std::array<char[256], 10> vars;
    char output[256];
};



#endif //INTERPRETER_H
