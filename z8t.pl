#!/usr/bin/env perl

use strict;
use warnings;
use Data::Dumper;

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

sub convert {
    my ($num) = @_;
    return (substr($num, 0, 2) eq '0x') ? hex($num) : $num;
}

my %COMMANDS = (
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
    DIAG => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^"([^"]+)"$/) {
                print STDERR "# $1\n";
                return;
            }
            die "Invalid DIAG at line " . $self->{line};
        },
        post => sub {},
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
                print "$got == $expected?\n";
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
            my @data = map {convert($_)} split(/\s+/, $mem);
            my $idx = hex($self->{stash}->{start});
            while (my $dat = shift @data) {
                #if ($dat == $self->{memory}->[$idx]) {
                $idx++;
            }
            $self->{line_buf} = [];
            $self->{record} = 0;
        },
    },
    RESET => {
        args => sub {
            my ($self) = @_;
            $self->{line_buf} = [];
            $self->{src} = [];
        },
        post => sub {},
    },
    STACK => {
        args => sub {},
        post => sub {},
    },
);

sub run_test {
    my ($self) = @_;
    write_lines("test.asm", $self->{src});
    `z80asm-gnu -o test.bin test.asm`;
    open(my $fh, './z8t -r test.bin|') || die "unable to open pipe to './z8t': $!";
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
    }
    close $fh;
}

sub run_test_file {
    my ($test_file) = @_;
    my $self = {
        line => 0,
        src => [],
        current_command => undef,
        line_buf => [],
    };
    open(my $fh, '<', $test_file) || die "unable to open test file '$test_file': $!";
    my $no = 0;
    LINE: while (my $line = <$fh>) {
        chomp $line;
        $self->{line}++;
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

sub get_args {
    my @test_files;
    for my $arg (@ARGV) {
        if ($arg =~ /^-/) {
        }
        else {
            push @test_files, $arg;
        }
    }
    return {
        test_files => \@test_files,
    };
}

sub main {
    my ($args) = @_;
    for my $test_file (@{$args->{test_files}}) {
        run_test_file($test_file);
    }
}

my $args = get_args();

exit main($args);
