#include "solver.h"

#include <algorithm>
#include <cinttypes>
#include <emmintrin.h>
#include <immintrin.h>
#include <cstdio>
#include <bit>

#include "../submodules/doctest/doctest/doctest.h"

// operators: & | >> << ~ * + - ==

uint32_t prefix_or(uint32_t x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x;
}

// returns the value >= a, such that value & b == c
uint32_t solve_and(uint32_t a, uint32_t b, uint32_t c) {
    // bits at or below the highest incorrect bit
    uint32_t mask = prefix_or((uint32_t)(a & b) ^ c);

    // set low bits to result
    uint32_t a_low = (a & ~mask) | (c & mask);
    if (a_low < a) {
        // if the result is smaller than a,
        // increment with carry over constrained bits
        a = (a_low | mask | b) + 1;
        return (a & ~b) | c;
    } else {
        return a_low;
    }
}

TEST_CASE("solver/and") {
    uint32_t b = 0b110110, c = 0b010010;

    CHECK(solve_and(0b000000, b, c) == 0b010010);
    CHECK(solve_and(0b000001, b, c) == 0b010010);
    CHECK(solve_and(0b010011, b, c) == 0b010011);
    CHECK(solve_and(0b010100, b, c) == 0b011010);

    for (uint32_t a = 0; a < 16; a++) {
        for (uint32_t b = 0; b < 16; b++) {
            for (uint32_t l = 0; l < 16; l++) {
                uint32_t c = (a & b);
                uint32_t i = l;
                while ((i & b) != c)
                    i++;
                CHECK(solve_and(l, b, c) == i);
            }
        }
    }
}

uint32_t solve_or(uint32_t a, uint32_t b, uint32_t c) {
    // for or the bits that are fixed and variable in a are inverted from and
    // need to mask c too to avoid unsolvable equations
    return solve_and(a, ~b, c & ~b);
}

TEST_CASE("solver/or") {
    for (uint32_t a = 0; a < 16; a++) {
        for (uint32_t b = 0; b < 16; b++) {
            for (uint32_t l = 0; l < 16; l++) {
                uint32_t c = (a | b);
                uint32_t i = l;
                while ((i | b) != c)
                    i = (i + 1) % 32;
                CHECK(solve_or(l, b, c) == i);
            }
        }
    }
}
