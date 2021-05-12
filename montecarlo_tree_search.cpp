#include "montecarlo_tree_search.h"

#include <cmath>
#include <tuple>

inline unsigned get_random_number(unsigned& seed, unsigned size) {
    seed = seed * 1664525 + 1013904223;
    return seed % size;
}

inline float get_confidence(
    unsigned score, unsigned samples, unsigned parent_samples
) {
    return
        static_cast<float>(score) / samples +
        sqrtf(logf(parent_samples) / samples) * sqrtf(2.f);
}

bool local_edge::operator==(const local_edge &o) const {
    return
        std::tie(start, move, start_data) ==
        std::tie(o.start, o.move, o.start_data);
}

montecarlo_tree_search::montecarlo_tree_search(game_tree &tree) :
    tree(tree),
    nodes_visited{false,},
    local_statistics(tree.get_player_size(), {0, 0}),
    players_move(new unsigned[tree.get_player_size()]),
    players_node(new unsigned[tree.get_player_size()]),
    players_node_data(new node_information[tree.get_player_size()]),
    simulation_players_node_data(new node_information[tree.get_player_size()]),
    simulation_players_move(new unsigned[tree.get_player_size()])
{

}

void montecarlo_tree_search::step() {
    // select
    unsigned node = 0;
    for (unsigned i = 0; i < tree.get_player_size(); i++) {
        players_node[i] = i;
        depths_players_local.push_back(i);
    }
    tree.get_data(node, players_node_data.get());
    while(nodes_visited[node] && players_node_data[0].moves_size > 0) {
        // this loop could operate on cached nodes but not doing so allows
        // stochastic game_trees

        for (unsigned player = 0; player < tree.get_player_size(); player++) {
            auto player_node = players_node[player];
            unsigned parent_count = local_statistics[player_node].count;

            unsigned best_move;
            float best_confidence;
            for (
                unsigned i = 0; i < players_node_data[player].moves_size; i++
            ) {
                // TODO: Reifsteck et al. calculate the minimum confidence over
                // indistinguishable nodes. This would require back-references
                // from player_nodes to nodes.
                auto child = local_children.at(
                    {player_node, i, players_node_data[player]}
                );
                // TODO: stochastic game_trees may return unknown edge
                auto statistic = local_statistics[child];
                float confidence = get_confidence(
                    statistic.sum, statistic.count, parent_count
                );
                if (confidence > best_confidence || i == 0) {
                    best_confidence = confidence;
                    best_move = i;
                    players_node[player] = child;
                }
            }

            players_move[player] = best_move;
            depths_players_local.push_back(players_node[player]);
        }

        node = tree.get_child(node, players_move.get());
        nodes_visited.resize(node + 1);

        tree.get_data(node, players_node_data.get());
    }

    nodes_visited[node] = true;

    for (unsigned i = 0; i < tree.get_player_size(); i++) {
        players_move[i] = 0;
    }
    while (players_move[0] < players_node_data[0].moves_size) {
        // expand (joint with sim and prop)
        for (unsigned p = 0; p < tree.get_player_size(); p++) {
            // move this loop out of while
            local_edge edge{
                players_node[p], players_move[p],
                players_node_data[p]
            };

            auto e = local_children.find(edge);
            if (e == local_children.end()) {
                unsigned new_node = local_statistics.size();
                local_statistics.push_back({0, 0});
                e = local_children.insert(e, {edge, new_node});
            }

            depths_players_local.push_back(e->second);
        }

        // simulation
        unsigned child = tree.get_child(node, players_move.get());
        tree.get_data(child, simulation_players_node_data.get());

        unsigned simulation_node = child;
        while (simulation_players_node_data[0].moves_size > 0) {
            // randomly pick moves
            for (unsigned p = 0; p < tree.get_player_size(); p++) {
                simulation_players_move[p] = get_random_number(
                    seed, simulation_players_node_data[p].moves_size
                );
            }

            simulation_node =
                tree.get_child(simulation_node, simulation_players_move.get());
            tree.get_data(simulation_node, simulation_players_node_data.get());
        }

        // backpropagation
        for (unsigned n = 0; n < depths_players_local.size();) {
            for (unsigned p = 0; p < tree.get_player_size(); ++p, ++n) {
                local_statistics[depths_players_local[n]].count++;
                local_statistics[depths_players_local[n]].sum +=
                    simulation_players_node_data[p].score;
            }
        }
        for (unsigned p = 0; p < tree.get_player_size(); ++p) {
            depths_players_local.pop_back();
        }

        // next possible combination move
        unsigned p = tree.get_player_size() - 1;
        players_move[p]++;
        while (players_move[p] == players_node_data[p].moves_size && p > 0) {
            players_move[p] = 0;
            p--;
            players_move[p]++;
        }
    }

    depths_players_local.clear();
}
