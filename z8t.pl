#!/usr/bin/env perl

use strict;
use warnings;
use FindBin;
use Getopt::Long;

my %REG2REGPAIR = (
    A => ['AF', 'U'],
    F => ['AF', 'L'],
    B => ['BC', 'U'],
    C => ['BC', 'L'],
    D => ['DE', 'U'],
    E => ['DE', 'L'],
    H => ['HL', 'U'],
    L => ['HL', 'L'],
);

my %COMMANDS = (
    DIAG => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^"([^"]+)"$/) {
                diag($self, "# $1");
                return;
            }
            die "Invalid DIAG at line " . $self->{line};
        },
        post => sub {},
    },
    CODE => {
        args => sub {
            my ($self) = @_;
            $self->{record} = 1;
        },
        post => sub {
            my ($self) = @_;
            for my $line (@{$self->{line_buf}}) {
                push @{$self->{src}}, $line;
            }
            push @{$self->{src}}, "    halt";
            $self->{line_buf} = [];
            $self->{record} = 0;
        },
    },
    INCLUDE => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^"([^"]+)"$/) {
                push @{$self->{src}}, slurp_lines($1);
                return;
            }
            die "Invalid INCLUDE at line " . $self->{line};
        },
        post => sub {},
    },
    RUN => {
        args => sub {},
        post => sub {
            my ($self) = @_;
            run_test($self);
        },
    },
    REG => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^([AFBCDEHLSP]+)\s+([a-zA-Z0-9]+)\s+"([^"]+)"/) {
                my ($register, $expected, $name) = ($1, $2, $3);
                my $regpair = $register;
                my $UL = 'UL';
                if ($REG2REGPAIR{$register}) {
                    $regpair = $REG2REGPAIR{$register}->[0];
                    $UL = $REG2REGPAIR{$register}->[1];
                }
                $expected = convert($expected);
                my $got = $self->{registers}->{$regpair}->{$UL};
                if ($UL eq 'UL') {
                    test_16b($self, $got, $expected, $name);
                }
                else {
                    test_8b($self, $got, $expected, $name);
                }
                return;
            }
            die "Invalid REG at line " . $self->{line};
        },
        post => sub {},
    },
    MEM => {
        args => sub {
            my ($self, $rest) = @_;
            $self->{record} = 1;
            if ($rest =~ /^([a-zA-Z0-9]+)\s+"([^"]+)"/) {
                my ($start, $name) = ($1, $2);
                $self->{stash} = {
                    start => $start,
                    name => $name,
                };
                return;
            }
            die "Invalid MEM at line " . $self->{line};
        },
        post => sub {
            my ($self) = @_;
            my $mem = join(" ", @{$self->{line_buf}});
            my @data = map {$_ =~ /xx/i ? undef : convert($_)} split(/\s+/, $mem);
            my $nr_bytes = scalar(@data);
            my $idx = hex($self->{stash}->{start});
            my $ok = 0;
            my $didx = 0;
            my @mem;
            BYTE: for (my $didx = 0; $didx < scalar(@data); $didx++) {
                my $dat = $data[$didx];
                push @mem, $self->{memory}->[$idx];
                if (!defined $dat) {
                    $ok++;
                    $idx++;
                    next BYTE;
                }
                if ($dat == $self->{memory}->[$idx]) {
                    $ok++;
                }
                $idx++;
            }
            if ($nr_bytes == $ok) {
                diag($self, "ok: " . $self->{stash}->{name});
                $self->{test}->{pass}++;
            }
            else {
                diag($self, "FAIL: " . $self->{stash}->{name});
                diag($self, "  got: " . join(" ", map {sprintf("%02x", $_)} @mem));
                diag($self, "  exp: " . join(" ", map {defined $_ ? sprintf("%02x", $_) : ".."} @data));
                $self->{test}->{fail}++;
            }
            $self->{line_buf} = [];
            $self->{record} = 0;
        },
    },
    CYCLES => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^([a-zA-Z0-9]+)/) {
                my $expected = convert($1);
                my $cycles = $self->{cycles};
                if ($cycles > $expected) {
                    diag($self, "WARN: program took $cycles cycles, but $expected expected");
                }
            }
        },
        post => sub {},
    },
    RESET => {
        args => sub {
            my ($self) = @_;
            $self->{line_buf} = [];
            $self->{src} = [];
            $self->{cycles} = 0;
        },
        post => sub {},
    },
    STACK => {
        args => sub {},
        post => sub {},
    },
    EXIT => {
        args => sub {
            my ($self) = @_;
            $self->{exit} = 1;
            diag($self, "# EXITING");
        },
        post => sub {},
    },
);

