# Z8T - Z80 testing framework

Run chunks of Z80 machine code in an emulator and test if output values are
expected. First create some test case in assembler:

    org 0x00
    test_setup:
        ld a, 0x02
        lb b, 0x03
        call add_a_and_b
        halt
    add_a_and_b:
        add a, b
        ret

Note: probably you want `add_a_and_b` to be defined in it's own source file and
`include` it into the test case. The halt instruction is important or your
program will never end. Assemble your source code into a binary using some
assembler. Then create a test runner:

    #include <z8t.h>
    #include <stdio.h>

    int main(int argc, char* argv[]) {
        // Create the context for the test
        //
        struct z8t_t z8t;

        // Initialize the context: this parses commandline arguments
        // like --rom=<the test rom>
        //
        z8t_init(&z8t, argc, argv);

        // Runs the code. Here 0 means run until a halt instruction. Any other
        // positive number means number of instructions to execute. The
        // function also returns the total number of clock cycles.
        //
        uint32_t clock_cycles = z8t_run(&z8t, 0);

        // Now execute the tests of what you expect the state to look
        // like after your code has run.
        //
        z8t_reg_a_is(&z8t, 0x05, "add works as expected");
        z8t_reg_b_is(&z8t, 0x03, "register b was left alone");

        // Produce a text report of what happened.
        //
        z8t_report(&z8t);

        // Perhaps you want to know if your optimizing your code or not?
        //
        printf("clock_cycles: %d\n", clock_cycles);

        return 0;
    }

Compile the test and include the archive `z8t.a`:

    gcc -Wall -Werror -o my_test.t my_test.c z8t.a


