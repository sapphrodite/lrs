#include <stdio.h>
#include <string>

#include "interpreter.h"


const char* interpreter::exec(const char* cmdin) {
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
		moveto(e, argtoi(0), argtoi(1), argtoi(2), argtoi(3));
	} else if (strcmp(cmd, "setbounds") == 0) {
		setbounds(e, argtoi(0), argtoi(1), argtoi(2), argtoi(3), argtoi(4), argtoi(5));
	} else if (strcmp(cmd, "setuv") == 0) {
		setuv(e, argtoi(0), argtoi(1), argtof(2), argtof(3), argtof(4), argtof(5));
	} else if (strcmp(cmd, "settex") == 0) {
		settex(e, argtoi(0), argtoi(1));
	} else if (strcmp(cmd, "rotate") == 0) {
		rotate(e, argtoi(0), argtoi(1), argtoi(2));
	} else if (strcmp(cmd, "addhitbox") == 0) {
		addhitbox(e, argtoi(0), argtoi(1), argtoi(2), argtoi(3), argtoi(4));
	} else if (strcmp(cmd, "addforce") == 0) {
		addforce(e, argtoi(0),  argtof(1), argtof(2), argtoi(3), strcmp(getarg(4), "true") == 0);
	} else if (strcmp(cmd, "setattr") == 0) {
		std::array<char*, 256> ptrarr;
		int attr_args = argc - argoffset - 1; // first two are the entity and attr name
		for (int i = 0; i < attr_args; i++) {
			ptrarr[i] = getarg(i + 2);
		}
		setattr(e, argtoi(0), getarg(1), attr_args, ptrarr.data());
	} else if (strcmp(cmd, "addcomponent") == 0) {
		addcomponent(e, argtoi(0), getarg(1));
	} else if (strcmp(cmd, "run") == 0) {
		run(e, getarg(0));
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

int interpreter::get_varidx(const char* ref) {
	assert(ref != nullptr);
	if (strlen(ref) != 2 || ref[0] != '$' || ref[1] < '0' || ref[1] > '9')
		return -1;
	return ref[1] - '0';
}

// todo - argument parsing to restrict to alphanumeric characters
char* interpreter::decodearg(char* arg) {
	int idx = get_varidx(arg);
	return idx >= 0 ? vars[idx] : arg;
}

char* interpreter::getarg(size_t i) { return decodearg(argv[i + argoffset]); }
size_t interpreter::argtoi(size_t argi) { return std::stoi(decodearg(argv[argi + argoffset])); }
size_t interpreter::argtof(size_t argi) { return std::stof(decodearg(argv[argi + argoffset])); }

// as it stands, this function works, although it doesn't preserve the '='...
// returns argc
size_t interpreter::parse(const char* cmd) {
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