sub convert {
    my ($num) = @_;
    return (substr($num, 0, 2) eq '0x') ? hex($num) : $num;
}

sub test_8b {
    my ($self, $got, $expected, $name) = @_;
    if ($got == $expected) {
        $self->{test}->{pass}++;
        diag($self, "ok: $name");
    }
    else {
        $self->{test}->{fail}++;
        diag($self, sprintf("FAIL: got: 0x%02x exp: 0x%02x: %s", $got, $expected, $name));
    }
}

sub test_16b {
    my ($self, $got, $expected, $name) = @_;
    if ($got == $expected) {
        $self->{test}->{pass}++;
        diag($self, "ok: $name");
    }
    else {
        $self->{test}->{fail}++;
        diag($self, sprintf("FAIL: got: 0x%04x exp: 0x%04x: %s", $got, $expected, $name));
    }
}

sub diag {
    my ($self, $msg) = @_;
    if (!$self->{quiet}) {
        print "$msg\n";
    }
}

sub assemble {
    my ($self, $asm_file, $bin_file, $lst_file) = @_;
    my $cmd = $self->{args}->{asm};
    $cmd =~ s/<ASM>/$asm_file/;
    $cmd =~ s/<BIN>/$bin_file/;
    `z80asm-gnu -L -o $bin_file $asm_file > $lst_file 2>&1`;
}

sub runbin {
    my ($self, $bin_file, $core_file) = @_;
    my $cmd = $self->{args}->{z8t};
    `$cmd -r $bin_file > $core_file`;
}

sub run_test {
    my ($self) = @_;
    write_lines("test.asm", $self->{src});
    assemble($self, 'test.asm', 'test.bin', 'test.lst');
    runbin($self, 'test.bin', 'test.core');
    open(my $fh, '<', 'test.core') || die "unable to open 'test.core': $!";
    LINE: while (my $line = <$fh>) {
        if ($line =~ /^([a-z0-9]+): ([a-z0-9\s]+)/) {
            my ($addr, $datastr) = ($1, $2);
            my $idx = hex($addr);
            my @data = split(/\s+/, $datastr);
            for my $dat (@data) {
                $self->{memory}->[$idx] = hex($dat);
                $idx++;
            }
            next LINE;
        }
        if ($line =~ /^([AFBCDEHLSP]{2})\s+([a-z0-9]{2})\s+([a-z0-9]{2})/) {
            my ($regpair, $upper, $lower) = ($1, $2, $3);
            $self->{registers}->{$regpair}->{L} = hex($lower);
            $self->{registers}->{$regpair}->{U} = hex($upper);
            $self->{registers}->{$regpair}->{UL} = hex($upper . $lower);
            next LINE;
        }
        if ($line =~ /^CYCLES\s+([a-z0-9]{8})/) {
            $self->{cycles} = hex($1);
        }
    }
    close $fh;
    if (!$self->{args}->{keep}) {
        `rm test.asm`;
        `rm test.core`;
        `rm test.bin`;
        `rm test.lst`;
    }
    if ($self->{registers}->{SP}->{UL} != 0xffff) {
        diag($self, sprintf("WARN: the stack looks pooped: 0x%04x", $self->{registers}->{SP}->{UL}));
    }
}

