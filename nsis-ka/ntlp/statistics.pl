#! /usr/bin/env perl

print "Statistics extractor from timedump\n\n";



print "Getting startup time\n\n";

open(INFILE, "timedump") or die "Can't open timedump: $!";

@lines = <INFILE>;
$firstline = $lines[0];

$starttime =23;
@line = split(/ - /, $firstline);
$starttime = $line[0];

print "Starting timestamp: " . $starttime . "\n\n";





open(INFILE, "timedump") or die "Can't open timedump: $!";



open(OUTFILE, ">statemodule_send") or die "Can't open statemodule_send";

use Statistics::Basic::StdDev;

my $mo = new Statistics::Basic::StdDev([]);


print "############# State Module SEND ##################################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;


while (<INFILE>) {
    
    $timestamp = 0;
   
   

    if (/State Module/) {
	
    @line = split(/ - /, $_);

	if ($line[2]=~ /IN API/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT SIG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";

my $mo = new Statistics::Basic::StdDev([]);

print "############# State Module RECV ##################################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

open(OUTFILE, ">statemodule_recv") or die "Can't open statemodule_recv";

open(INFILE, "timedump") or die "Can't open timedump: $!";
while (<INFILE>) {
    
    $timestamp = 0;

    if (/State Module/) {
	
	@line = split(/ - /, $_);

	# incoming message
	if ($line[2]=~ /IN SIG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "SIG In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	# reset if no delivery
	if ($line[2]=~ /OUT SIG/) {
	    #$oldtimestamp = $line[0];
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=0;
	}

	#API CALL
	if (($line[2]=~ /OUT API/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "API Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";

open(OUTFILE, ">signaling_send") or die "Can't open signaling_send";

my $mo = new Statistics::Basic::StdDev([]);

print "############# Signaling Module SEND ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/Signaling Module/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN SIG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">signaling_recv") or die "Can't open signaling_recv";


my $mo = new Statistics::Basic::StdDev([]);

print "############# Signaling Module RECV ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/Signaling Module/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT SIG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);

	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";

open(INFILE, "timedump") or die "Can't open timedump: $!";

open(OUTFILE, ">tp_over_udp_send") or die "Can't open tp_over_udp_send";


my $mo = new Statistics::Basic::StdDev([]);



print "############# TPoverUDP Module SEND ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over UDP/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP Call/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT MSG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";



open(INFILE, "timedump") or die "Can't open timedump: $!";
my $mo = new Statistics::Basic::StdDev([]);
open(OUTFILE, ">tp_over_udp_recv") or die "Can't open tp_over_udp_recv";

print "############# TPoverUDP Module RECV ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over UDP/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN MSG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_uds_send") or die "Can't open tp_over_uds_send";


my $mo = new Statistics::Basic::StdDev([]);

print "############# TPoverUDS Module SEND ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over UDS/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP Call/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT MSG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";



open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_uds_recv") or die "Can't open tp_over_uds_recv";

my $mo = new Statistics::Basic::StdDev([]);

print "############# TPoverUDS Module RECV ##############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over UDS/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN MSG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";








open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_queryencap_send") or die "Can't open tp_queryencap_send";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TPqueryEncap Module SEND ###########################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TPqueryEncap/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP Call/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT MSG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_queryencap_recv") or die "Can't open tp_queryencap_recv";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TPqueryEncap Module RECV ###########################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TPqueryEncap/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN MSG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";




open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_tcp_send") or die "Can't open tp_over_tcp_send";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TP over TCP Module SEND ############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over TCP/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP CALL/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT MSG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_tcp_recv") or die "Can't open tp_over_tcp_recv";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TP over TCP Module RECV ############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over TCP/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN MSG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";






open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_tls_send") or die "Can't open tp_over_tls_send";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TP over TLS_TCP Module SEND ########################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over TLS/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP CALL/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT MSG/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">tp_over_tls_recv") or die "Can't open tp_over_tls_recv";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TP over TLS Module RECV ############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/TP over TLS/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN MSG/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";







open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">apiwrapper_send") or die "Can't open apiwrapper_send";
my $mo = new Statistics::Basic::StdDev([]);

print "############# TP over APIWrapper SEND ############################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/APIWrapper/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN TP/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT API/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">apiwrapper_recv") or die "Can't open apiwrapper_recv";
my $mo = new Statistics::Basic::StdDev([]);

print "############# APIWrapper RECV ####################################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if (/APIWrapper/) {
	
	@line = split(/ - /, $_);

	if ($line[2]=~ /IN API/) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /OUT TP/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";






open(INFILE, "timedump") or die "Can't open timedump: $!";
open(OUTFILE, ">queue_state_in_sig") or die "Can't open queue_state_in_sig";
my $mo = new Statistics::Basic::StdDev([]);

print "############# Queue Statemodule -> Signaling Module ##############\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if ((/Signaling Module/) || (/State Module/)) {
	
	@line = split(/ - /, $_);

	if (($line[2]=~ /OUT SIG/) && ($line[1]=~ /State/)) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /IN SIG/) && ($line[1]=~ /Signaling/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";



open(INFILE, "timedump") or die "Can't open timedump: $!";
my $mo = new Statistics::Basic::StdDev([]);

print "############# Queue Statemodule <- Signaling Module ##############\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if ((/Signaling Module/) || (/State Module/)) {
	
	@line = split(/ - /, $_);

	if (($line[2]=~ /OUT SIG/) && ($line[1]=~ /Signaling/)) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	}

	if (($line[2]=~ /IN SIG/) && ($line[1]=~ /State/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"


	}
    
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";






open(INFILE, "timedump") or die "Can't open timedump: $!";
my $mo = new Statistics::Basic::StdDev([]);

print "############# Queue Statemodule -> APIWrapper ###################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if ((/APIWrapper/) || (/State/)) {
	
	@line = split(/ - /, $_);

	if (($line[2]=~ /OUT API/) && ($line[1]=~ /State/)) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	    
	}
	
	if (($line[2]=~ /IN API/) && ($line[1]=~ /API/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	    
	}
	
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";


open(INFILE, "timedump") or die "Can't open timedump: $!";
my $mo = new Statistics::Basic::StdDev([]);

print "############# Queue Statemodule <- APIWrapper ###################\n";


$oldtimestamp = 0;
$following = 0;
$count = 0;
$sum = 0;
$max = 0;
$min = 999999999;

$timesum = 0;

while (<INFILE>) {
    
    $timestamp = 0;

    if ((/APIWrapper/) || (/State/)) {
	
	@line = split(/ - /, $_);

	if (($line[2]=~ /OUT API/) && ($line[1]=~ /API/)) {
	    $oldtimestamp = $line[0];
	    if ($oldtimestamp < $starttime) { $oldtimestamp = $oldtimestamp + 3600000000; }
	    #print "API In :";
	    #print $oldtimestamp;
	    #print "\n";
	    $following=1;
	    
	}
	
	if (($line[2]=~ /IN API/) && ($line[1]=~ /State/) && ($following == 1)) {
	    $timestamp = $line[0];
	    if ($timestamp < $starttime) { $timestamp = $timestamp + 3600000000; }
	    #print "Sig Out:";
	    my $difference = $timestamp - $oldtimestamp;
	    #print $timestamp;
	    #print " - ";
	    #print $difference;
	    #print "\n";
	    $following=0;
	    $count++;
	    $sum +=$difference;
	    if ($difference > $max) {
		$max = $difference;
	    }
	    if ($difference < $min) {
		$min = $difference;
	    }
	    $mo->ginsert($difference);
	    $timesum = $timesum + $difference;
	    print OUTFILE $timestamp . " " . $difference . "\n"

	    
	}
	
	
    }
}
print "#   EVENTS:          ";
print $count;
print "\n";

print "#   MAX (msec):       ";
print $max / 1000;
print "\n";

print "#   MIN (msec):       ";
print $min / 1000;
print "\n";

print "#   StdDev (msec):    ";
print $mo->query / 1000;
print "\n";

print "#   AVERAGE (msec):   ";
if ($count != 0 ) {
    $result = $sum / $count;
    print $result / 1000;
}
print "\n";



