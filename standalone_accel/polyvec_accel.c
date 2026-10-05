#include "polyvec.h"
#include "poly.h"
#include "fips202.h"
#include "polyvec_accel.h"

void polyvec_accel(
    uint8_t op,
    int16_t *mem_pv_r, const int16_t *mem_pv_a, const int16_t *mem_pv_b,
    int16_t *mem_p_r, 
    uint64_t *mem_keccak_state
) {
    #pragma HLS INTERFACE s_axilite port=return bundle=control
    #pragma HLS INTERFACE s_axilite port=op bundle=control

    #pragma HLS INTERFACE m_axi port=mem_pv_r depth=KYBER_N*KYBER_K bundle=gmem0 offset=slave \
        num_write_outstanding=16 max_write_burst_length=256 \
        num_read_outstanding=16 max_read_burst_length=256 max_widen_bitwidth=64
    #pragma HLS INTERFACE s_axilite port=mem_pv_r bundle=control

    #pragma HLS INTERFACE m_axi port=mem_pv_a depth=KYBER_N*KYBER_K bundle=gmem1 offset=slave \
        num_read_outstanding=16 max_read_burst_length=256 max_widen_bitwidth=64
    #pragma HLS INTERFACE s_axilite port=mem_pv_a bundle=control

    #pragma HLS INTERFACE m_axi port=mem_pv_b depth=KYBER_N*KYBER_K bundle=gmem2 offset=slave \
        num_read_outstanding=16 max_read_burst_length=256 max_widen_bitwidth=64
    #pragma HLS INTERFACE s_axilite port=mem_pv_b bundle=control

    #pragma HLS INTERFACE m_axi port=mem_p_r depth=KYBER_N bundle=gmem0 offset=slave \
        num_write_outstanding=16 max_write_burst_length=256 \
        num_read_outstanding=16 max_read_burst_length=256 max_widen_bitwidth=64
    #pragma HLS INTERFACE s_axilite port=mem_p_r bundle=control

    #pragma HLS INTERFACE m_axi port=mem_keccak_state depth=25 bundle=gmem3 offset=slave \
        num_write_outstanding=16 max_write_burst_length=32 \
        num_read_outstanding=16 max_read_burst_length=32 max_widen_bitwidth=64
    #pragma HLS INTERFACE s_axilite port=mem_keccak_state bundle=control

    polyvec local_pv_r, local_pv_a, local_pv_b;
    poly    local_p_r;
    uint64_t local_keccak_state[25];

    #pragma HLS ARRAY_PARTITION variable=local_pv_r.vec dim=2 type=cyclic factor=16
    #pragma HLS ARRAY_PARTITION variable=local_pv_a.vec dim=2 type=cyclic factor=16
    #pragma HLS ARRAY_PARTITION variable=local_pv_b.vec dim=2 type=cyclic factor=16
    #pragma HLS ARRAY_PARTITION variable=local_p_r.coeffs dim=1 type=cyclic factor=16
    #pragma HLS ARRAY_PARTITION variable=local_keccak_state complete dim=1

    switch(op) {
        
        case OP_POLYVEC_NTT:
            load_pv_r_ntt: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_r[i];
            }
            
            polyvec_ntt(&local_pv_r); 
            
            write_pv_r_ntt: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                mem_pv_r[i] = local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N];
            }
            break;
            
        case OP_POLYVEC_INVNTT_TOMONT:
            load_pv_r_invntt: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_r[i];
            }
            
            polyvec_invntt_tomont(&local_pv_r);
            
            write_pv_r_invntt: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                mem_pv_r[i] = local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N];
            }
            break;
            
        case OP_POLYVEC_BASEMUL_ACC_MONTGOMERY:
            load_pv_a_basemul: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_a.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_a[i];
            }
            load_pv_b_basemul: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_b.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_b[i];
            }
            
            polyvec_basemul_acc_montgomery(&local_p_r, &local_pv_a, &local_pv_b);
            
            write_p_r_basemul: for(int i = 0; i < KYBER_N; i++) {
                #pragma HLS PIPELINE II=1
                mem_p_r[i] = local_p_r.coeffs[i];
            }
            break;
            
        case OP_POLYVEC_ADD:
            load_pv_a_add: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_a.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_a[i];
            }
            load_pv_b_add: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_b.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_b[i];
            }
            
            polyvec_add(&local_pv_r, &local_pv_a, &local_pv_b);
            
            write_pv_r_add: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                mem_pv_r[i] = local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N];
            }
            break;
            
        case OP_POLYVEC_REDUCE:
            load_pv_r_reduce: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N] = mem_pv_r[i];
            }
            
            polyvec_reduce(&local_pv_r);
            
            write_pv_r_reduce: for(int i = 0; i < KYBER_N * KYBER_K; i++) {
                #pragma HLS PIPELINE II=1
                mem_pv_r[i] = local_pv_r.vec[i / KYBER_N].coeffs[i % KYBER_N];
            }
            break;

        case OP_KECCAK_PERMUTE:
            load_keccak_state: for(int i = 0; i < 25; i++) {
                #pragma HLS PIPELINE II=1
                local_keccak_state[i] = mem_keccak_state[i];
            }

            KeccakF1600_StatePermute(local_keccak_state);

            write_keccak_state: for(int i = 0; i < 25; i++) {
                #pragma HLS PIPELINE II=1
                mem_keccak_state[i] = local_keccak_state[i];
            }
            break;

        default:
            break;
    }
}