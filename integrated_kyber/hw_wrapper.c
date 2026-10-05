#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <libxlnk_cma.h> 
#include <string.h>
#include "hw_wrapper.h"

#define IP_BASE_ADDR 0x40000000 
#define IP_MAP_SIZE  0x1000

#define REG_CTRL          0x00
#define REG_OP            0x10
#define REG_MEM_PV_R_LOW  0x18
#define REG_MEM_PV_R_HIGH 0x1c
#define REG_MEM_PV_A_LOW  0x24
#define REG_MEM_PV_A_HIGH 0x28
#define REG_MEM_PV_B_LOW  0x30
#define REG_MEM_PV_B_HIGH 0x34
#define REG_MEM_P_R_LOW   0x3c
#define REG_MEM_P_R_HIGH  0x40
#define REG_MEM_KECCAK_STATE_LOW  0x48
#define REG_MEM_KECCAK_STATE_HIGH 0x4c

static volatile uint32_t *ip_regs = NULL;
static int mem_fd = -1;

// Buffers CMA (Contiguous Memory Allocation)
static int16_t *cma_buf_pv_r = NULL;
static int16_t *cma_buf_pv_a = NULL;
static int16_t *cma_buf_pv_b = NULL;
static int16_t *cma_buf_p_r  = NULL; 
static uint64_t *cma_buf_keccak_state = NULL;

static void set_hw_pointer(uint32_t offset_low, uint32_t offset_high, void *cma_ptr) {
    uint64_t phy_addr = (uint64_t)cma_get_phy_addr(cma_ptr);
    ip_regs[offset_low / 4]  = (uint32_t)(phy_addr & 0xFFFFFFFF);
    ip_regs[offset_high / 4] = (uint32_t)((phy_addr >> 32) & 0xFFFFFFFF);
}

static void run_hw_ip(uint8_t op) {
    ip_regs[REG_OP / 4] = op;
    
    ip_regs[REG_CTRL / 4] = 0x01;
    
    while ((ip_regs[REG_CTRL / 4] & 0x02) == 0) {
    }
}


int init_hw_accelerator(void) {
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Error to open /dev/mem (are you root)");
        return -1;
    }
    
    ip_regs = (uint32_t *)mmap(NULL, IP_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, IP_BASE_ADDR);
    if (ip_regs == MAP_FAILED) {
        perror("Error on mmap");
        close(mem_fd);
        return -1;
    }

    size_t size_pv = KYBER_N * KYBER_K * sizeof(int16_t);
    size_t size_p  = KYBER_N * sizeof(int16_t);
    size_t size_keccak = 25 * sizeof(uint64_t);

    cma_buf_keccak_state = (uint64_t *)cma_alloc(size_keccak, 0);
    cma_buf_pv_r = (int16_t *)cma_alloc(size_pv, 0);
    cma_buf_pv_a = (int16_t *)cma_alloc(size_pv, 0);
    cma_buf_pv_b = (int16_t *)cma_alloc(size_pv, 0);
    cma_buf_p_r  = (int16_t *)cma_alloc(size_p, 0);
    
    if (!cma_buf_pv_r || !cma_buf_pv_a || !cma_buf_pv_b || !cma_buf_p_r || !cma_buf_keccak_state) {
        fprintf(stderr, "Error to allocate CMA memory.\n");
        return -1;
    }
    
    set_hw_pointer(REG_MEM_PV_R_LOW, REG_MEM_PV_R_HIGH, cma_buf_pv_r);
    set_hw_pointer(REG_MEM_PV_A_LOW, REG_MEM_PV_A_HIGH, cma_buf_pv_a);
    set_hw_pointer(REG_MEM_PV_B_LOW, REG_MEM_PV_B_HIGH, cma_buf_pv_b);
    set_hw_pointer(REG_MEM_P_R_LOW,  REG_MEM_P_R_HIGH,  cma_buf_p_r);
    set_hw_pointer(REG_MEM_KECCAK_STATE_LOW, REG_MEM_KECCAK_STATE_HIGH, cma_buf_keccak_state);

    return 0;
}

void cleanup_hw_accelerator(void) {
    if (cma_buf_pv_r) cma_free(cma_buf_pv_r);
    if (cma_buf_pv_a) cma_free(cma_buf_pv_a);
    if (cma_buf_pv_b) cma_free(cma_buf_pv_b);
    if (cma_buf_p_r)  cma_free(cma_buf_p_r);
    if (cma_buf_keccak_state) cma_free(cma_buf_keccak_state);
    
    if (ip_regs && ip_regs != MAP_FAILED) {
        munmap((void*)ip_regs, IP_MAP_SIZE);
    }
    if (mem_fd >= 0) {
        close(mem_fd);
    }
}

void hw_polyvec_ntt(polyvec *r) {
    // Copy-in 
    memcpy(cma_buf_pv_r, r->vec, sizeof(polyvec));

    // Run Hardware
    run_hw_ip(OP_POLYVEC_NTT);

    // Copy-out 
    memcpy(r->vec, cma_buf_pv_r, sizeof(polyvec));
}

void hw_polyvec_invntt_tomont(polyvec *r) {
    // Copy-in
    memcpy(cma_buf_pv_r, r->vec, sizeof(polyvec));

    // Run Hardware
    run_hw_ip(OP_POLYVEC_INVNTT_TOMONT);

    // Copy-out
    memcpy(r->vec, cma_buf_pv_r, sizeof(polyvec));
}

void hw_polyvec_reduce(polyvec *r) {
    // Copy-in
    memcpy(cma_buf_pv_r, r->vec, sizeof(polyvec));

    // Run Hardware
    run_hw_ip(OP_POLYVEC_REDUCE);

    // Copy-out
    memcpy(r->vec, cma_buf_pv_r, sizeof(polyvec));
}

void hw_polyvec_basemul_acc_montgomery(poly *r, const polyvec *a, const polyvec *b) {
    // Copy-in A e B
    memcpy(cma_buf_pv_a, a->vec, sizeof(polyvec));
    memcpy(cma_buf_pv_b, b->vec, sizeof(polyvec));

    // Run Hardware
    run_hw_ip(OP_POLYVEC_BASEMUL_ACC_MONTGOMERY);

    // Copy-out 
    memcpy(r->coeffs, cma_buf_p_r, sizeof(poly));
}

void hw_polyvec_add(polyvec *r, const polyvec *a, const polyvec *b) {
    // Copy-in A e B
    memcpy(cma_buf_pv_a, a->vec, sizeof(polyvec));
    memcpy(cma_buf_pv_b, b->vec, sizeof(polyvec));

    // Run Hardware
    run_hw_ip(OP_POLYVEC_ADD);

    // Copy-out R
    memcpy(r->vec, cma_buf_pv_r, sizeof(polyvec));
}

void hw_keccakf1600_statepermute(uint64_t state[25]) {
    // Copy-in 
    memcpy(cma_buf_keccak_state, state, 25 * sizeof(uint64_t));

    // Run Hardware
    run_hw_ip(OP_KECCAK_F1600);

    // Copy-out 
    memcpy(state, cma_buf_keccak_state, 25 * sizeof(uint64_t));
}