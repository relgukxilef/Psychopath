#pragma once

/**
 * @brief The player_node struct represents a players view of a node.
 * Each node has 0 or more children, one color and a score per player.
 * Two nodes are indistinguishable for a player if they have the same color.
 * The score is only relevant for terminal nodes.
 */
struct node_information {
    bool operator== (const node_information&) const;

    unsigned color, moves_size, score;
};

/**
 * @brief The game_tree struct allows iterating over a game tree.
 * A tree may be stochastic, meaning that get_child is non-determinstic.
 * Node 0 is the root.
 */
struct game_tree {
    game_tree() = delete;
    game_tree(unsigned player_size) : player_size(player_size) {}
    virtual ~game_tree() = default;

    virtual void get_data(unsigned node, node_information* player_nodes) = 0;
    virtual unsigned get_child(unsigned node, unsigned* moves) = 0;

    unsigned get_player_size() { return player_size; }

private:
    unsigned player_size;
};
