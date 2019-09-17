#ifndef Z80TEST_H
#define Z80TEST_H

#include <cck.h>
#include <z80ex.h>

struct z8t_t {
    Z80EX_CONTEXT* cpu;
    struct cck_t* cck;
    uint8_t* memory;
};

void z8t_memdump(struct z8t_t* z8t, uint16_t len);

void z8t_regdump(struct z8t_t* z8t);

uint32_t z8t_run(struct z8t_t* z8t, uint32_t steps);

void z8t_init(struct z8t_t* z8t, int argc, char* argv[]);

void z8t_reg_a_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_b_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_c_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_bc_is(struct z8t_t* z8t, uint16_t desired, const char* test_name);

void z8t_reg_d_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_e_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_de_is(struct z8t_t* z8t, uint16_t desired, const char* test_name);

void z8t_reg_h_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_l_is(struct z8t_t* z8t, uint8_t desired, const char* test_name);

void z8t_reg_hl_is(struct z8t_t* z8t, uint16_t desired, const char* test_name);

void z8t_reg_ix_is(struct z8t_t* z8t, uint16_t desired, const char* test_name);

void z8t_reg_iy_is(struct z8t_t* z8t, uint16_t desired, const char* test_name);

void z8t_memory_is(struct z8t_t* z8t, uint8_t* desired, uint16_t length, const char* test_name);

#endif
