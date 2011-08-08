/// ----------------------------------------*- mode: C++; -*--
/// @file msghandle.h
/// message handle object type
/// ----------------------------------------------------------
/// $Id: msghandle.h 5249 2010-04-26 08:40:59Z roehricht $
/// $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/include/msghandle.h $
// ===========================================================
//                      
// Copyright (C) 2005-2010, all rights reserved by
// - Institute of Telematics, Karlsruhe Institute of Technology
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
#ifndef MSGHANDLE_H
#define MSGHANDLE_H

/**
 *
 * The message handle is used to refer locally to messages
 * that GIST should send. Sometimes messages cannot be sent
 * due to errors and failures. In order to report transport
 * failure, the msghandle is used as reference to the affected 
 * message.
 *
 * Since the GIST API doesn't specify how the msg handle 
 * is structured, it could be everything. This implementation
 * should be at least flexible in using different types alternatively.
 * Basically it could be seen as opaque bunch of bytes.
 *
 **/

namespace ntlp 
{
  // #define MSG_HANDLE_GOETTINGEN
#ifdef MSG_HANDLE_GOETTINGEN
	struct msghandle_goe_t {
		msghandle_goe_t(uint32 x) {
			
		};

		msghandle_goe_t() {
			
		};
		
		unsigned char handle[16];
	};
  typedef struct msghandle_goe_t msghandle_t; // compatability with Goettingen API
#else
  typedef uint32 msghandle_t; // that's for GIST-ka
#endif

} // end namespace 
#endif
