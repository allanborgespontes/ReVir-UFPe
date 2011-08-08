#! /usr/bin/env python
#
# Like eval_journal.py, but dumps LaTeX-Tables.
#
# $Id: eval_journal.py 2296 2006-11-08 11:56:24Z stud-matfried $
# $HeadURL: https://i72projekte.tm.uka.de/nsis/natfw-nslp/trunk/eval/eval_journal.py $
#
import sys
import stats
import math

if len(sys.argv) != 4:
	print >>sys.stderr, 'Usage: eval_table.py mp_id_1 mp_id_2 Label' 
	sys.exit(1)

mp_id_start = int(sys.argv[1])
mp_id_stop = int(sys.argv[2])
label = sys.argv[3]

times = { }
differences = [ ]

for line in sys.stdin:
	if line.startswith('#'):
		continue

	(mp_id, thread_id, secs, ns) = [int(x) for x in line.split()]
	nanosecs = (secs*1000000000+ns)

	#print mp_id, thread_id, nanosecs

	if mp_id == mp_id_start:
		times[thread_id] = nanosecs
	elif mp_id == mp_id_stop and times.has_key(thread_id):
		differences.append(nanosecs - times[thread_id])
		del times[thread_id]


# print 'Values = %8d' % len(differences)
# print 'Min    = %8d' % min(differences)
# print 'Max    = %8d' % max(differences)
# print 'Mean   = %8d' % stats.mean(differences)
# print 'Stdev  = %8d' % stats.stdev(differences)

# label = '#' + str(mp_id_start) + '-#' + str(mp_id_stop)

# print '\hline'
# print 'Benchmark & Min & Max & Mean & StdDev \\\\'
# print '\hline'
# print '\hline'

# no result for these meassuring points
if len(differences) == 0:
	sys.exit(0)

line = '%s & %.3f & %.3f & %.3f & %.3f \\\\' % (
	label,
	min(differences) / 1000.0,
	max(differences) / 1000.0,
	stats.mean(differences) / 1000.0,
	stats.stdev(differences) / 1000.0)

line = line.replace('.', ',')

print line

# EOF
