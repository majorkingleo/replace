#!/usr/local/bin/perl

# Author: Philipp Jocham, 23.02.2000
# $Id: ap.pl,v 1.1.1.1 2003/02/07 12:01:57 mmovia Exp $
#
# sends a ping to a host and print the average round-trip time
# on stdout, 0 if no connection
#
# 	0 = ok
#       1 = no connection
#how many packets should be send
$count = 3;


#--------end of user config section--------------------------------

die "Usage: $0 <host>" if(@ARGV != 1);

if($^O eq "linux") {
	$para = "-c";
} elsif($^O eq "aix") {
	$para = "32";
} else {
	$para = "-n";
}

@ping = `ping $ARGV[0] $para $count`;

$avg = 0;
$n = 0;
$noconnect = 1;
if($^O eq "MSWin32") {
	foreach $line (@ping) {
		if($line =~ /time[<=](\d+)ms/) {
			$avg += $1;
			$n++;
		} else {
			if($line =~ /Zeit[<=](\d+)ms/) {
				$avg += $1;
				$n++;
			}
		}
		
	}
	if($n) {
		$noconnect = 0;
		$avg = $avg/$n;
		$avg = 0.01 unless $avg > 10; #10 ms is minimum on windows
	}
} else {
	foreach $line (@ping) {
		if($line =~ /100\% packet loss$/) {
			$avg = 0;
			last;
		} elsif($line =~ /^round-trip\s+(\(ms\)\s+)?min\/avg\/max = [\d\.]+\/([\d\.]+)\/[\d\.]+(\s+ms)?$/) {
			$avg = $2 > 0.01 ? $2 : 0.01;
			$noconnect = 0;
		}
	}

}

$avg = -1 if $noconnect;
print "$avg\n";

exit $noconnect;

