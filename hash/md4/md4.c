#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_ROUNDS 48

#define WORDS_PER_BLOCK 16
#define BYTES_PER_BLOCK 64
#define BYTES_PER_WORDS 4
#define BUFFER_SIZE 4
#define START_PADDING 56

#define F(B, C, D) ((B & C) | ((~B) & D))
#define G(B, C, D) ((B & C) | (B & D) | (C & D))
#define H(B, C, D) (B ^ C ^ D)
#define LEFT_ROT(X, i) ((X) << (i)) | ((X) >> (32 - (i)))

#define BYTE_2_WORD(b) (b[0] | b[1] << 8 | b[2] << 16 | b[3] << 24)

typedef uint32_t word;
typedef uint8_t byte;

const static word iv[BUFFER_SIZE] = {
    0x67452301,  // Word A.
    0xEFCDAB89,  // Word B.
    0x98BADCFE,  // Word C.
    0x10325476,  // Word D.
};

const static size_t g[NUM_ROUNDS] = {
    0, 1, 2, 3,  4, 5,  6, 7,  8, 9, 10, 11, 12, 13, 14, 15,
    0, 4, 8, 12, 1, 5,  9, 13, 2, 6, 10, 14, 3,  7,  11, 15,
    0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5,  13, 3,  11, 7,  15,
};

const static size_t s[NUM_ROUNDS] = {
    3, 7, 11, 19, 3, 7, 11, 19, 3, 7, 11, 19, 3, 7, 11, 19,
    3, 5, 9,  13, 3, 5, 9,  13, 3, 5, 9,  13, 3, 5, 9,  13,
    3, 9, 11, 15, 3, 9, 11, 15, 3, 9, 11, 15, 3, 9, 11, 15,
};

/**
 * @brief Performs the MD4 compression function on a 512-bit block.
 *
 * Processes the 48 operations of the three MD4 rounds and updates the
 * internal state consisting of four 32-bit words.
 *
 * @param block 512-bit message block represented as 16 32-bit words
 *              in little-endian format.
 * @param buffer MD4 internal state consisting of four 32-bit words.
 */
void md4_compression(word* block, word* buffer);

/**
 * @brief Adds the MD4 padding and message length and processes the final block.
 *
 * Builds the final block or blocks required by the MD4 specification by
 * appending the padding bit, the required zero bits, and the original
 * message length in bits as a 64-bit little-endian value.
 *
 * @param message Pointer to the beginning of the remaining message data after
 *                all complete blocks have been processed.
 * @param length Total length of the original message in bytes.
 * @param buffer MD4 internal state, which is updated with the final block(s).
 */
void md4_padding(byte* message, size_t length, word* buffer);

/**
 * @brief Finalizes the MD4 computation and generates the resulting digest.
 *
 * Allocates memory for the 16-byte MD4 digest and copies the final internal
 * state into it in little-endian format.
 *
 * @param buffer Final MD4 internal state consisting of four 32-bit words.
 * @param output Pointer where the address of the generated digest is stored.
 *               The caller is responsible for freeing the allocated memory.
 */
void md4_finalization(word* buffer, byte** output);

byte* md4(byte* message, size_t length) {
    word block[WORDS_PER_BLOCK];
    word buffer[BUFFER_SIZE] = {iv[0], iv[1], iv[2], iv[3]};
    byte* output = NULL;

    // Process full blocks.
    size_t final_block = length / BYTES_PER_BLOCK;
    for (size_t block_i = 0; block_i < final_block; block_i++) {
        // Load block.
        for (size_t word_i = 0, offset; word_i < WORDS_PER_BLOCK; word_i++) {
            offset = block_i * BYTES_PER_BLOCK + word_i * BYTES_PER_WORDS;
            block[word_i] = BYTE_2_WORD((message + offset));
        }

        md4_compression(block, buffer);
    }

    md4_padding((message + (final_block * BYTES_PER_BLOCK)), length, buffer);
    md4_finalization(buffer, &output);

    return output;
}

void md4_compression(word* block, word* buffer) {
    word A = buffer[0];
    word B = buffer[1];
    word C = buffer[2];
    word D = buffer[3];

    word Kt = 0x00000000;
    word Ft = 0x00000000;
    word Mg = 0x00000000;
    size_t Si = 0;

    for (size_t i = 0; i < NUM_ROUNDS; i++) {
        if (i < 16) {
            Kt = 0x00000000;
            Ft = F(B, C, D);
        } else if (i < 32) {
            Kt = 0x5A827999;
            Ft = G(B, C, D);
        } else {
            Kt = 0x6ED9EBA1;
            Ft = H(B, C, D);
        }
        Mg = block[g[i]];
        Si = s[i];

        Ft = A + Ft + Mg + Kt;
        Ft = LEFT_ROT(Ft, Si);

        A = D;
        D = C;
        C = B;
        B = Ft;
    }

    buffer[0] += A;
    buffer[1] += B;
    buffer[2] += C;
    buffer[3] += D;
}

void md4_padding(byte* message, size_t length, word* buffer) {
    word block[WORDS_PER_BLOCK];
    uint64_t size = length << 3;
    size_t block_size = length % BYTES_PER_BLOCK;
    size_t i;

    for (i = 0; i < WORDS_PER_BLOCK; i++) {
        block[i] = 0x00000000;
    }

    if (block_size == 0) {
        // There is no block. The message size matches the block size.
        block[0] = 0X00000080;
        block[WORDS_PER_BLOCK - 2] = size;
        block[WORDS_PER_BLOCK - 1] = size >> 32;
        md4_compression(block, buffer);

    } else if (block_size < START_PADDING) {
        // Block with space for padding.

        for (i = 0; i < block_size; i++) {
            block[i / 4] += message[i] << (8 * (i % 4));
        }
        block[i / 4] += 0x80 << (8 * (i % 4));

        block[WORDS_PER_BLOCK - 2] = size;
        block[WORDS_PER_BLOCK - 1] = size >> 32;
        md4_compression(block, buffer);

    } else {
        // Block with no space for padding. A block is added to provide space
        // for it.
        for (i = 0; i < block_size; i++) {
            block[i / 4] += message[i] << (8 * (i % 4));
        }
        block[i / 4] += 0x80 << (8 * (i % 4));
        md4_compression(block, buffer);

        for (i = 0; i < WORDS_PER_BLOCK; i++) {
            block[i] = 0x00000000;
        }

        block[WORDS_PER_BLOCK - 2] = size;
        block[WORDS_PER_BLOCK - 1] = size >> 32;
        md4_compression(block, buffer);
    }
}

void md4_finalization(word* buffer, byte** output) {
    *output = (byte*)malloc(BUFFER_SIZE * BYTES_PER_WORDS);
    if (*output == NULL) {
        return;
    };

    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        (*output)[i * BYTES_PER_WORDS] = (byte)(buffer[i]);
        (*output)[i * BYTES_PER_WORDS + 1] = (byte)(buffer[i] >> 8);
        (*output)[i * BYTES_PER_WORDS + 2] = (byte)(buffer[i] >> 16);
        (*output)[i * BYTES_PER_WORDS + 3] = (byte)(buffer[i] >> 24);
    }
}
