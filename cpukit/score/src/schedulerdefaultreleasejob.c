/**
 * @file
 *
 * @brief Default Scheduler Release Job Operation
 *
 * @ingroup ScoreScheduler
 */

/*
 *  Copyright (C) 2011 Petr Benes.
 *  Copyright (C) 2011 On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/score/scheduler.h>

Thread_Control *_Scheduler_default_Release_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread,
  uint64_t                 deadline
)
{
  (void) scheduler;
  (void) the_thread;
  (void) deadline;

  return NULL;
}

Thread_Control *_Scheduler_default_Cancel_job(
  const Scheduler_Control *scheduler,
  Thread_Control          *the_thread
)
{
  (void) scheduler;
  (void) the_thread;

  return NULL;
}
