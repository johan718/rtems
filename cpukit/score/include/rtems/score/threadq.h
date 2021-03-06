/**
 *  @file
 *
 *  @brief Constants and Structures Needed to Declare a Thread Queue
 *
 *  This include file contains all the constants and structures
 *  needed to declare a thread queue.
 */

/*
 *  COPYRIGHT (c) 1989-2014.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_THREADQ_H
#define _RTEMS_SCORE_THREADQ_H

#include <rtems/score/chain.h>
#include <rtems/score/isrlock.h>
#include <rtems/score/object.h>
#include <rtems/score/priority.h>
#include <rtems/score/rbtree.h>
#include <rtems/score/watchdog.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup ScoreThreadQueue Thread Queue Handler
 *
 *  @ingroup Score
 *
 *  This handler provides the capability to have threads block in
 *  ordered sets. The sets may be ordered using the FIFO or priority
 *  discipline.
 */
/**@{*/

typedef struct _Thread_Control Thread_Control;

typedef struct Thread_queue_Queue Thread_queue_Queue;

typedef struct Thread_queue_Operations Thread_queue_Operations;

typedef struct Thread_queue_Path Thread_queue_Path;

/**
 * @brief Thread queue deadlock callout.
 *
 * @param the_thread The thread that detected the deadlock.
 *
 * @see _Thread_queue_Context_set_deadlock_callout().
 */
typedef void ( *Thread_queue_Deadlock_callout )(
  Thread_Control *the_thread
);

#if defined(RTEMS_MULTIPROCESSING)
/**
 * @brief Multiprocessing (MP) support callout for thread queue operations.
 *
 * @param the_proxy The thread proxy of the thread queue operation.  A thread
 *   control is actually a thread proxy if and only if
 *   _Objects_Is_local_id( the_proxy->Object.id ) is false.
 * @param mp_id Object identifier of the object containing the thread queue.
 *
 * @see _Thread_queue_Context_set_MP_callout().
 */
typedef void ( *Thread_queue_MP_callout )(
  Thread_Control *the_proxy,
  Objects_Id      mp_id
);
#endif

#if defined(RTEMS_SMP)
/**
 * @brief The thread queue gate is an SMP synchronization means.
 *
 * The gates are added to a list of requests.  A busy wait is performed to make
 * sure that preceding requests are carried out.  Each predecessor notifies its
 * successor about on request completion.
 *
 * @see _Thread_queue_Gate_add(), _Thread_queue_Gate_wait(), and
 *   _Thread_queue_Gate_open().
 */
typedef struct {
  Chain_Node Node;

  Atomic_Uint go_ahead;
} Thread_queue_Gate;
#endif

/**
 * @brief Thread queue context for the thread queue methods.
 *
 * @see _Thread_queue_Context_initialize().
 */
typedef struct {
  /**
   * @brief The lock context for the thread queue acquire and release
   * operations.
   */
  ISR_lock_Context Lock_context;

  /**
   * @brief The expected thread dispatch disable level for
   * _Thread_queue_Enqueue_critical().
   *
   * In case the actual thread dispatch disable level is not equal to the
   * expected level, then a fatal error occurs.
   */
  uint32_t expected_thread_dispatch_disable_level;

  /**
   * @brief The clock discipline for the interval timeout.
   * Use WATCHDOG_NO_TIMEOUT to block indefinitely.
   */
  Watchdog_Discipline timeout_discipline;

  /**
   * @brief Interval to wait.
   */
  uint64_t timeout;

  /**
   * @brief Invoked in case of a detected deadlock.
   *
   * Must be initialized for _Thread_queue_Enqueue_critical() in case the
   * thread queue may have an owner, e.g. for mutex objects.
   *
   * @see _Thread_queue_Context_set_deadlock_callout().
   */
  Thread_queue_Deadlock_callout deadlock_callout;

#if defined(RTEMS_MULTIPROCESSING)
  /**
   * @brief Callout to unblock the thread in case it is actually a thread
   * proxy.
   *
   * This field is only used on multiprocessing configurations.  Used by
   * thread queue extract and unblock methods for objects with multiprocessing
   * (MP) support.
   *
   * @see _Thread_queue_Context_set_MP_callout().
   */
  Thread_queue_MP_callout mp_callout;
#endif

#if defined(RTEMS_SMP)
  /**
   * @brief Data to support thread queue enqueue operations.
   */
  struct {
    /**
     * @brief Gate to synchronize thread wait lock requests.
     *
     * @see _Thread_Wait_acquire_critical() and _Thread_Wait_tranquilize().
     */
    Thread_queue_Gate Gate;

    /**
     * @brief The thread queue in case the thread is blocked on a thread queue.
     */
    Thread_queue_Queue *queue;
  } Wait;
#endif
} Thread_queue_Context;