sub run_test_file {
    my ($args, $test_file, $quiet) = @_;
    my $self = {
        args => $args,
        exit => 0,
        quiet => $quiet,
        line => 0,
        src => [],
        current_command => undef,
        line_buf => [],
        test => {
            pass => 0,
            fail => 0,
        },
    };
    open(my $fh, '<', $test_file) || die "unable to open test file '$test_file': $!";
    my $no = 0;
    LINE: while (my $line = <$fh>) {
        chomp $line;
        $self->{line}++;
        if ($self->{exit}) {
            last;
        }
        if ($line =~ /^>([A-Z]+)\s+(.+)/) {
            my ($command, $rest) = ($1, $2);
            if ($self->{current_command}) {
                command_post($self, $self->{current_command});
            }
            $self->{current_command} = $command;
            command_args($self, $command, $rest);
            next LINE;
        }
        if ($line =~ /^>([A-Z]+)$/) {
            my ($command) = ($1);
            if ($self->{current_command}) {
                command_post($self, $self->{current_command});
            }
            $self->{current_command} = $command;
            command_args($self, $command);
            next LINE;
        }
        if ($line =~ /^#/) {
            next LINE;
        }
        if ($self->{record}) {
            push @{$self->{line_buf}}, $line;
        }
    }
    if ($self->{current_command}) {
        command_post($self, $self->{current_command});
    }

    my $pass = $self->{test}->{pass};
    my $fail = $self->{test}->{fail};
    my $total = $pass + $fail;
    printf("pass/total: %d/%d [%s]\n", $pass, $total, $test_file);

    close $fh;
}

sub slurp_lines {
    my ($file_name) = @_;
    open(my $fh, '<', $file_name) || die "unable to open file '$file_name': $!";
    my @lines;
    while (my $line = <$fh>) {
        chomp $line;
        push @lines, $line;
    }
    close $fh;
    return @lines;
}

sub write_lines {
    my ($file_name, $lines) = @_;
    open(my $fh, '>', $file_name) || die "unable to open file '$file_name': $!";
    for my $line (@{$lines}) {
        print $fh "$line\n";
    }
    close $fh;
}

sub command_args {
    my ($self, $command, $rest) = @_;
    if (!$COMMANDS{$command}) {
        die "unknown command: $command";
    }
    $COMMANDS{$command}->{args}->($self, $rest);
}

sub command_post {
    my ($self, $command, $rest) = @_;
    if (!$COMMANDS{$command}) {
        die "unknown command: $command";
    }
    $COMMANDS{$command}->{post}->($self);
}

sub print_help {
    print "Usage: $0 [--keep][--asm=<assembler>][--help] <test1> <test2> ..\n\n";
    print "  Run one or more .z8t test scripts and report on results.\n\n";
    print "  --keep (-k):\n";
    print "      Keep the .asm, .bin and .core files after compliation.\n\n";
    print "  --asm=<assembler>:\n";
    print "      Specifiy an assembler to use for compliation.\n\n";
}

sub get_args {
    my $args = {
        keep => 0,
        test_files => [],
    };
    GetOptions(
        $args,
        'keep',     # keep .asm, .bin and .core files after compliation
        'asm=s',    # the assembler to use
        'z8t=s',    # location of the z8t binary
        'help',     # help
    );
    if (!$args->{z8t}) {
        $args->{z8t} = sprintf("%s/z8t", $FindBin::Bin);
    }
    if (!$args->{asm}) {
        $args->{asm} = 'z80asm -o ${BIN} ${ASM}';
    }
    for my $arg (@ARGV) {
        push @{$args->{test_files}}, $arg;
    }
    return $args;
}

sub main {
    my ($args) = @_;
    if ($args->{help}) {
        print_help();
        exit 0;
    }
    my $quiet = (scalar(@{$args->{test_files}}) > 1) ? 1 : 0;
    for my $test_file (@{$args->{test_files}}) {
        run_test_file($args, $test_file, $quiet);
    }
}

exit main(get_args());
