#! /usr/bin/env python
#
# Dump results from a journal in a format suitable for gnuplot.
#
# $Id: eval_journal.py 2296 2006-11-08 11:56:24Z stud-matfried $
# $HeadURL: https://i72projekte.tm.uka.de/nsis/natfw-nslp/trunk/eval/eval_journal.py $
#
import sys
import stats
import math

if len(sys.argv) != 3:
	print >>sys.stderr, 'Usage: dump_gnuplot.py mp_id_1 mp_id_2' 
	sys.exit(1)

mp_id_start = int(sys.argv[1])
mp_id_stop = int(sys.argv[2])

times = { }
i = 1

for line in sys.stdin:
	if line.startswith('#'):
		continue

	(mp_id, thread_id, secs, ns) = [int(x) for x in line.split()]
	nanosecs = (secs*1000000000+ns)

	if mp_id == mp_id_start:
		times[thread_id] = nanosecs
	elif mp_id == mp_id_stop and times.has_key(thread_id):
		diff = nanosecs - times[thread_id]
		print '%d %f' % (i, diff / 1000.0)
		del times[thread_id]
		i+=1

# EOF
