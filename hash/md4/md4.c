#include <stdint.h>
#include <stdlib.h>

#define ROUNDS 3
#define STEPS 16

#define BLOCK_BYTES 64 /* Tamaño de bloque: 512 bits. */
#define BLOCK_WORDS 16 /* 64 bytes / 4 bytes por palabra. */
#define WORD_BYTES 4   /* Tamaño de la palabra: 32 bits. */

#define BUFFER_LEN 4 /* Tamaño del buffer: 4 palabras. */
#define START_LENGTH 56

typedef uint32_t word;
typedef uint8_t byte;
typedef word block[BLOCK_WORDS];
typedef word (*round_function)(word, word, word);

#define ROTL(X, i) ((X) << (i)) | ((X) >> (32 - (i)))
#define BYTE_2_WORD(b) ((b[0] | b[1] << 8 | b[2] << 16 | b[3] << 24))

static const word IV[BUFFER_LEN] = {
    0x67452301,  // Word A.
    0xEFCDAB89,  // Word B.
    0x98BADCFE,  // Word C.
    0x10325476,  // Word D.
};

static const word K[ROUNDS] = {
    0x00000000,  // K_1.
    0x5A827999,  // K_2.
    0x6ED9EBA1,  // K_3.
};

// F functions.
static word f_func(word B, word C, word D) {return ((B & C) | ((~B) & D));}
static word g_func(word B, word C, word D) {return ((B & C) | (B & D) | (C & D));}
static word h_func(word B, word C, word D) {return (B ^ C ^ D);}
static const round_function F[ROUNDS] = {
    f_func,
    g_func,
    h_func,
};

static const size_t G_WORD[ROUNDS][STEPS] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15},
    {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15},
};

static const size_t S_SHIFT[ROUNDS][STEPS] = {
    {3, 7, 11, 19, 3, 7, 11, 19, 3, 7, 11, 19, 3, 7, 11, 19},
    {3, 5, 9, 13, 3, 5, 9, 13, 3, 5, 9, 13, 3, 5, 9, 13},
    {3, 9, 11, 15, 3, 9, 11, 15, 3, 9, 11, 15, 3, 9, 11, 15},
};

void md4_compression(word* block, word* buffer);

