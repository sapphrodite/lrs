#ifndef RAII_TYPES_H
#define RAII_TYPES_H

struct no_copy {
    no_copy() = default;
    no_copy (no_copy&) = delete;
    no_copy& operator = (no_copy&) = delete;
};

struct no_move {
    no_move() = default;
    no_move (no_move&&) = delete;
    no_move& operator = (no_move&&) = delete;
};

#endif //RAII_TYPES_H
