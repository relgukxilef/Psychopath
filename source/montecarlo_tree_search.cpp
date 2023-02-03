#include "montecarlo_tree_search.h"

#include "../submodules/doctest/doctest/doctest.h"

struct tic_tac_toe {
    typedef uint32_t position_t;
    static const int move_size = 1;
    static const int player_size = 2;
    static const int position_digest_size = 1;

    static constexpr uint64_t board = 0b111'111'111;
    static constexpr int player_o = 0, player_x = 9;
    static constexpr uint64_t
        vertical = 0b001'001'001,
        horizontal = 0b111,
        diagonal_0 = 0b001'010'100,
        diagonal_1 = 0b100'010'001;

    position_t start() { return 0; };
    int player(position_t p) { return std::popcount(p) % 2; }
    void moves(position_t p, uint64_t* moves) {
        // check if someone won
        int scores[2];
        score(p, scores);
        moves[0] = 0;
        if (scores[0] == 1) // no winner yet
            moves[0] = board & ~((p >> player_o) | (p >> player_x));
    }
    void score(position_t p, int* scores) {
        int score_x = 1; // 1 = draw
        for (int player = 0; player < 2; player++) {
            for (int i = 0; i < 3; i++) {
                if (
                    (((p >> i) & vertical) == vertical) |
                    (((p >> i * 3) & horizontal) == horizontal)
                ) {
                    score_x = 2 * player;
                }
            }
            if (
                (p & diagonal_0) == diagonal_0 |
                (p & diagonal_1) == diagonal_1
            ) {
                score_x = 2 * player;
            }
            p >>= player_x;
        }
        scores[0] = 2 - score_x;
        scores[1] = score_x;
    }
    position_t play(position_t p, int move, uint64_t* color) {
        color[0] = move;
        color[1] = move;
        return p | (1 << (move + player(p) * player_x));
    }
    void digest(position_t p, uint64_t* d) { d[0] = p; };

    void print(position_t p) {
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                if (p & 1) {
                    printf("o");
                } else if (p & (1 << player_x)) {
                    printf("x");
                } else {
                    printf(" ");
                }
                p >>= 1;
            }
            printf("\n");
        }
    }
};

TEST_CASE("tic_tac_toe") {
    tic_tac_toe game;
    monte_carlo_tree_search<tic_tac_toe> search(game);
    monte_carlo_tree_search_thread<tic_tac_toe> thread(search);

    tic_tac_toe::position_t p = game.start();
    uint64_t colors[2];
    uint64_t moves;
    game.moves(p, &moves);
    CHECK(moves == 0b111'111'111);
    p = game.play(p, 0, colors);
    game.moves(p, &moves);
    CHECK(moves == 0b111'111'110);
    for (int i = 1; i < 7; i++) {
        p = game.play(p, i, colors);
    }
    CHECK(p == 0b000101010'001010101);
    int scores[2];
    game.score(p, scores);
    CHECK(scores[0] == 2);
    CHECK(scores[1] == 0);
    game.moves(p, &moves);
    CHECK(moves == 0);
    game.print(p);

    printf("\n");

    thread.step(1000);
    CHECK(thread.get_best_move() == 4); // Center

    game.print(thread.start_position);

    for (int i = 0; i < 50; i++) {
        thread.play(thread.get_best_move());
        game.print(thread.start_position);
        printf("--- %d\n\n", i);
        thread.step(10000);
    }
}
