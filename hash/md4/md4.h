#ifndef MD4_H
#define MD4_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Computes the MD4 hash of a message.
 *
 * @param message Pointer to the input message to be hashed.
 * @param length Length of the input message in bytes.
 *
 * @return Pointer to a dynamically allocated 16-byte buffer containing
 *         the MD4 digest in little-endian format, or NULL if memory
 *         allocation fails.
 */
uint8_t* md4(uint8_t* message, size_t length);

#endif