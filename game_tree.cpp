#include "game_tree.h"

#include <tuple>

bool node_information::operator==(const node_information &o) const {
    return
        std::tie(color, moves_size, score) ==
        std::tie(o.color, o.moves_size, o.score);
}
