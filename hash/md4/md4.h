#ifndef MD4_H
#define MD4_H

#include <stdint.h>

int md4(uint8_t* message, uint64_t length, uint8_t* digest);

#endif