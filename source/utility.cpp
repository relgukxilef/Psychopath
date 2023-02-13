#include "utility.h"

#include "../submodules/doctest/doctest/doctest.h"

int index_of_nth_one(uint64_t bits, int n) {
    while (n > 0) {
        bits &= ~(1ull << std::countr_zero(bits));
        n--;
    }
    return std::countr_zero(bits);
}

void prefix_popcount(const uint64_t *bits, int *prefix_sum, int size) {
    prefix_sum[0] = std::popcount(bits[0]);
    for (int i = 1; i < size; ++i) {
        prefix_sum[i] = prefix_sum[i - 1] + std::popcount(bits[i]);
    }
}

int index_of_nth_one(
    const uint64_t *bits, int size, const int *prefix_sum, int bit_index
) {
    int word_index = 0;
    int bit_offset = bit_index;
    while (bit_index >= prefix_sum[word_index]) {
        bit_offset = bit_index - prefix_sum[word_index];
        ++word_index;
    }
    return word_index * 64 + index_of_nth_one(bits[word_index], bit_offset);
}

int random_one(
    const uint64_t *bits, const int *prefix_sum, int size,
    uint32_t &random_state
) {
    int bit_index = random(random_state) % prefix_sum[size - 1];
    return index_of_nth_one(bits, size, prefix_sum, bit_index);
}

TEST_CASE("index_of_nth_one(uint64_t, int)") {
    CHECK(index_of_nth_one(0b1, 0) == 0);
    CHECK(index_of_nth_one(0b10, 0) == 1);
    CHECK(index_of_nth_one(0b11, 0) == 0);
    CHECK(index_of_nth_one(0b11, 1) == 1);
    CHECK(index_of_nth_one(0b110, 0) == 1);
    CHECK(index_of_nth_one(0b110, 1) == 2);
    CHECK(index_of_nth_one(0b1010, 0) == 1);
    CHECK(index_of_nth_one(0b1010, 1) == 3);
    CHECK(index_of_nth_one(0b1110, 1) == 2);
}

TEST_CASE("index_of_nth_one(uint64_t*, int, uint64_t*, int)") {
    auto bits = std::make_unique<uint64_t[]>(3);
    auto prefix_sum = std::make_unique<int[]>(3);

    bits[0] = 0b110;
    bits[1] = 0b1110;
    bits[2] = 0b1010;
    prefix_popcount(bits.get(), prefix_sum.get(), 3);
    CHECK(prefix_sum[0] == 2);
    CHECK(prefix_sum[1] == 5);
    CHECK(prefix_sum[2] == 7);
    CHECK(index_of_nth_one(bits.get(), 2, prefix_sum.get(), 0) == 1);
    CHECK(index_of_nth_one(bits.get(), 2, prefix_sum.get(), 1) == 2);
    CHECK(index_of_nth_one(bits.get(), 2, prefix_sum.get(), 2) == 64 + 1);
    CHECK(index_of_nth_one(bits.get(), 2, prefix_sum.get(), 3) == 64 + 2);
    CHECK(index_of_nth_one(bits.get(), 3, prefix_sum.get(), 5) == 2 * 64 + 1);
    CHECK(index_of_nth_one(bits.get(), 3, prefix_sum.get(), 6) == 2 * 64 + 3);
}

TEST_CASE("random_one") {
    auto bits = std::make_unique<uint64_t[]>(3);
    auto prefix_sum = std::make_unique<int[]>(3);

    bits[0] = 0b110;
    bits[1] = 0b1110;
    bits[2] = 0b1010;
    int histogram[3 * 64] = {};
    int ones[] = {1, 2, 64 + 1, 64 + 2, 64 + 3, 2 * 64 + 1, 2 * 64 + 3};
    uint32_t state = 0;
    int runs = 70;
    for (int i = 0; i < runs; i++) {
        prefix_popcount(bits.get(), prefix_sum.get(), 3);
        int bit = random_one(bits.get(), prefix_sum.get(), 3, state);
        histogram[bit]++;
    }
    int sum = 0;
    for (int one : ones) {
        CHECK(histogram[one] > 0);
        sum+=histogram[one];
    }
    CHECK(sum == runs);
}
