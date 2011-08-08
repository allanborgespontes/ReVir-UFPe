#! /usr/bin/env python
#
# Read a NATFW log file and check how long it took to establish sessions.
#
# Usage: gist_analyze < natfwd.log > out.dat
#
import sys
import re

# Note: all times in milliseconds
started_sessions = { }
diffs = [ ]

PATTERN_TIME = r'.*\d{4}-\d{2}-\d{2} (\d\d):(\d\d):(\d\d)\.(\d+)'

PATTERN_CREATED = PATTERN_TIME + r'.*created new NF session (.*)'
PATTERN_INIT = PATTERN_TIME + r'.*initiated session (.*)'

PC = re.compile(PATTERN_CREATED)
PI = re.compile(PATTERN_INIT)

for line in sys.stdin:
	#m = re.match(PATTERN_CREATED, line)
	m = PC.match(line)

	if m:
		(hour, min, sec, ms) = [ int(x) for x in m.groups()[:4] ]
		#print hour, min, sec, ms
		ms = hour*60*60*1000 + min*60*1000 + sec*1000 + ms
		sid = m.group(5)
		started_sessions[m.group(5)] = ms
		#print 'created session', sid, ms
		continue


	#m = re.match(PATTERN_INIT, line)
	m = PI.match(line)

	if m:
		sid = m.group(5)
		if started_sessions.has_key(sid):
			(hour, min, sec, ms) = [ int(x) for x in m.groups()[:4] ]
			ms = hour*60*60*1000 + min*60*1000 + sec*1000 + ms
			#print 'created, time diff', ms - started_sessions[sid]
			#diffs.append( ms - started_sessions[sid] )
			print ms - started_sessions[sid]
	

#for d in diffs:
#	print d

# EOF
