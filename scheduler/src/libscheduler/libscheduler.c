/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"



typedef enum {IDLE = -1, SUSPENDED, RUNNING} job_state_t;

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/
typedef struct _job_t
{
  int job_number;
  int running_time;
  int elapsed_time;
  int priority;
  job_state_t state;

} job_t;



struct _global_state
{
  int cores;
  int schedulable_core;
  int time;  
  scheme_t scheme;
} global_state;

int comparer_FCFS(const void* job1, const void* job2){
  return ((job_t*)job2)->job_number - ((job_t*)job1)->job_number;
}

int comparer_SJF(const void* job1, const void* job2){return 0;}
int comparer_PSJF(const void* job1, const void* job2){return 0;}
int comparer_PRI(const void* job1, const void* job2){return 0;}
int comparer_PPRI(const void* job1, const void* job2){return 0;}
int comparer_RR(const void* job1, const void* job2){return 0;}

int (*comparers[6]) (const void*,const void*) = {comparer_FCFS, 
                                                comparer_SJF,
                                                comparer_PSJF,
                                                comparer_PRI,
                                                comparer_PPRI,
                                                comparer_RR};
        
priqueue_t* priqueues;


/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
  global_state.cores = cores;
  global_state.schedulable_core = 0;
  global_state.scheme = scheme;
  priqueues = malloc(cores*sizeof(priqueue_t));
  for(int core = 0; core < cores; ++core)
     priqueue_init(&priqueues[core], comparers[scheme]); 
}

/*sets the core on which the next job must be scheduled. The core
  will be selected in such a way to balance the total load on all cores.*/ 
void set_next_schedulable_core(){
  int smallest_core = 0;
  for(int core = 0; core < global_state.cores; ++core){
    if(priqueue_size(&priqueues[core]) < priqueue_size(&priqueues[smallest_core])){
      smallest_core = core;
    }
  }
  global_state.schedulable_core = smallest_core;
}


/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  global_state.time = time;
  job_t *job = malloc(sizeof(job_t));  
  job->job_number = job_number;
  job->running_time = running_time;
  job->elapsed_time = 0;
  job->priority = priority;

  int schedulable_core = global_state.schedulable_core;
  int empty_queue = priqueue_size(&priqueues[global_state.schedulable_core]) == 0;

  //schedule the job on the schedulable core
  switch(global_state.scheme){
    case FCFS:
      priqueue_offer(&priqueues[global_state.schedulable_core], job); //push to the queue. the comparator function should take care of the location within the queue.
      break;
    default:{} 
  }
  //determine which core must take the next job that arrives
  set_next_schedulable_core();
  //if this core's queue was empty (available), return this core, for it will start running this job right now
  if(empty_queue){
    return schedulable_core;
  }
	return -1;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  priqueue_poll(&priqueues[core_id]); //remove the finished job from this core's queue (tail);
  int next_job_number;
  if(priqueue_size(&priqueues[core_id]) == 0)
    return -1; //there's no next job on this queue
  next_job_number = ((job_t*)priqueue_peek(&priqueues[core_id]))->job_number; //get the next job on this queue
  if(core_id == global_state.schedulable_core) //if the given core is to run the next job in the next cycle, return the next job number
	  return next_job_number;
  return -1; //otherwise the given core will idle
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return 0.0;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return 0.0;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return 0.0;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{

}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
  printf("\n  (H->T)\n");
  for(int core = 0; core < global_state.cores; ++core){
    printf("  Core %d Queue: ", core);
    for(node_t *node = (&priqueues[core])->head; node != NULL; node = node->next){
      printf("%d", ((job_t*)node->data)->job_number);
    }
    printf("\n");
  }

}
