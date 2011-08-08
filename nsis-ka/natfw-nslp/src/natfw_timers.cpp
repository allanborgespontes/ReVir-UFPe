/// ----------------------------------------*- mode: C++; -*--
/// @file natfw_timers.cpp
/// Timer-related classes.
/// ----------------------------------------------------------
/// $Id: natfw_timers.cpp 2558 2007-04-04 15:17:16Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/natfw_timers.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#include "natfw_timers.h"
#include "dispatcher.h"


using namespace natfw;


/**
 * Constructor.
 */
timer::timer(session *s)
		: id(0), owning_session(s) {
	// nothing to do
}


void timer::start(dispatcher *d, int seconds) {
	id = d->start_timer(owning_session, seconds);
}

void timer::restart(dispatcher *d, int seconds) {
	id = d->start_timer(owning_session, seconds);
}

void timer::stop() {
	id = 0;
}


// EOF
