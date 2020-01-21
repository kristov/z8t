# Z8T - Z80 testing framework

Run chunks of Z80 machine code in an emulator and test if output values are
expected. First separate the thing you want to test into it's own asm file:

    ; contents of my_special_func.asm
    my_special_func:
        ld hl, some_var
        ld (hl), 0x23
        ret

Then create a z8t test script:

    >INIT
    # initalization code
    some_var: equ 0x26
    >DIAG "Preparing to test with a thing"
    >TEST
        ld a, 0xde
        call my_special_func
    >INCLUDE "../my_special_func.asm"
    # Comments about the test are allowed like this
    >REG A  0x04   "Register A is 4"
    >REG HL 0x0302 "Register HL is correct"
    >MEM 0xaf "Memory chunk correct"
    0x04 0x06 0x07 0x03 0x02
    0x02 0x07
    >RESET
    >DIAG "Preparing to test with different thing"
    >TEST
        ld a, 0xb4
        call my_special_func
    >INCLUDE "../my_special_func.asm"
    >REG DE 0x22 "Register DE is correct"
    >STACK 0x01 0x01 0x00 0x22 "Stack top looks good"
    >MEM 0xaf "Memory chunk correct"
    0x04 0x06 0x07 0x03 0x02
    0x02 0x07

## Test script commands

Z8t commands begin with `>`. Each explained:

### >INIT

The init code is included at the begininng of the asm source. If you need to
set some global variables or otherwise prepare the environment, set a chunk of
memory etc - something you want to do at the start of every test.

### >INCLUDE "`<file>`"

This code is included in the test asm at the moment it's encountered in the
test script. A `RESET` will compile a new binary so if you want something
included for each test is must be done after each `RESET`. The `INCLUDE` can
either be before or after the `TEST` block depending on what you want, although
there is an implicit `halt` instruction injected after each `TEST` block,
before any subsequent `INCLUDE`s.

### >DIAG "..."

Print this dialog at this point in the test. These will only be seen if there
is only one z8t test script provided to `z8t.pl`, as otherwise only a final
single line report is printed.

### >TEST

The code that will be run to do the test. No infinite loops are allowed as the
test will spin here and never proceed. After the test code a `halt` instruction
is injected to signal to the emulator that the code has completed and the state
of the machine can be inspected.

### >REG `<X>` `<expected>` "..."

Test that register `<x>` is the value provided.

### >MEM `<start>` "..."

Test that the memory starting at `<start>` is equal to the chunk of numbers
following until the next test instruction (or the end of the script). It
doesn't matter how many bytes are per line so you can arrange them for
readability.

### >RESET

If you want the machine to be reset then here a new binary will be compiled.
Otherwise the machine will be started again with the program counter set to the
location after the last injected `halt`. If reset the new binary will have the
`INIT` code prepended, then any `INCLUDE` lines after this `RESET`. It will
then be compiled into a new binary and run on the reset machine.

### >STACK `<stack contents>`

This tests the contents of the stack starting from the "top" of the stack
(lower memory location up).

### Values

Values can be provided as either 8 bit hex (`0xff`), 16 bit hex (`0x00ff`) or 8
bit decimal (255). If you don't care about a value use the special `0xXX`,
`0xXXXX` or `XXX` placeholders respectively. This can be used to skip over
values that you can't know (eg: addresses of functions only known after
assembly).

## Running

    `z8t.pl <test>.z8t`

This will shell out to the configured assembler to compile each test, and run
`z8t` the binary to execute the test and dump the registers and memory to a
temporary file for reading and testing. The return code will be zero if all the
tests passed. It will produce a summary report for each test script if more
than one test script is provided, otherwise it will be more verbose showing the
results of each subtest in the script.
