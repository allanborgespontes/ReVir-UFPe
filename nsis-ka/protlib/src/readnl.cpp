/// ----------------------------------------*- mode: C++; -*--
/// @file readnl.cpp
/// Read netlink socket
/// ----------------------------------------------------------
/// $Id: readnl.cpp 6176 2011-05-20 07:39:55Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/src/readnl.cpp $
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
#include <net/if.h>
#include <linux/types.h> // in case that this one is not included in netlink.h
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <iostream>

#include "readnl.h"

using namespace std;

namespace protlib {

	namespace util {
// read a netlink reply in it's entirety
// responsibility of caller to make buf must at least NL_BUFSIZE
int
readnl(int sock, char *buf)
{
  unsigned int nread = 0, ntread = 0;
  u_char flag = 0;
  char *tmp = buf;
  struct nlmsghdr *nlhdrp = NULL;
	    
  // read the reply, check for multi-part reply
  // fill up buffer to NL_BUFSIZE bytes at most
  do 
  {
    // we can't read more than NL_BUFSIZE bytes
    if ( (nread = read(sock, tmp, NL_BUFSIZE - ntread )) < 0)
      cout << "Error on Netlink Socket read" << endl;
		
    nlhdrp = (struct nlmsghdr *)tmp;
		
    // FIRST ALWAYS: check to make sure the message structure is ok
    // then check to make sure type isn't error type
    if ( (NLMSG_OK(nlhdrp, nread) == 0) || (nlhdrp->nlmsg_type == NLMSG_ERROR) )
      return -1;
		
    //if this is the last message then it contains no data
    //so we dont want to return it
    if (nlhdrp->nlmsg_type == NLMSG_DONE)
      flag = 1;
    else {
      tmp += nread;
      ntread += nread;
    }
		
    // in this case we never had a multipart message
    if ( (nlhdrp->nlmsg_flags & NLM_F_MULTI) == 0 )
      flag = 1;		
  } 
  while (flag == 0);
  //nlhdrp->nlmsg_seq != seq || nlhdrp->nlmsg_pid != pid || 

  return ntread;
}

	} // end namespace util

} // end namespace protlib
