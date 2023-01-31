#pragma once

#include <concepts>
#include <cinttypes>
#include <utility>
#include <memory>
#include <vector>
#include <bit>
#include <cmath>

#include "utility.h"

template<typename Game>
concept Playable = requires(
    Game g, typename Game::position_t p, typename Game::position_t p2,
    uint64_t* moves, int* scores, uint32_t move, uint64_t* colors,
    uint64_t* digest
) {
    { g.move_size } -> std::convertible_to<int>;
    { g.player_size } -> std::convertible_to<int>;
    { g.position_digest_size } -> std::convertible_to<int>;
    { g.start() } -> std::same_as<typename Game::position_t>;
    { g.player(p) } -> std::same_as<int>;
    { g.moves(p, moves) } -> std::same_as<void>;
    { g.score(p, scores) } -> std::same_as<void>;
    { g.play(p, move, colors) } -> std::same_as<typename Game::position_t>;
    { g.digest(p, digest) } -> std::same_as<void>;
    { p = std::move(p2) };
};

template<Playable Game>
struct monte_carlo_tree_search {
    monte_carlo_tree_search(Game& game) : game(game) {}
    Game& game;
};

template<Playable Game>
struct monte_carlo_tree_search_thread {
    monte_carlo_tree_search_thread(monte_carlo_tree_search<Game>& search);
    void step(int steps);
    monte_carlo_tree_search<Game>& search;

    typename Game::position_t current_position;

    std::unique_ptr<uint64_t[]> moves;
    std::unique_ptr<uint64_t[]> colors;
    std::unique_ptr<uint64_t[]> digest;
    std::unique_ptr<int[]> scores;

    std::unique_ptr<int[]> moves_prefix_sum;

    std::vector<int> positions_begin_child;
    std::vector<int> positions_end_child;
    std::vector<int> positions_players_score_sum;
    std::vector<int> positions_score_count;
    std::vector<int> positions_move;
    std::vector<char> positions_expanded;

    // player-local game state needs to be stored in a B-tree or hash table
    // (local_position, color) -> (local_position, position)
    // need fast iteration over values with same key
    // need fast insertion and deletion
    std::vector<int> local_positions_parent;
    std::vector<int> local_positions_color;
    std::vector<int> local_positions_position;

    // storing heap of upper confidence bounds is not worth it because with each
    // simulation all values can change

    std::vector<int> current_line;

    uint32_t random_state;
};

template<Playable Game>
void evaluate_game(monte_carlo_tree_search_thread<Game>& thread) {
    thread.search.game.score(thread.current_position, thread.scores.get());
    for (int j = 0; j < thread.current_line.size(); j++) {
        for (
            int player = 0; player < thread.search.game.player_size; ++player
        ) {
            thread.positions_players_score_sum[
                thread.current_line[j] * thread.search.game.player_size + player
            ] += thread.scores[player];
        }
        ++thread.positions_score_count[thread.current_line[j]];
    }
}

template<Playable Game>
monte_carlo_tree_search_thread<Game>::monte_carlo_tree_search_thread(
    monte_carlo_tree_search<Game> &search
) : search(search) {
    moves = std::make_unique<uint64_t[]>(Game::move_size);
    colors = std::make_unique<uint64_t[]>(Game::player_size);
    digest = std::make_unique<uint64_t[]>(Game::position_digest_size);
    scores = std::make_unique<int[]>(Game::player_size);
    moves_prefix_sum = std::make_unique<int[]>(Game::move_size);

    current_position = search.game.start();
    random_state = 1;

    search.game.moves(current_position, moves.get());
    prefix_popcount(moves.get(), moves_prefix_sum.get(), Game::move_size);
    int move_count = moves_prefix_sum[Game::move_size - 1];

    positions_begin_child = {1};
    positions_end_child = {1 + move_count};
    positions_players_score_sum = std::vector<int>(Game::player_size, 0);
    positions_score_count = {0};
    positions_move = {-1};
    positions_expanded = {true};
    current_line = {0};

    // TODO: move this to function
    // TODO: remove inner loop in index_of_nth_one
    for (int i = 0; i < move_count; ++i) {
        int move_index = index_of_nth_one(
            moves.get(), Game::move_size, moves_prefix_sum.get(), i
        );
        positions_end_child.push_back(0);
        positions_begin_child.push_back(0);
        positions_players_score_sum.insert(
            positions_players_score_sum.end(),
            Game::player_size, 0
        );
        positions_score_count.push_back(0);
        positions_move.push_back(move_index);
        positions_expanded.push_back(false);
    }
}

template<Playable Game>
void monte_carlo_tree_search_thread<Game>::step(int steps) {
    for (int step = 0; step < steps; ++step) {
        // play a random move
        search.game.moves(current_position, moves.get());
        prefix_popcount(moves.get(), moves_prefix_sum.get(), Game::move_size);

        if (moves_prefix_sum[search.game.move_size - 1] > 0) {
            int move_index = random_one(
                moves.get(), moves_prefix_sum.get(), Game::move_size,
                random_state
            );
            current_position =
                search.game.play(current_position, move_index, colors.get());

        } else {
            evaluate_game(*this);

            current_line = {0};
            current_position = search.game.start();

            // select next position to simulate
            int position = 0;
            while (positions_expanded[position]) {
                int player = search.game.player(current_position);
                int children_count =
                    positions_end_child[position] -
                    positions_begin_child[position];

                if (children_count > 0) {
                    int best_position = -1;
                    float best_score = -1;
                    for (int i = 0; i < children_count; i++) {
                        // permute a little
                        int child =
                            positions_begin_child[position] +
                            (i + position) % children_count;

                        if (positions_score_count[child] == 0) {
                            best_position = child;
                            break;
                        }
                        float score =
                            float(positions_players_score_sum[
                                child * search.game.player_size + player
                            ]) / positions_score_count[child] + sqrtf(
                                2 * logf(positions_score_count[position]) /
                                positions_score_count[child]
                            );
                        if (score > best_score) {
                            best_score = score;
                            best_position = child;
                        }
                    }

                    current_position = search.game.play(
                        current_position, positions_move[best_position],
                        colors.get()
                    );
                    position = best_position;
                    current_line.push_back(best_position);

                } else {
                    break;
                }
            }
            if (!positions_expanded[position]) {
                positions_expanded[position] = true;

                search.game.moves(current_position, moves.get());
                prefix_popcount(
                    moves.get(), moves_prefix_sum.get(), Game::move_size
                );
                int move_count = moves_prefix_sum[Game::move_size - 1];

                positions_begin_child[position] = positions_begin_child.size();
                positions_end_child[position] =
                    positions_begin_child.size() + move_count;

                // TODO: move this to function
                // TODO: remove inner loop in index_of_nth_one
                for (int i = 0; i < move_count; ++i) {
                    int move_index = index_of_nth_one(
                        moves.get(), Game::move_size, moves_prefix_sum.get(), i
                    );
                    positions_end_child.push_back(-1);
                    positions_begin_child.push_back(-1);
                    positions_players_score_sum.insert(
                        positions_players_score_sum.end(),
                        Game::player_size, 0
                    );
                    positions_score_count.push_back(0);
                    positions_move.push_back(move_index);
                    positions_expanded.push_back(false);
                }
            }
        }
    }
}
