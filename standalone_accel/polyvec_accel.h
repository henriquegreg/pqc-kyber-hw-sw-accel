#include <stdint.h>
#include <string.h>

#define OP_POLYVEC_NTT 0
#define OP_POLYVEC_INVNTT_TOMONT 1
#define OP_POLYVEC_BASEMUL_ACC_MONTGOMERY 2
#define OP_POLYVEC_ADD 3 
#define OP_POLYVEC_REDUCE 4
#define OP_KECCAK_PERMUTE 5

void polyvec_accel(
    uint8_t op,
    int16_t *mem_pv_r, const int16_t *mem_pv_a, const int16_t *mem_pv_b,
    int16_t *mem_p_r,
    uint64_t *mem_keccak_state
);
