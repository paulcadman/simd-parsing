#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>

enum category {
  // ',' = 0x2c
  CAT_COMMA = 1u << 0,
  // ':' = 0x3a
  CAT_COLON = 1u << 1,
  // '[', ']', '{', '}' = 0x5b, 0x5d, 0x7b, 0x7d
  CAT_BRACKET = 1u << 2,
  // '\t', '\n', '\r' = 0x09, 0x0a, 0x0d
  CAT_WS = 1u << 3,
  // ' ' = 0x20
  CAT_SPACE = 1u << 4
};

static uint8_t LOW_NIBBLES[16] = {
    /* 0 */ CAT_SPACE, // 0x20
    /* 1 */ 0,
    /* 2 */ 0,
    /* 3 */ 0,
    /* 4 */ 0,
    /* 5 */ 0,
    /* 6 */ 0,
    /* 7 */ 0,
    /* 8 */ 0,
    /* 9 */ CAT_WS,               // 0x0
    /* a */ CAT_COLON | CAT_WS,   // 0x3a, 0x0a
    /* b */ CAT_BRACKET,          // 0x5b, 0x7b
    /* c */ CAT_COMMA,            // 0x2c
    /* d */ CAT_BRACKET | CAT_WS, // 0x5d, 0x7d, 0x0d
    /* e */ 0,
    /* f */ 0};

static uint8_t HIGH_NIBBLES[16] = {
    /* 0 */ CAT_WS, // 0x09, 0x0a, 0x0d
    /* 1 */ 0,
    /* 2 */ CAT_COMMA | CAT_SPACE, // 0x2c, 0x20
    /* 3 */ CAT_COLON,             // 0x3a
    /* 4 */ 0,
    /* 5 */ CAT_BRACKET, // 0x5b, 0x5d
    /* 6 */ 0,
    /* 7 */ CAT_BRACKET, // 0x7b, 0x7d
    /* 8 */ 0,
    /* 9 */ 0,
    /* a */ 0,
    /* b */ 0,
    /* c */ 0,
    /* d */ 0,
    /* e */ 0,
    /* f */ 0};

uint8x16_t classify(uint8x16_t input) {
    uint8x16_t low_nibbles = vld1q_u8(LOW_NIBBLES);
    uint8x16_t high_nibbles = vld1q_u8(HIGH_NIBBLES);

    // Apply `b & 0x0f` to each input byte
    uint8x16_t mask = vdupq_n_u8(0x0f);
    uint8x16_t low_bytes = vandq_u8(input, mask);
    // Apply `b >> 4` to each input byte
    uint8x16_t high_bytes = vshrq_n_u8(input, 4);

    // low_lookup[i] = low_nibbles[low_byes[i]]
    uint8x16_t low_lookup = vqtbl1q_u8(low_nibbles, low_bytes);
    // high_lookup[i] = high_nibbles[high_byes[i]]
    uint8x16_t high_lookup = vqtbl1q_u8(high_nibbles, high_bytes);

    // low_lookup[i] & high_lookup[i]
    return vandq_u8(low_lookup, high_lookup);
}

// This is adapted from _mm_movemask_epi8 in
//https://developer.arm.com/community/arm-community-blogs/b/servers-and-cloud-computing-blog/posts/porting-x86-vector-bitmask-optimizations-to-arm-neon
uint16_t make_bitmask(uint8x16_t classified) {
    uint8x16_t indicator = vcgtq_u8(classified, vdupq_n_u8(0));
    uint16x8_t high_bits = vreinterpretq_u16_u8(vshrq_n_u8(indicator, 7));
    uint32x4_t paired16 = vreinterpretq_u32_u16(vsraq_n_u16(high_bits, high_bits, 7));
    uint64x2_t paired32 = vreinterpretq_u64_u32(vsraq_n_u32(paired16, paired16, 14));
    uint8x16_t paired64 = vreinterpretq_u8_u64(vsraq_n_u64(paired32, paired32, 28));
    return (uint16_t)vgetq_lane_u8(paired64, 0) | ((uint16_t)vgetq_lane_u8(paired64, 8) << 8);
}

size_t get_indices_loop(uint8x16_t classified, uint8_t* indices) {
    uint8x16_t mask = vdupq_n_u8(0);
    // indicator[i] = if classified[i] == 0 then 0x00 else 0xFF
    uint8x16_t indicator = vcgtq_u8(classified, mask);

    uint8_t indicator_out[16] = {};
    vst1q_u8(indicator_out, indicator);

    // compress indicator bytes into a 16-bit bitset (1 byte becomes 1 bit).
    // bits are stored in reverse order so that the trailing zeros gives the correct
    // index (compatible with the ctz = count trailing zeros instruction)
    uint16_t bitset = 0;
    for (size_t i = 0; i < 16; i++) {
        bitset |= (indicator_out[i] >> 7) << i;
    }

    // extract the indices of the set bits in the bitset
    size_t num_indices = 0;
    while (bitset != 0) {
        uint8_t idx = __builtin_ctz(bitset);
        indices[num_indices++] = idx;
        // Clear the least-significant set bit
        bitset &= bitset - 1;
    }
    return num_indices;
}

size_t get_indices_movemask(uint8x16_t classified, uint8_t* indices) {
    uint16_t bitset = make_bitmask(classified);

    size_t num_indices = 0;
    while (bitset != 0) {
        uint8_t idx = __builtin_ctz(bitset);
        indices[num_indices++] = idx;
        bitset &= bitset - 1;
    }
    return num_indices;
}

int main(void) {
    uint8_t input[16] = "hello[world,foo ";
    uint8x16_t bytes = vld1q_u8(input);

    uint8x16_t classified = classify(bytes);

    uint8_t out[16] = {};
    vst1q_u8(out, classified);

    uint8_t loop_indices[16] = {};
    size_t num_loop_indices = get_indices_loop(classified, loop_indices);

    uint8_t movemask_indices[16] = {};
    size_t num_movemask_indices = get_indices_movemask(classified, movemask_indices);

    printf("---------LOW_NIBBLES----------\n");
    for (size_t i = 0; i < 16; i++) {
        printf("%02d ", LOW_NIBBLES[i]);
    }
    printf("\n");
    printf("\n");

    printf("---------HIGH_NIBBLES----------\n");
    for (size_t i = 0; i < 16; i++) {
        printf("%02d ", HIGH_NIBBLES[i]);
    }
    printf("\n");
    printf("\n");

    printf("---------input classification----------\n");
    for (size_t i = 0; i < 16; i++) {
        printf("%02zu '%c' -> 0x%02x\n", i, input[i], out[i]);
    }
    printf("\n");
    printf("\n");

    printf("---------loop indices----------\n");
    for (size_t i = 0; i < num_loop_indices; i++) {
        printf("%02d -> '%c'\n", loop_indices[i], input[loop_indices[i]]);
    }
    printf("\n");
    printf("\n");

    printf("---------movemask indices----------\n");
    for (size_t i = 0; i < num_movemask_indices; i++) {
        printf("%02d -> '%c'\n", movemask_indices[i], input[movemask_indices[i]]);
    }
}
