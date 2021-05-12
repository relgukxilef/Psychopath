#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <random>

#include "game_tree.h"

struct statistics {
    unsigned sum, count;
};

struct local_edge {
    bool operator== (const local_edge&) const;

    unsigned start; // current node
    unsigned move; // possible move to make
    node_information start_data; // info at the current node
};

template<> struct std::hash<local_edge> {
    std::size_t operator() (const local_edge& n) const noexcept {
        return n.start + n.move * 114381301u + n.start_data.color * 779512493u;
    }
};

struct montecarlo_tree_search {
    montecarlo_tree_search(game_tree& tree);

    void step();

    game_tree& tree;

    std::vector<bool> nodes_visited; // visits during simulation don't count

    std::vector<statistics> local_statistics;
    std::unordered_map<local_edge, unsigned> local_children;

    std::vector<unsigned> depths_players_local; // player * depth array

    std::unique_ptr<unsigned[]> players_move;
    std::unique_ptr<unsigned[]> players_node;
    std::unique_ptr<node_information[]> players_node_data;
    std::unique_ptr<node_information[]> simulation_players_node_data;
    std::unique_ptr<unsigned[]> simulation_players_move;

    unsigned seed;
};

