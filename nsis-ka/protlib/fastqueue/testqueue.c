/// ----------------------------------------*- mode: C++; -*--
/// @file testqueue.c
/// Testing fastqueue.
/// ----------------------------------------------------------
/// $Id: testqueue.c 2872 2008-02-18 10:58:03Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/protlib/trunk/fastqueue/testqueue.c $
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

/**
 * @ingroup fastqueue
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h> /* Headers for POSIX-Threads */
#include <time.h>    /* needed for getting Timestamps */

#include "fastqueue.h"

#define error_check(status,string) \
        if (status==-1) perror(string);

#define MAXELEMENTS 10000

                         /*** global variables ***/
pthread_t       producer_thread,             /* Thread Objects (sim. to TCB) */
                consumer_thread;

float           queuetime, porttime;


/** both queues must be created before monitor tasks are started **/
queue_t       *consumer_cmdq;     /** queue for consumertask **/
struct timespec ts_start,ts_end;
int status;              /* Hold status from pthread_ calls */

void *producertask(void * argp)
{
  long i;
  fprintf(stderr,"QUEUETEST started. Please wait.\n");
  clock_gettime(CLOCK_REALTIME,&ts_start);
  for (i=1; i<=MAXELEMENTS; i++)
      enqueue_element_signal(consumer_cmdq, (void *) i);

  return NULL;
}

void *consumertask(void * argp)
{
  long j;
  /** test queue **/
  while ((j= (int) dequeue_element_wait(consumer_cmdq))<MAXELEMENTS);

  clock_gettime(CLOCK_REALTIME,&ts_end);  
  queuetime= ts_end.tv_sec-ts_start.tv_sec +
             (ts_end.tv_nsec-ts_start.tv_nsec)*1E-9;
  fprintf(stderr,"QUEUETEST stopped (%d elements): %gs\n",MAXELEMENTS,queuetime);
  return NULL;
}

int
main(int argc, char **argv)
{
  void *exit_value;        /* for pthread_join */

  fprintf(stderr,"<program start>\n");
  /** create a queue **/
  if ((consumer_cmdq= create_queue("testqueue"))==NULL) exit(1);

  /** Start Threads **/
  status= pthread_create(&consumer_thread,
                         NULL,
                         consumertask,
                         NULL);
  error_check(status,"fatal: cannot create consumertask");

  status= pthread_create(&producer_thread,
                         NULL,
                         producertask,
                         NULL);
  error_check(status,"fatal: cannot create producertask");

  /** wait for threads to end **/
  status= pthread_join(producer_thread, &exit_value);
  error_check(status,"pthread_join");

  status= pthread_join(consumer_thread, &exit_value);
  error_check(status,"pthread_join");

  /** destroy all queues **/
  status= destroy_queue(consumer_cmdq);
  error_check(status,"destroying consumer queue");

  fprintf(stderr,"<program exited normally>\n");
   return 0;
}
/**** end of source ****/

//@}