int md4(const byte* message, uint64_t length, byte* digest) {
    block* message_in_blocks = NULL;  // El mensaje formateado en bloques.
    word* current_block = NULL;       // Apunta al bloque que se esta procesando.
    const byte* tail = message;       // Apunta al byte del mensaje que se esta procesando.

    size_t num_blocks = (length / BLOCK_BYTES) + 1;
    /* Mín. nº de bloques para alojar el mensaje. Si length % BLOCK_BYTES fuera
    0 no seria necesario el + 1 para alojar el contenido del mensaje, pero
    seguiría siendo necesario para el padding.*/

    size_t full_blocks = length / BLOCK_BYTES;        // Número de bloques completos.
    size_t last_block_length = length % BLOCK_BYTES;  // Tamaño del último bloque en bytes.
    uint64_t length_b = (uint64_t) length << 3;       // Tamaño del mensaje en bits para el
                                                      // padding (X << 3 equivale a X * 8).

    word buffer[BUFFER_LEN] = {IV[0], IV[1], IV[2], IV[3]};

    if (last_block_length >= START_LENGTH) {
        // Si el padding no cabe en el espacio del último bloque, se añade otro bloque.
        num_blocks++;
    }

    message_in_blocks = (block*)calloc(num_blocks, sizeof(block));
    if (message_in_blocks == NULL) {
        return -1;
    }

    // Se cargan los bloques completos.
    for (size_t block_i = 0; block_i < full_blocks; block_i++) {
        current_block = message_in_blocks[block_i];

        // Se carga cada palabra del bloque que se esta procesando.
        for (size_t word_i = 0; word_i < BLOCK_WORDS; word_i++) {
            current_block[word_i] = BYTE_2_WORD(tail);

            // Cada vez que se procesa una palabras, se desplaza el puntero.
            tail += WORD_BYTES;
        }
    }

    // Se cargan los bloques finales.
    if (last_block_length == 0) {
        // Si es 0 significa que el tamaño del mensaje es múltiplo del tamaño de bloque, por lo que
        // ya no hay mensaje y solo queda añadir el padding.

        current_block = message_in_blocks[num_blocks - 1];

        // Se pone el bit 1 al principio.
        current_block[0] = 0X00000080;
        // Se pone el tamaño del mensaje original al final.
        current_block[BLOCK_WORDS - 2] = length_b;
        current_block[BLOCK_WORDS - 1] = length_b >> 32;

    } else if (last_block_length < START_LENGTH) {
        // Significa que lo que falta de procesar del mensaje cabe en un bloque junto al padding.

        current_block = message_in_blocks[num_blocks - 1];

        // Se procesa lo que quede el mensaje.
        for (size_t byte_i = 0; byte_i < last_block_length; byte_i++) {
            current_block[byte_i / 4] |= (word)(*tail) << (8 * (byte_i % 4));

            // Cada vez que se procesa una byte, se desplaza el puntero.
            tail++;
        }

        // Se concatena el bit bit 1.
        current_block[last_block_length / 4] |= 0x80 << (8 * (last_block_length % 4));
        // Se pone el tamaño del mensaje original al final.
        current_block[BLOCK_WORDS - 2] = length_b;
        current_block[BLOCK_WORDS - 1] = length_b >> 32;

    } else {
        // Significa que lo que falta de procesar del mensaje no cabe en un bloque junto al
        // padding por lo que el padding se dividirá en 2 bloques.

        // El primer bloque tendrá lo que falta de mensaje, el bit 1 y 0s.
        current_block = message_in_blocks[num_blocks - 2];

        // Se procesa lo que quede el mensaje.
        for (size_t byte_i = 0; byte_i < last_block_length; byte_i++) {
            current_block[byte_i / 4] |= (word)(*tail) << (8 * (byte_i % 4));

            // Cada vez que se procesa una byte, se desplaza el puntero.
            tail++;
        }

        // Se concatena el bit bit 1.
        current_block[last_block_length / 4] |= 0x80 << (8 * (last_block_length % 4));

        // El segundo bloque esta vacío, salvo por el tamaño del mensaje original.
        current_block = message_in_blocks[num_blocks - 1];
        current_block[BLOCK_WORDS - 2] = length_b;
        current_block[BLOCK_WORDS - 1] = length_b >> 32;
    }

    // Se comprimen los bloques.
    for (size_t i = 0; i < num_blocks; i++) {
        md4_compression(message_in_blocks[i], buffer);
    }

    // Se convierte el buffer en el digest.
    for (size_t i = 0; i < BUFFER_LEN; i++) {
        digest[i * WORD_BYTES] = (byte)(buffer[i]);
        digest[i * WORD_BYTES + 1] = (byte)(buffer[i] >> 8);
        digest[i * WORD_BYTES + 2] = (byte)(buffer[i] >> 16);
        digest[i * WORD_BYTES + 3] = (byte)(buffer[i] >> 24);
    }
    free(message_in_blocks);
    
    return 0;
}

void md4_compression(word* block, word* buffer) {
    word A = buffer[0];
    word B = buffer[1];
    word C = buffer[2];
    word D = buffer[3];

    word aux = 0x00000000;
    word Kt = 0x00000000;
    word (*Ft)(word, word, word);
    size_t g_i = 0;
    size_t s_i = 0;

    for (size_t t = 0; t < ROUNDS; t++) {
        Kt = K[t];
        Ft = F[t];

        for (size_t i = 0; i < STEPS; i++) {
            g_i = G_WORD[t][i];
            s_i = S_SHIFT[t][i];

            aux = ROTL(A + Ft(B, C, D) + block[g_i] + Kt, s_i);
            A = D;
            D = C;
            C = B;
            B = aux;
        }
    }

    buffer[0] += A;
    buffer[1] += B;
    buffer[2] += C;
    buffer[3] += D;
}
