#ifndef FIPS202_H
#define FIPS202_H

#include <stddef.h>
#include <stdint.h>

#define SHAKE128_RATE 168
#define SHAKE256_RATE 136
#define SHA3_256_RATE 136
#define SHA3_512_RATE 72

#define FIPS202_NAMESPACE(s) pqcrystals_kyber_fips202_ref_##s

typedef struct {
  uint64_t s[25];
  unsigned int pos;
} keccak_state;

#define KeccakF1600_StatePermute FIPS202_NAMESPACE(KeccakF1600_StatePermute)
void KeccakF1600_StatePermute(uint64_t state[25]);

#endif
