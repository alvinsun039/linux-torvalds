#!/usr/bin/perl
# [[file:../agendas/snippets.org::*generate excel with perl][generate excel with perl:1]]
# install Spreadsheet::ParseXLSX
# install Spreadsheet::Read
# install Excel::Writer::XLSX
# install utf8::all
use warnings;
use strict;
use bigint;
use utf8::all;
use Excel::Writer::XLSX;
use Spreadsheet::Read;
use open qw( :std :encoding(UTF-8) );

use Getopt::Long qw(GetOptions);
Getopt::Long::Configure qw(gnu_getopt);

my $debug;
my $output = "prfcnt.xlsx";

GetOptions(
    'debug|d' => \$debug,
    'output|o=s' => \$output,
    ) or die "Usage: $0 [-d] [-o name.xlsx] \${gb_version} \${layout.xlsx} \${primary.hex} \${secondary.hex}\n";

my $num_args = $#ARGV + 1;
if ($num_args != 4) {
    die "Usage: $0 [-d] [-o name.xlsx] \${gb_version} \${layout.xlsx} \${primary.hex} \${secondary.hex}\n";
}

my ($version, $layout_filename, $primary, $secondary) = @ARGV;

my $layout = Spreadsheet::Read->new($layout_filename);
my $meta_sheet = $layout->sheet("meta");

sub find_row {
    my ($name, $value, $sheet) = @_;
    my @header = $sheet->row(1);
    my $column = -1;
    my $row;

    foreach my $n (1 .. scalar @header) {
        if ($header[$n-1] eq $name) {
            $column = $n - 1;
            last;
        }
    }
    if ($column == -1) {
        printf("can not find row by name: %s, value: %s, under: %s", $name, $value, $sheet) if $debug;
        return $row;
    }
    my @rows = $sheet->rows();
    foreach my $r (2 .. scalar @rows) {
        if ($rows[$r - 1][$column] eq $value) {
            $row = $rows[$r -1];
            last;
        }
    }
    return $row;
}

sub locate {
    my ($addr, $sel, @regions) = @_;
    my $region;
    foreach my $r (2 .. scalar @regions) {
        if ($regions[$r - 1][0] eq "N/A") {
            printf("0x%04X %d is N/A\n", $addr, $sel) if $debug;
            next;
        }
        if ($regions[$r - 1][0] eq "") {
            printf("0x%04X %d not found\n", $addr, $sel) if $debug;
            last;
        }
        my $base = hex($regions[$r - 1][0]);
        my $size = hex($regions[$r - 1][1]);
        my $se = int($regions[$r - 1][2]);
        # printf("region: base: 0x%04X size: 0x%04X sel: %d\n", $base, $size, $se) if $debug;
        if ($addr >= $base && $addr < $base+$size && $se == $sel) {
            printf("found region: base: 0x%04X size: 0x%04X \n", $base, $size) if $debug;
            $region = $regions[$r -1];
            last;
        }
    }
    return $region;
}


sub counter_at {
    my ($addr, $sel, @regions) = @_;
    my $counter_found;

    my $cnt_addr = $addr & 0xFFFF;
    #$cnt_addr = $cnt_addr - 0xe20;
    my $region = locate($cnt_addr, $sel, @regions);
    if (not defined $region) {
        printf("region with addr: 0x%09X not found\n", $addr) if $debug;
        return $counter_found;
    }
    printf("found region: %s %s %s\n", $region->[2], $region->[3], $region->[4]) if $debug;
    my $base = hex($region->[0]);
    my @module_counters = $layout->sheet($region->[4])->rows();
    my $idx = int($cnt_addr - $base)/4;
    printf("find counter: addr: 0x%09x at: 0x%04X with idx: %d\n", $addr, $cnt_addr, $idx) if $debug;
    $counter_found  = $module_counters[$idx+1];

    return $counter_found;
}


open(FH, '<', $primary) or die $!;
open(FHS, '<', $secondary) or die $!;

