# Z8T - Z80 testing framework

Run chunks of Z80 machine code in an emulator and test if output values are
expected. First separate the thing you want to test into it's own asm file:

    ; contents of my_special_func.asm
    my_special_func:
        ld hl, some_var
        ld (hl), 0x23
        ret

Then create a z8t test script:

    >DIAG "Preparing to test with a thing"
    >CODE
    some_var: equ 0x26
        ld a, 0xde
        call my_special_func
    >INCLUDE "my_special_func.asm"
    >RUN
    # Comments about the test are allowed like this
    >REG A  0x04   "Register A is 4"
    >REG HL 0x0302 "Register HL is correct"
    >MEM 0xaf "Memory chunk correct"
    0x04 0x06 0x07 0x03 0x02
    0x02 0x07
    >RESET
    >DIAG "Preparing to test with different thing"
    >CODE
    some_var: equ 0x26
        ld a, 0xb4
        call my_special_func
    >INCLUDE "my_special_func.asm"
    >RUN
    >REG DE 0x22 "Register DE is correct"
    >STACK 0x01 0x01 0x00 0x22 "Stack top looks good"
    >MEM 0xaf "Memory chunk correct"
    0x04 0x06 0x07 0x03 0x02
    0x02 0x07

## Test script commands

Z8t commands begin with `>`. Each explained:

### >CODE

This is inline code that is injected into the source. It is used to initialise
the test, set up global variables etc. Note: common parts can be added to an
`INCLUDE` file and be imported like the source being tested. Note: all `CODE`
sections have a `halt` instruction inserted after them. This is to ensure that
when a test is run the program does not fall through to subsequent code and
corrupt the machine state before tests can be made.

### >INCLUDE "`<file>`"

This code is included in the source at the moment it's encountered in the test
script.

### >DIAG "..."

Print this dialog at this point in the test. These will only be seen if there
is only one z8t test script provided to `z8t.pl`, as otherwise only a final
single line report is printed.

### >RUN

This generates the assembly from the `CODE` and `INCLUDE` source, compiles to
binary and executes the binary in `z8t`. That runs the code until a halt and
dumps the machine state.

### >REG `<X>` `<expected>` "..."

Test that register `<x>` is the value provided.

### >MEM `<start>` "..."

Test that the memory starting at `<start>` is equal to the chunk of numbers
following until the next test instruction (or the end of the script). It
doesn't matter how many bytes are per line so you can arrange them for
readability.

### >RESET

This wipes the source code so the next test is done from a clean slate. i can
not think of a case where you would not want to do this immediately after your
`REG`, `MEM` or `STACK` tests are completed and before the next test.

### >STACK `<stack contents>`

This tests the contents of the stack starting from the "top" of the stack
(lower memory location up). *NOT IMPLEMENTED*

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
