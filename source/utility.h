#pragma once

#include <cinttypes>
#include <bit>
#include <memory>

inline uint16_t random(uint32_t& state) {
    return (state = 0x915f77f5u * state + 1u) >> 16;
}

int index_of_nth_one(uint64_t bits, int n);

void prefix_popcount(const uint64_t* bits, int* prefix_sum, int size);

int index_of_nth_one(
    const uint64_t* bits, int size, const int* prefix_sum, int bit_index
);

int random_one(
    const uint64_t* bits, const int* prefix_sum, int size,
    uint32_t& random_state
);