my $prfcnt = Excel::Writer::XLSX->new($output);
my $counters = $prfcnt->add_worksheet("counters");
my $meta_line = find_row("VERSION", "$version", $meta_sheet);
my $layout_sheet_name = $meta_line->[1];
my $layout_sheet = $layout->sheet($layout_sheet_name);
my @regions = $layout_sheet->rows();

while(<FH>){
    my ($addr, $v1, $v2, $v3, $v4) = split(/:?\s/, $_);
    my %counter_values = (hex($addr) => hex($v1),
                          hex($addr)+0x4 => hex($v2),
                          hex($addr)+0x8 => hex($v3),
                          hex($addr)+0xC => hex($v4));
    my $sel = 0;
    for my $k (keys %counter_values) {
        my $counter = counter_at($k, $sel, @regions);
        printf("find addr: 0x%09X value: 0x%08X inside layout: %s with sel: $sel\n", $k, $counter_values{$k}, $layout_sheet_name) if $debug;
        if (not defined $counter) {
            printf("addr: 0x%09X value: 0x%08X not found inside layout: %s\n", $k, $counter_values{$k}, $layout_sheet_name) if $debug;
            next;
        }
        printf("find counter: %s %s %s\n", $counter->[0], $counter->[1], $counter->[2]) if $debug;
        my $region_offset = $k & 0xFF00;
        my $region_index = $region_offset / 0x100;
        my $counter_offset = $k & 0xFF;
        my $counter_index = $counter_offset / 0x4;
        my $row = $region_index * 64 + $counter_index +1;
        my $column = $sel * 4 ;
        printf("addr: 0x%09X value: 0x%08X at region_index: 0x%08X at counter_index: 0x%08X\n", $k, $counter_values{$k}, $region_index, $counter_index) if $debug;
        $counters->write($row, $column, sprintf("0x%08X", $k));
        #$counters->write($row, $column+1, sprintf("%08X", $counter_values{$k}));
        $counters->write($row, $column+1, sprintf("%D", $counter_values{$k}));
        $counters->write($row, $column+2, $counter->[1]);
        $counters->write($row, $column+3,  $counter->[2]);
    }
}

while(<FHS>){
    my ($addr, $v1, $v2, $v3, $v4) = split(/:?\s/, $_);
    my %counter_values = (hex($addr) => hex($v1),
                          hex($addr)+0x4 => hex($v2),
                          hex($addr)+0x8 => hex($v3),
                          hex($addr)+0xC => hex($v4));
    my $sel = 1;
    for my $k (keys %counter_values) {
        my $counter = counter_at($k, $sel, @regions);
        printf("find addr: 0x%09X value: 0x%08X inside layout: %s with sel: $sel\n", $k, $counter_values{$k}, $layout_sheet_name) if $debug;
        if (not defined $counter) {
            printf("addr: 0x%09X value: 0x%08X not found inside layout: %s\n", $k, $counter_values{$k}, $layout_sheet_name) if $debug;
            next;
        }
        printf("find counter: %s %s %s\n", $counter->[0], $counter->[1], $counter->[2]) if $debug;
        my $region_offset = $k & 0xFF00;
        my $region_index = $region_offset / 0x100;
        my $counter_offset = $k & 0xFF;
        my $counter_index = $counter_offset / 0x4;
        my $row = $region_index * 64 + $counter_index +1;
        my $column = $sel * 4 ;
        printf("addr: 0x%09X value: 0x%08X at region_index: 0x%08X at counter_index: 0x%08X\n", $k, $counter_values{$k}, $region_index, $counter_index) if $debug;
        $counters->write($row, $column, sprintf("0x%08X", $k));
        $counters->write($row, $column+1, sprintf("%D", $counter_values{$k}));
        #$counters->write($row, $column+1, sprintf("%08X", $counter_values{$k}));
        $counters->write($row, $column+2, $counter->[1]);
        $counters->write($row, $column+3,  $counter->[2]);
    }
}

#sub calu

$prfcnt->close;
close(FH);
close(FHS);
# generate excel with perl:1 ends here
