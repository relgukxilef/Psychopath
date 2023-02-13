#include <concepts>
#include <cinttypes>
#include <utility>
#include <memory>
#include <vector>
#include <bit>

template<typename Game>
concept Playable = requires(
    Game g, Game::position_t p, Game::position_t p2, uint64_t* moves, 
    int* scores, uint32_t move, uint64_t* colors, uint64_t* digest
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

uint16_t random(uint32_t& state) {
    return (state = 0x915f77f5u * state + 1u) >> 16;
}

template<Playable Game>
struct monte_carlo_tree_search_thread {
    monte_carlo_tree_search_thread(monte_carlo_tree_search<Game>& search) : 
        search(search) 
    {}
    void step(int steps);
    monte_carlo_tree_search<Game>& search;

    Game::position_t current_position;
    
    std::unique_ptr<uint64_t[]> moves;
    std::unique_ptr<uint64_t[]> colors;
    std::unique_ptr<uint64_t[]> digest;
    std::unique_ptr<int[]> scores;

    std::unique_ptr<uint8_t[]> moves_prefix_sum;
    
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
    thread.search.game.score(current_position, scores.get());
    for (int j = current_line.size() - 1; j >= 0; --j) {
        for (
            int player = 0; player < thread.search.game.player_size; ++player
        ) {
            positions_players_score_sum[
                current_line[j] * thread.search.game.player_size + player
            ] += scores[player];
        }
        ++positions_score_count[current_line[j]];
    }
}

int index_of_nth_one(uint64_t bits, int n) {
    while (n > 0) {
        bits &= ~(1ull << std::countr_zero(bits));
        n--;
    }
    return std::countr_zero(bits);
}

int random_one_bit(
    uint64_t* bits, int size, uint64_t* prefix_sum, uint32_t& random_state
) {
    prefix_sum[0] = std::popcount(bits[0]);
    for (int i = 1; i < size; ++i) {
        prefix_sum[i] = prefix_sum[i - 1] + std::popcount(bits[i]);
    }
    int bit_index = random(random_state) % prefix_sum[size - 1];
    int word_index = 0;
    int bit_offset = bit_index;
    while (bit_index >= prefix_sum[word_index]) {
        ++word_index;
        bit_offset -= prefix_sum[word_index];
    }
    return word_index * 64 + index_of_nth_one(bits[word_index], bit_offset);
}

template<Playable Game>
void monte_carlo_tree_search_thread<Game>::step(int steps) {
    for (int step = 0; step < steps; ++step) {
        // play a random move
        search.game.moves(current_position, moves.get());
        moves_prefix_sum[0] = std::popcount(moves[0]);
        for (int i = 0; i < search.game.move_size; ++i) {
            moves_prefix_sum[i] = 
                moves_prefix_sum[i - 1] + std::popcount(moves[i]);
        }
        int move = 
            random(random_state) % moves_prefix_sum[search.game.move_size - 1];

        if (moves_prefix_sum[search.game.move_size - 1] > 0) {
            int move_index = 0;
            while (move >= moves_prefix_sum[move_index]) {
                ++move_index;
            }
            move -= moves_prefix_sum[move_index - 1];
            current_position = 
                search.game.play(current_position, move_index, colors.get());

        } else {
            evaluate_game(*this);
            
            current_line.clear();
            current_position = search.game.start();

            // select next position to simulate
            int position = 0;
            while (positions_expanded[position]) {
                int best_position = -1;
                int best_score = -1;
                int best_child = -1;
                int player = search.game.player(current_position);
                for (
                    int child = positions_begin_child[position]; 
                    child < positions_end_child[position]; 
                    ++child
                ) {
                    if (positions_score_count[child] == 0) {
                        best_position = child;
                        break;
                    }
                    float score = 
                        float(positions_players_score_sum[
                            child * search.game.player_size + player
                        ]) / positions_score_count[child] + std::sqrtf(
                            2 * std::logf(
                                positions_score_count[position]
                            ) / positions_score_count[child]
                        );
                    if (score > best_score) {
                        best_score = score;
                        best_position = child;
                    }
                }
                if (best_position != -1) {
                    current_position = search.game.play(
                        current_position, positions_move[best_position], 
                        colors.get()
                    );
                    position = best_position;
                    current_line.push_back(best_position);

                } else {
                    evaluate_game(*this);
                    break;
                }
            }
            if (!positions_expanded[position]) {
                positions_expanded[position] = true;
                search.game.moves(current_position, moves.get());
                positions_begin_child[position] = positions_end_child.size();
                for (int i = 0; i < search.game.move_size; ++i) {
                    if (moves[i]) {
                        positions_end_child.push_back(0);
                        positions_begin_child.push_back(0);
                        positions_players_score_sum.push_back(0);
                        positions_score_count.push_back(0);
                        positions_move.push_back(i);
                        positions_expanded.push_back(false);
                    }
                }
                positions_end_child[position] = positions_end_child.size();
            }
        }
    }
}

struct chess_position {
    uint8_t pieces[32];
    bool turn;
};

struct chess {
    typedef chess_position position_t;
    static const int move_size = 16;
    static const int player_size = 2;
    static const int position_digest_size = 2;
    
    chess_position start();
    int player(chess_position);
    void moves(chess_position, uint64_t*);
    void score(chess_position, int*);
    chess_position play(chess_position, int, uint64_t*);
    void digest(chess_position, uint64_t*);
};

int main() {
    chess game;
    monte_carlo_tree_search<chess> chess_player(game);
    monte_carlo_tree_search_thread<chess> thread(chess_player);

    thread.step(1000);

    return 0;
}