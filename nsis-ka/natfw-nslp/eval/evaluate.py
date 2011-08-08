#! /usr/bin/env python
#
# Evaluate the journal printed by the benchmark class.
#
# Note: This script requires standard python and the python-stats module
#
# Usage: evaluate.py [-l] < benchmark_journal.txt
#
# $Id: evaluate.py 2307 2006-11-10 17:08:05Z stud-matfried $
# $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/eval/evaluate.py $
#
import sys
import stats
import getopt

points = (
	(3, 4, 'Mapping'),
	(9, 10, 'Deserialisierung'),
	(5, 6, 'Session-Manager'),
	(13, 14, 'Protokoll-Automat'),
	(7, 8, 'Serialisierung'),
	(3, 14, 'Gesamt')
)


latex_header = '''\
\\begin{table}[h]
\\begin{center}
\\begin{tabular}{|l|r|r|r|r|}
\\hline
Benchmark & Min & Max & Mean & StdDev \\\\
\\hline
\\hline\
'''

latex_footer = '''\
\\hline
\\end{tabular}
\\caption{Werte in $\mu s$}
%\\label{tab:ni_1}
\\end{center}
\\end{table}\
'''


def calc_differences(data, start, stop):
	differences = [ ]
	#
	for (thread, tuples) in data.items():
		time = 0
		for (mp_id, nanosecs) in tuples:
			if mp_id == start:
				time = nanosecs
			elif mp_id == stop and time != 0:
				differences.append(nanosecs - time)

	return differences


# returns a dictionary
# keys are Thread-IDs, values are lists of (MP, nanosecs) values
def read_data_file(in_file):
	data = { }

	for line in in_file:
		if line.startswith('#'):
			continue

		(mp_id, thread_id, secs, ns) = [ int(x) for x in line.split()]
		nanosecs = (secs*1000000000+ns)

		#print mp_id, thread_id, nanosecs

		if not data.has_key(thread_id):
			data[thread_id] = [ ]

		data[thread_id].append( (mp_id, nanosecs) )

	return data


def print_latex_stats(diffs, label):
	print '%s & %.3f & %.3f & %.3f & %.3f \\\\' % (
		label,
		min(diffs) / 1000.0,
		max(diffs) / 1000.0,
		stats.mean(diffs) / 1000.0,
		stats.stdev(diffs) / 1000.0)


def print_ascii_stats(diffs, label):
	print '%8d %8d %8d %8d %8d   %-20s' % (
		len(diffs), min(diffs), max(diffs),
		stats.mean(diffs), stats.stdev(diffs), label)


try:
	opts, args = getopt.getopt(sys.argv[1:], 'l')
except getopt.GetoptError, e:
	print "error:", e
	sys.exit(1)

latex_mode = False
for (o, a) in opts:
	if o == '-l':
		latex_mode = True


data = read_data_file(sys.stdin)

if latex_mode:
	print latex_header
else:
	print '# Values      Min      Max     Mean    Stdev   Benchmark'

for (mp_id_start, mp_id_stop, label) in points:
	diffs = calc_differences(data, mp_id_start, mp_id_stop)

	if len(diffs) == 0:
		continue		# no matching measuring points
	
	if latex_mode:
		print_latex_stats(diffs, label)
	else:
		print_ascii_stats(diffs, label)

if latex_mode:
	print latex_footer

# EOF
