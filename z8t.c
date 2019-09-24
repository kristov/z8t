#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <regex.h>
#include <stdint.h>
#include <z80ex.h>

struct z8t_t {
    Z80EX_CONTEXT* cpu;
    uint32_t passes;
    uint32_t failures;
    uint32_t clock_cycles;
    uint8_t* memory;
    char* test_file;
};

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

static void is_equal_uint16(struct z8t_t* z8t, uint16_t provided, uint16_t desired, const char* test_name) {
    if (provided != desired) {
        z8t->failures++;
        fprintf(stderr, "FAIL: %s\n", test_name);
        fprintf(stderr, "    provided: %d, desired: %d\n", provided, desired);
    }
    else {
        z8t->passes++;
        fprintf(stderr, "pass: %s\n", test_name);
    }
}

static void is_equal_uint8(struct z8t_t* z8t, uint8_t provided, uint8_t desired, const char* test_name) {
    if (provided != desired) {
        z8t->failures++;
        fprintf(stderr, "FAIL: %s\n", test_name);
        fprintf(stderr, "    provided: %d, desired: %d\n", provided, desired);
    }
    else {
        z8t->passes++;
        fprintf(stderr, "pass: %s\n", test_name);
    }
}

void z8t_report(struct z8t_t* z8t) {
    fprintf(stderr, "Ran %d tests:\n", z8t->failures + z8t->passes);
    fprintf(stderr, "  %d Failures\n", z8t->failures);
    fprintf(stderr, "  %d Passes\n", z8t->passes);
    if (0 == z8t->failures) {
        fprintf(stderr, "-- ALL TESTS PASS --\n");
    }
    else {
        fprintf(stderr, "-- SOME TESTS FAILED --\n");
    }
    fprintf(stderr, "CYCLES: %d\n", z8t->clock_cycles);
}

uint32_t z8t_run(struct z8t_t* z8t, uint32_t steps) {
    uint32_t clock_cycles = 0;
    if (steps == 0) {
        while (1) {
            if (z80ex_doing_halt(z8t->cpu)) {
                break;
            }
            clock_cycles += z80ex_step(z8t->cpu);
        }
    }
    else {
        for (uint32_t i = 0; i < steps; i++) {
            if (z80ex_doing_halt(z8t->cpu)) {
                break;
            }
            clock_cycles += z80ex_step(z8t->cpu);
        }
    }
    z8t->clock_cycles = clock_cycles;
    return clock_cycles;
}

static void test_reg8L(struct z8t_t* z8t, uint8_t reg, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, reg);
    uint8_t reg8 = (uint8_t)(reg16 & 0xff);
    is_equal_uint8(z8t, reg8, desired, test_name);
}

static void test_reg8H(struct z8t_t* z8t, uint8_t reg, uint8_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, reg);
    uint8_t reg8 = (uint8_t)(reg16 >> 8);
    is_equal_uint8(z8t, reg8, desired, test_name);
}

static void test_reg16(struct z8t_t* z8t, uint8_t reg, uint16_t desired, const char* test_name) {
    uint16_t reg16 = (uint16_t)z80ex_get_reg(z8t->cpu, reg);
    is_equal_uint16(z8t, reg16, desired, test_name);
}

