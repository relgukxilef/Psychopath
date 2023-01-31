#include <cinttypes>
#include <vector>
#include <memory>
#include <initializer_list>

struct value_type_t {
    uint32_t size;
    uint8_t bits;
};

struct graph_t;

struct value_t {
    uint32_t operation;
    value_type_t type;
    graph_t* graph;

    value_t bits() const;
    value_t slice(int begin, int end);
    value_t operator[] (int index);
    value_t operator[] (const value_t& index);
    value_t operator~();
};

value_t operator<<(int left, const value_t& right);
value_t operator<<(const value_t& left, int right);
value_t operator>>(const value_t& left, int right);
value_t operator*(int left, const value_t& right);
value_t operator*(const value_t& left, int right) {
    return right * left;
};
value_t operator+(int left, const value_t& right);
value_t operator+(const value_t& left, int right) {
    return right + left;
};
value_t operator+(const value_t& left, const value_t& right);
value_t operator-(int left, const value_t& right);
value_t operator/(const value_t& left, uint64_t right);
value_t operator%(const value_t& left, uint64_t right);
value_t operator|(const value_t& left, const value_t& right);
value_t operator&(const value_t& left, const value_t& right);
value_t operator&(uint64_t left, const value_t& right);
value_t operator&(const value_t& left, uint64_t right) {
    return right & left;
}
value_t operator==(int left, const value_t& right);
value_t operator==(const value_t& left, int right) {
    return right == left;
};
value_t operator==(const value_t& left, const value_t& right);

value_t condition(const value_t&, const value_t&, const value_t&);
value_t condition(const value_t&, int, const value_t&);
value_t condition(const value_t&, const value_t&, int);
value_t condition(const value_t&, int, int);

struct variable_t : public value_t {
};

struct operation_t {
    virtual ~operation_t() {}
};

struct graph_t {
    std::vector<uint32_t> operations_first_dependency;
    std::vector<uint32_t> dependencies_operation;
    std::vector<value_type_t> state;
    std::vector<uint64_t> initial_state;
    std::vector<std::unique_ptr<operation_t>> operations;

    variable_t variable(int size, int bits);
    value_t value(int size, int bits);
    value_t constant(uint64_t value);
    value_t constant(std::initializer_list<uint64_t> value);
};

struct transition_t {
    variable_t variable;
    value_t next_value;
    value_t initial_value;
};

struct game_t {
    game_t(
        value_t player, value_t moves, 
        std::initializer_list<value_t> scores, 
        variable_t move, 
        std::initializer_list<transition_t> transitions
    );
    void play();
};

int from_xy(int x, int y) {
    return x + y * 8;
}

value_t board_bits(value_t position) {
    return (1 << position).bits();
}

uint64_t board_bits(int x, int y) {
    return 1ul << from_xy(x, y);
}

value_t move_index(value_t piece, value_t to) {
    return piece * 64 + to;
}

int main() {
    graph_t g;

    const int 
        PAWNS = 0, ROOKS = 8, BISHOPS = 10, KNIGHTS = 12, QUEEN = 14, KING = 15;
    const int WHITE = 0, BLACK = 16;
    const int PIECES = 16, BOARD = 64;

    uint64_t x_0 = 0, x_7 = 0, x_01 = 0, x_67 = 0;
    for (int i = 0; i < 8; i++) {
        x_0 |= board_bits(0, i);
        x_7 |= board_bits(7, i);
        x_01 |= board_bits(1, i);
        x_67 |= board_bits(6, i);
    }
    x_01 |= x_0;
    x_67 |= x_7;

    variable_t pieces = g.variable(/*size*/ PIECES, /*bits*/ 6);

    value_t white_occupied = board_bits(pieces.slice(0, 16));
    value_t black_occupied = board_bits(pieces.slice(16, 32));
    value_t occupied = white_occupied | black_occupied;

    variable_t turn = g.variable(1, 1);

    value_t self = condition(turn == 1, WHITE, BLACK);
    value_t opponent = condition(turn == 1, BLACK, WHITE);
    value_t self_occupied = 
        condition(turn == 1, white_occupied, black_occupied);
    value_t opponent_occupied =
        condition(turn == 1, black_occupied, white_occupied);


    value_t moves = g.value(PIECES, BOARD);
    // pawn moves
    for (int i = 0; i < 8; i++) {
        value_t p = board_bits(pieces[PAWNS + self + i]);
        
        value_t moved = condition(
            turn == 1,
            p << 8,
            p >> 8
        ) & ~occupied;
        
        value_t captured = condition(
            turn == 1,
            (p << 7) & x_7 | (p << 9) & x_0,
            (p >> 7) & x_0 | (p >> 9) & x_7
        ) & opponent_occupied;

        // TOOO: promotion

        moves[PAWNS + self + i] = moved | captured;
    }

    // TODO: check for check
    value_t check = g.constant(0);

    // same as graph_t::value, but determines size from mask
    variable_t move = g.variable(1, PIECES * BOARD);
    //g.move(/*player*/ turn, /*possible moves*/ moves);

    value_t piece = move / BOARD;
    value_t to = move % BOARD;

    value_t next_pieces = pieces;
    next_pieces[piece] = to;

    value_t score = condition(check, turn * 2, 1);

    game_t chess = game_t(
        turn, // current player
        moves, // possible moves
        {score, 2 - score}, // score for each player
        move, // move to make
        { // state transitions and initial state
            transition_t{ pieces, next_pieces, g.constant({
                8, 9, 10, 11, 12, 13, 14, 15, 
                0, 7, 
                2, 5,
                1, 6,
                3, 
                4,

                48, 49, 50, 51, 52, 53, 54, 55,
                56, 63,
                58, 61,
                57, 62,
                59, 
                60
            }) },
            { turn, ~turn, g.constant(1) }
        }
    );

    chess.play();
}
