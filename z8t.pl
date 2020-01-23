#!/usr/bin/env perl

use strict;
use warnings;

sub run_test_file {
    my ($test_file) = @_;
    my $self = {
        line => 0,
        init => "",
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
        push @{$self->{line_buf}}, $line;
    }
    if ($self->{current_command}) {
        command_post($self, $self->{current_command});
    }

    close $fh;
}

my %COMMANDS = (
    INIT => {
        args => sub {},
        post => sub {
            my ($self) = @_;
            $self->{init} = join("\n", @{$self->{line_buf}});
            $self->{line_buf} = [];
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
                $self->{include} = slurp_file($1);
                return;
            }
            die "Invalid INCLUDE at line " . $self->{line};
        },
        post => sub {},
    },
    TEST => {
        args => sub {},
        post => sub {
            my ($self) = @_;
            my $test = join("\n", @{$self->{line_buf}});
            $self->{line_buf} = [];
            run_test($self, $test);
        },
    },
    REG => {
        args => sub {
            my ($self, $rest) = @_;
            if ($rest =~ /^([AFBCDEHLSP]+)\s+([a-zA-Z0-9]+)\s+"([^"]+)"/) {
                my ($register, $expected, $name) = ($1, $2, $3);
                return;
            }
            die "Invalid REG ('$rest') at line " . $self->{line};
        },
        post => sub {},
    },
    MEM => {
        args => sub {
        },
        post => sub {
        },
    },
    RESET => {
        args => sub {
            my ($self) = @_;
        },
        post => sub {
        },
    },
);

sub run_test {
    my ($self, $test) = @_;
    print $self->{init} . "\n" . $test . "\n";
}

sub slurp_file {
    my ($file_name) = @_;
    open(my $fh, '<', $file_name) || die "unable to open file '$file_name': $!";
    my $text = "";
    while (my $line = <$fh>) {
        $text .= $line;
    }
    close $fh;
    return $text;
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