#if defined(RTEMS_SMP)
/**
 * @brief A thread queue link from one thread to another specified by the
 * thread queue owner and thread wait queue relationships.
 */
typedef struct {
  /**
   * @brief Node to register this link in the global thread queue links lookup
   * tree.
   */
  RBTree_Node Registry_node;

  /**
   * @brief The source thread queue determined by the thread queue owner.
   */
  Thread_queue_Queue *source;

  /**
   * @brief The target thread queue determined by the thread wait queue of the
   * source owner.
   */
  Thread_queue_Queue *target;

  /**
   * @brief Node to add this link to a thread queue path.
   */
  Chain_Node Path_node;

  /**
   * @brief The owner of this thread queue link.
   */
  Thread_Control *owner;

  /**
   * @brief The queue context used to acquire the thread wait lock of the
   * owner.
   */
  Thread_queue_Context Queue_context;
} Thread_queue_Link;
#endif

/**
 * @brief Thread priority queue.
 */
typedef struct {
#if defined(RTEMS_SMP)
  /**
   * @brief Node to enqueue this queue in the FIFO chain of the corresponding
   * heads structure.
   *
   * @see Thread_queue_Heads::Heads::Fifo.
   */
  Chain_Node Node;
#endif

  /**
   * @brief The actual thread priority queue.
   */
  RBTree_Control Queue;
} Thread_queue_Priority_queue;

/**
 * @brief Thread queue heads.
 *
 * Each thread is equipped with spare thread queue heads in case it is not
 * enqueued on a thread queue.  The first thread enqueued on a thread queue
 * will give its spare thread queue heads to that thread queue.  The threads
 * arriving at the queue will add their thread queue heads to the free chain of
 * the queue heads provided by the first thread enqueued.  Once a thread is
 * dequeued it use the free chain to get new spare thread queue heads.
 *
 * Uses a leading underscore in the structure name to allow forward
 * declarations in standard header files provided by Newlib and GCC.
 */
typedef struct _Thread_queue_Heads {
  /** This union contains the data structures used to manage the blocked
   *  set of tasks which varies based upon the discipline.
   */
  union {
    /**
     * @brief This is the FIFO discipline list.
     *
     * On SMP configurations this FIFO is used to enqueue the per scheduler
     * instance priority queues of this structure.  This ensures FIFO fairness
     * among the highest priority thread of each scheduler instance.
     */
    Chain_Control Fifo;

#if !defined(RTEMS_SMP)
    /**
     * @brief This is the set of threads for priority discipline waiting.
     */
    Thread_queue_Priority_queue Priority;
#endif
  } Heads;

  /**
   * @brief A chain with free thread queue heads providing the spare thread
   * queue heads for a thread once it is dequeued.
   */
  Chain_Control Free_chain;

  /**
   * @brief A chain node to add these thread queue heads to the free chain of
   * the thread queue heads dedicated to the thread queue of an object.
   */
  Chain_Node Free_node;

#if defined(RTEMS_SMP)
  /**
   * @brief One priority queue per scheduler instance.
   */
  Thread_queue_Priority_queue Priority[ RTEMS_ZERO_LENGTH_ARRAY ];
#endif
} Thread_queue_Heads;

#if defined(RTEMS_SMP)
  #define THREAD_QUEUE_HEADS_SIZE( scheduler_count ) \
    ( sizeof( Thread_queue_Heads ) \
      + ( scheduler_count ) * sizeof( Thread_queue_Priority_queue ) )
#else
  #define THREAD_QUEUE_HEADS_SIZE( scheduler_count ) \
    sizeof( Thread_queue_Heads )
#endif

struct Thread_queue_Queue {
  /**
   * @brief Lock to protect this thread queue.
   *
   * It may be used to protect additional state of the object embedding this
   * thread queue.
   *
   * Must be the first component of this structure to be able to re-use
   * implementation parts for structures defined by Newlib <sys/lock.h>.
   *
   * @see _Thread_queue_Acquire(), _Thread_queue_Acquire_critical() and
   * _Thread_queue_Release().
   */
#if defined(RTEMS_SMP)
  SMP_ticket_lock_Control Lock;
#endif

  /**
   * @brief The thread queue heads.
   *
   * This pointer is NULL, if and only if no threads are enqueued.  The first
   * thread to enqueue will give its spare thread queue heads to this thread
   * queue.
   */
  Thread_queue_Heads *heads;

