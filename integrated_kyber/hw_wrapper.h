#ifndef HW_WRAPPER_H
#define HW_WRAPPER_H

#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"

#define OP_POLYVEC_NTT 0
#define OP_POLYVEC_INVNTT_TOMONT 1
#define OP_POLYVEC_BASEMUL_ACC_MONTGOMERY 2
#define OP_POLYVEC_ADD 3 
#define OP_POLYVEC_REDUCE 4
#define OP_KECCAK_F1600 5

int init_hw_accelerator(void);
void cleanup_hw_accelerator(void);

void hw_polyvec_ntt(polyvec *r);
void hw_polyvec_invntt_tomont(polyvec *r);
void hw_polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b);
void hw_polyvec_add(polyvec *r, const polyvec *a, const polyvec *b);
void hw_polyvec_reduce(polyvec *r);
void hw_keccakf1600_statepermute(uint64_t state[25]);

#endif