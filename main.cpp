#include <iostream>
#include <array>
#include <cassert>

#include "game_tree.h"
#include "montecarlo_tree_search.h"

static std::array<unsigned, 4> scores_a{2, 0, 3, 1};
static std::array<unsigned, 4> scores_b{2, 3, 0, 1};

struct prisoners : public game_tree {
    prisoners() : game_tree(2) {}

    void get_data(unsigned node, node_information *player_nodes) override {
        if (node == 0) {
            player_nodes[0] = {0, 2, 0};
            player_nodes[1] = {0, 2, 0};
        } else {
            player_nodes[0] = {0, 0, scores_a[node - 1]};
            player_nodes[1] = {0, 0, scores_b[node - 1]};
        }
    }

    unsigned get_child(unsigned node, unsigned *moves) override {
        assert(node == 0);
        assert(moves[0] < 2);
        assert(moves[1] < 2);

        return 1 + moves[0] * 2 + moves[1];
    }
};

int main() {
    std::cout << "Hello World!" << std::endl;

    prisoners p;
    montecarlo_tree_search s(p);

    s.step();
    s.step();
    s.step();
    s.step();
    s.step();

    return 0;
}
