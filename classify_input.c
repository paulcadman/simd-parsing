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

int main(void) {
    uint8_t input[16] = "hello[world,foo ";
    uint8x16_t bytes = vld1q_u8(input);

    uint8x16_t classified = classify(bytes);

    uint8_t out[16] = {};
    vst1q_u8(out, classified);

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
}