  /**
   * @brief The thread queue owner.
   */
  Thread_Control *owner;
};

/**
 * @brief Thread queue priority change operation.
 *
 * @param[in] queue The actual thread queue.
 * @param[in] the_thread The thread.
 * @param[in] new_priority The new priority value.
 *
 * @see Thread_queue_Operations.
 */
typedef void ( *Thread_queue_Priority_change_operation )(
  Thread_queue_Queue *queue,
  Thread_Control     *the_thread,
  Priority_Control    new_priority
);

/**
 * @brief Thread queue enqueue operation.
 *
 * A potential thread to update the priority due to priority inheritance is
 * returned via the thread queue path.  This thread is handed over to
 * _Thread_Update_priority().
 *
 * @param[in] queue The actual thread queue.
 * @param[in] the_thread The thread to enqueue on the queue.
 */
typedef void ( *Thread_queue_Enqueue_operation )(
  Thread_queue_Queue *queue,
  Thread_Control     *the_thread,
  Thread_queue_Path  *path
);

/**
 * @brief Thread queue extract operation.
 *
 * @param[in] queue The actual thread queue.
 * @param[in] the_thread The thread to extract from the thread queue.
 */
typedef void ( *Thread_queue_Extract_operation )(
  Thread_queue_Queue *queue,
  Thread_Control     *the_thread
);

/**
 * @brief Thread queue surrender operation.
 *
 * This operation must dequeue and return the first thread on the queue.
 *
 * @param[in] queue The actual thread queue.
 * @param[in] heads The thread queue heads.  It must not be NULL.
 * @param[in] previous_owner The previous owner of the thread queue.
 *
 * @return The previous first thread on the queue.
 */
typedef Thread_Control *( *Thread_queue_Surrender_operation )(
  Thread_queue_Queue *queue,
  Thread_queue_Heads *heads,
  Thread_Control     *previous_owner
);

/**
 * @brief Thread queue first operation.
 *
 * @param[in] heads The thread queue heads.
 *
 * @retval NULL No thread is present on the thread queue.
 * @retval first The first thread of the thread queue according to the insert
 * order.  This thread remains on the thread queue.
 */
typedef Thread_Control *( *Thread_queue_First_operation )(
  Thread_queue_Heads *heads
);

/**
 * @brief Thread queue operations.
 *
 * @see _Thread_wait_Set_operations().
 */
struct Thread_queue_Operations {
  /**
   * @brief Thread queue priority change operation.
   *
   * Called by _Thread_Change_priority() to notify a thread about a priority
   * change.  In case this thread waits currently for a resource the handler
   * may adjust its data structures according to the new priority value.  This
   * handler must not be NULL, instead the default handler
   * _Thread_Do_nothing_priority_change() should be used in case nothing needs
   * to be done during a priority change.
   */
  Thread_queue_Priority_change_operation priority_change;

  /**
   * @brief Thread queue enqueue operation.
   *
   * Called by object routines to enqueue the thread.
   */
  Thread_queue_Enqueue_operation enqueue;

  /**
   * @brief Thread queue extract operation.
   *
   * Called by object routines to extract a thread from a thread queue.
   */
  Thread_queue_Extract_operation extract;

  /**
   * @brief Thread queue surrender operation.
   */
  Thread_queue_Surrender_operation surrender;

  /**
   * @brief Thread queue first operation.
   */
  Thread_queue_First_operation first;
};

/**
 *  This is the structure used to manage sets of tasks which are blocked
 *  waiting to acquire a resource.
 */
typedef struct {
#if defined(RTEMS_SMP)
#if defined(RTEMS_DEBUG)
  /**
   * @brief The index of the owning processor of the thread queue lock.
   *
   * The thread queue lock may be acquired via the thread lock also.  This path
   * is not covered by this field.  In case the lock is not owned directly via
   * _Thread_queue_Acquire(), then the value of this field is
   * SMP_LOCK_NO_OWNER.
   *
   * Must be before the queue component of this structure to be able to re-use
   * implementation parts for structures defined by Newlib <sys/lock.h>.
   */
  uint32_t owner;
#endif

#if defined(RTEMS_PROFILING)
  /**
   * @brief SMP lock statistics in case SMP and profiling are enabled.
   *
   * Must be before the queue component of this structure to be able to re-use
   * implementation parts for structures defined by Newlib <sys/lock.h>.
   */
  SMP_lock_Stats Lock_stats;
#endif
#endif

  /**
   * @brief The actual thread queue.
   */
  Thread_queue_Queue Queue;
} Thread_queue_Control;

/**@}*/

#ifdef __cplusplus
}
#endif

#endif
/* end of include file */
