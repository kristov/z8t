#include <z8t.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

Z80EX_BYTE z8t_mem_read(Z80EX_CONTEXT* cpu, Z80EX_WORD addr, int m1_state, void* user_data) {
    struct z8t_t* z8t = (struct z8t_t*)user_data;
    return z8t->memory[(uint16_t)addr];
}

void z8t_mem_write(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void* user_data) {
    struct z8t_t* z8t = (struct z8t_t*)user_data;
    z8t->memory[(uint16_t)addr] = value;
}

Z80EX_BYTE z8t_port_read(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void* user_data) {
    //struct z8t_t* z8t = (struct z8t_t*)user_data;
    return 0;
}

void z8t_port_write(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE value, void* user_data) {
    //struct z8t_t* z8t = (struct z8t_t*)user_data;
}

Z80EX_BYTE z8t_int_read(Z80EX_CONTEXT *cpu, void* user_data) {
    //struct z8t_t* z8t = (struct z8t_t*)user_data;
    return 0;
}

Z80EX_BYTE z8t_mem_read_dasm(Z80EX_WORD addr, void* user_data) {
    struct z8t_t* z8t = (struct z8t_t*)user_data;
    return z8t->memory[(uint16_t)addr];
}

void z8t_regdump_28(const char* name, uint16_t val) {
    uint8_t lower = (uint8_t)(val & 0xff);
    uint8_t higher = (uint8_t)(val >> 8);
    fprintf(stdout, "%s %02x %02x\n", name, higher, lower);
}

void z8t_regdump(struct z8t_t* z8t) {
    uint16_t AF = (uint16_t)z80ex_get_reg(z8t->cpu, regAF);
    uint16_t BC = (uint16_t)z80ex_get_reg(z8t->cpu, regBC);
    uint16_t DE = (uint16_t)z80ex_get_reg(z8t->cpu, regDE);
    uint16_t HL = (uint16_t)z80ex_get_reg(z8t->cpu, regHL);
    uint16_t IX = (uint16_t)z80ex_get_reg(z8t->cpu, regIX);
    uint16_t IY = (uint16_t)z80ex_get_reg(z8t->cpu, regIY);
    uint16_t SP = (uint16_t)z80ex_get_reg(z8t->cpu, regSP);
    uint16_t PC = (uint16_t)z80ex_get_reg(z8t->cpu, regPC);
    z8t_regdump_28("AF", AF);
    z8t_regdump_28("BC", BC);
    z8t_regdump_28("DE", DE);
    z8t_regdump_28("HL", HL);
    z8t_regdump_28("IX", IX);
    z8t_regdump_28("IY", IY);
    z8t_regdump_28("SP", SP);
    z8t_regdump_28("PC", PC);
}

void z8t_memdump(struct z8t_t* z8t, uint16_t len) {
    uint16_t idx;
    uint16_t rows = len / 0x20;
    for (uint16_t y = 0; y < rows; y++) {
        fprintf(stdout, "%04x: ", idx);
        for (uint16_t x = 0; x < 0x20; x++) {
            fprintf(stdout, "%02x ", z8t->memory[idx]);
            idx++;
        }
        fprintf(stdout, "\n");
    }
}

void z8t_reg_a_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regAF);
    uint8_t reg8 = (uint8_t)(reg16 >> 8);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_b_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regBC);
    uint8_t reg8 = (uint8_t)(reg16 >> 8);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_c_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regBC);
    uint8_t reg8 = (uint8_t)(reg16 & 0xff);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_bc_is(struct z8t_t* z8t, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regBC);
    cck_is_equal_uint16(z8t->cck, reg16, desired, test_name);
}

void z8t_reg_d_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regDE);
    uint8_t reg8 = (uint8_t)(reg16 >> 8);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_e_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regDE);
    uint8_t reg8 = (uint8_t)(reg16 & 0xff);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_de_is(struct z8t_t* z8t, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regDE);
    cck_is_equal_uint16(z8t->cck, reg16, desired, test_name);
}

void z8t_reg_h_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regHL);
    uint8_t reg8 = (uint8_t)(reg16 >> 8);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_l_is(struct z8t_t* z8t, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regHL);
    uint8_t reg8 = (uint8_t)(reg16 & 0xff);
    cck_is_equal_uint8(z8t->cck, reg8, desired, test_name);
}

void z8t_reg_hl_is(struct z8t_t* z8t, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regHL);
    cck_is_equal_uint16(z8t->cck, reg16, desired, test_name);
}

void z8t_reg_ix_is(struct z8t_t* z8t, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regIX);
    cck_is_equal_uint16(z8t->cck, reg16, desired, test_name);
}

void z8t_reg_iy_is(struct z8t_t* z8t, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, regIY);
    cck_is_equal_uint16(z8t->cck, reg16, desired, test_name);
}

uint32_t z8t_run(struct z8t_t* z8t, uint32_t steps) {
    uint32_t steps_run = 0;
    for (uint32_t i = 0; i < steps; i++) {
        if (z80ex_doing_halt(z8t->cpu)) {
            steps_run--;
            break;
        }
        z80ex_step(z8t->cpu);
        steps_run++;
    }
    return steps_run;
}

void z8t_load_rom(struct z8t_t* z8t, char* file) {
    unsigned long len;

    FILE* rom_fh = fopen(file, "rb");
    if (rom_fh == NULL) {
        fprintf(stderr, "Could not open rom file: %s\n", file);
        exit(1);
    }

    fseek(rom_fh, 0, SEEK_END);
    len = ftell(rom_fh);
    rewind(rom_fh);

    if (len > 65536) {
        fprintf(stderr, "rom file %s longer than memory\n", file);
        len = 65536;
    }
    fread(z8t->memory, len, 1, rom_fh);

    fclose(rom_fh);
}

void z8t_args(struct z8t_t* z8t, int argc, char* argv[]) {
    int option_index = 0;
    char c;

    static struct option long_options[] = {
        {"rom", optional_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    while (1) {
        c = getopt_long(argc, argv, "r:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'r':
                z8t_load_rom(z8t, optarg);
                break;
            default:
                exit(1);
        }
    }
}

void z8t_init(struct z8t_t* z8t, int argc, char* argv[]) {
    memset(z8t, 0, sizeof(struct z8t_t));
    z8t->memory = malloc(sizeof(uint8_t) * 65536);
    if (z8t->memory == NULL) {
        return;
    }
    memset(z8t->memory, 0, sizeof(uint8_t) * 65536);
    z8t->cpu = z80ex_create(z8t_mem_read, z8t, z8t_mem_write, z8t, z8t_port_read, z8t, z8t_port_write, z8t, z8t_int_read, z8t);
    cck_init(z8t->cck);
    z8t_args(z8t, argc, argv);
}