void z8t_run_test(struct z8t_t* z8t) {
    if (!z8t->test_file) {
        fprintf(stderr, "No test file name set\n");
    }
    FILE* fh = fopen(z8t->test_file, "r");
    if (fh == NULL) {
        fprintf(stderr, "Could not open test file: %s\n", z8t->test_file);
        return;
    }

    regex_t r;
    regmatch_t matches[3];
    if (regcomp(&r, "^([A-Z]+):\\s+([0-9xa-f:,]+)", REG_EXTENDED) != 0) {
        fprintf(stderr, "Regex compilation failed!\n");
        return;
    }

    size_t len = 0;
    ssize_t read;
    char* line = NULL;
    char cmd[256];
    char val[256];
    while ((read = getline(&line, &len, fh)) != -1) {
        if (regexec(&r, line, 3, &matches[0], 0) != 0) {
            continue;
        }

        if (matches[1].rm_so != 0 || matches[1].rm_eo <= 0) {
            continue;
        }
        uint8_t cmd_len = matches[1].rm_eo - matches[1].rm_so;
        memset(cmd, 0, 256);
        memcpy(cmd, line + matches[1].rm_so, cmd_len);

        if (matches[2].rm_so <= 0 || matches[2].rm_eo <= 0) {
            continue;
        }
        uint8_t val_len = matches[2].rm_eo - matches[2].rm_so;
        memset(val, 0, 256);
        memcpy(val, line + matches[2].rm_so, val_len);

        int intval = (int)strtol(val, NULL, 16);

        switch (cmd[0]) {
            case 'A':
                switch (cmd_len) {
                    case 1:
                        test_reg8H(z8t, regAF, (uint8_t)intval, "Register A value correct");
                        break;
                    case 2:
                        test_reg16(z8t, regAF, (uint16_t)intval, "Register AF value correct");
                        break;
                    case 3:
                        test_reg16(z8t, regAF_, (uint16_t)intval, "Register AF' value correct");
                        break;
                }
                break;
            case 'B':
                switch (cmd_len) {
                    case 1:
                        test_reg8H(z8t, regBC, (uint8_t)intval, "Register A value correct");
                        break;
                    case 2:
                        test_reg16(z8t, regBC, (uint16_t)intval, "Register BC value correct");
                        break;
                    case 3:
                        test_reg16(z8t, regBC_, (uint16_t)intval, "Register BC' value correct");
                        break;
                }
                break;
            case 'C':
                switch (cmd_len) {
                    case 1:
                        test_reg8L(z8t, regBC, (uint8_t)intval, "Register C value correct");
                        break;
                    case 2:
                        test_reg8L(z8t, regBC_, (uint8_t)intval, "Register C' value correct");
                        break;
                }
                break;
            case 'D':
                switch (cmd_len) {
                    case 1:
                        test_reg8H(z8t, regDE, (uint8_t)intval, "Register D value correct");
                        break;
                    case 2:
                        test_reg16(z8t, regDE, (uint16_t)intval, "Register DE value correct");
                        break;
                    case 3:
                        test_reg16(z8t, regDE_, (uint16_t)intval, "Register DE' value correct");
                        break;
                }
                break;
            case 'E':
                switch (cmd_len) {
                    case 1:
                        test_reg8L(z8t, regDE, (uint8_t)intval, "Register E value correct");
                        break;
                    case 2:
                        test_reg8L(z8t, regDE_, (uint8_t)intval, "Register E' value correct");
                        break;
                }
                break;
            case 'H':
                switch (cmd_len) {
                    case 1:
                        test_reg8H(z8t, regHL, (uint8_t)intval, "Register H value correct");
                        break;
                    case 2:
                        test_reg16(z8t, regHL, (uint16_t)intval, "Register HL value correct");
                        break;
                    case 3:
                        test_reg16(z8t, regHL_, (uint16_t)intval, "Register HL' value correct");
                        break;
                }
                break;
            case 'L':
                switch (cmd_len) {
                    case 1:
                        test_reg8L(z8t, regHL, (uint8_t)intval, "Register L value correct");
                        break;
                    case 2:
                        test_reg8L(z8t, regHL_, (uint8_t)intval, "Register L' value correct");
                        break;
                }
                break;
            case 'S':
                switch (cmd_len) {
                    case 1:
                        fprintf(stderr, "No just 'S'\n");
                        break;
                    case 2:
                        break;
                    default:
                        fprintf(stderr, "'S'whaat?\n");
                }
                break;
            case 'I':
                switch (cmd_len) {
                    case 1:
                        test_reg8L(z8t, regI, (uint8_t)intval, "Register I value correct");
                        break;
                    case 2:
                        switch (cmd[1]) {
                            case 'X':
                                test_reg16(z8t, regIX, (uint16_t)intval, "Register IX value correct");
                                break;
                            case 'Y':
                                test_reg16(z8t, regIY, (uint16_t)intval, "Register IY value correct");
                                break;
                            default:
                                fprintf(stderr, "'I'whaat?\n");
                        }
                        break;
                    default:
                        fprintf(stderr, "Too many things for 'I'\n");
                        break;
                }
                break;
            case 'R':
                break;
            case 'M':
                break;
            default:
                fprintf(stderr, "unknown command: %s\n", cmd);
                break;
        }
    }

    fclose(fh);
    if (line) {
        free(line);
    }
}

void z8t_load_rom(struct z8t_t* z8t, char* file) {
    unsigned long len;

    FILE* fh = fopen(file, "rb");
    if (fh == NULL) {
        fprintf(stderr, "Could not open rom file: %s\n", file);
        exit(1);
    }

    fseek(fh, 0, SEEK_END);
    len = ftell(fh);
    rewind(fh);

    if (len > 65536) {
        fprintf(stderr, "rom file %s longer than memory\n", file);
        len = 65536;
    }
    fread(z8t->memory, len, 1, fh);

    fclose(fh);
}

void z8t_args(struct z8t_t* z8t, int argc, char* argv[]) {
    int option_index = 0;
    static struct option long_options[] = {
        {"rom", optional_argument, 0, 'r'},
        {"tspec", optional_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    char c;
    uint16_t file_name_len = 0;
    while (1) {
        c = getopt_long(argc, argv, "r:t:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'r':
                z8t_load_rom(z8t, optarg);
                break;
            case 't':
                file_name_len = strlen(optarg);
                z8t->test_file = malloc(sizeof(char) * (file_name_len + 1));
                memcpy(z8t->test_file, optarg, file_name_len);
                z8t->test_file[file_name_len] = 0;
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
    z8t_args(z8t, argc, argv);
}

int main(int argc, char* argv[]) {
    struct z8t_t z8t;
    z8t_init(&z8t, argc, argv);
    z8t_run(&z8t, 0);
    z8t_run_test(&z8t);
    z8t_report(&z8t);
    return 0;
}
