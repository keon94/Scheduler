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
  int remaining_time;
  int arrival_time;
  int priority;
  job_state_t state;

} job_t;



struct _global_state
{
  int cores;
  int schedulable_core;  
  scheme_t scheme;
} global_state;

int comparer_FCFS(const void* job1, const void* job2){  //insertion when compare(new_val,existing_val) <= 0
  return ((job_t*)job2)->job_number - ((job_t*)job1)->job_number;  //when job1 # is greater than the existing job2 #, insert
}

int comparer_SJF(const void* job1, const void* job2){
  int result = ((job_t*)job2)->running_time - ((job_t*)job1)->running_time;
  if(result == 0)
    return ((job_t*)job1)->arrival_time - ((job_t*)job2)->arrival_time;
  return result;
}
int comparer_PSJF(const void* job1, const void* job2){  
  int result = ((job_t*)job2)->remaining_time - ((job_t*)job1)->remaining_time;
  if(result == 0)
    return ((job_t*)job1)->arrival_time - ((job_t*)job2)->arrival_time;
  return result;
}
int comparer_PRI(const void* job1, const void* job2){
  int result = ((job_t*)job2)->priority - ((job_t*)job1)->priority;
  if(result == 0)
    return ((job_t*)job1)->arrival_time - ((job_t*)job2)->arrival_time;
  return result;
}
int comparer_PPRI(const void* job1, const void* job2){
  return comparer_PRI(job1,job2);
}
int comparer_RR(const void* job1, const void* job2){
  return 0;
}

int (*comparers[6]) (const void*,const void*) = {comparer_FCFS, 
                                                comparer_SJF,
                                                comparer_PSJF,
                                                comparer_PRI,
                                                comparer_PPRI,
                                                comparer_RR};
        
priqueue_t* priqueues;


//called when the scheme is preemptive. called when a 'superior' job arrives and must preempt the currently running one (push it back in the queue)
void nonpreemptive_offer(job_t *new_job){
//Possibly Unnecessary
}

void update_remaining_time(job_t* job, int current_time){
  if(job)
    job->remaining_time = job->running_time - (current_time - job->arrival_time);
}

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

/*sets the core on which the newly received job must be scheduled. The core
  will be selected in such a way to balance the total load on all cores.*/ 
void set_schedulable_core(){
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
  job_t *job = malloc(sizeof(job_t));  
  job->job_number = job_number;
  job->running_time = running_time;
  job->remaining_time = running_time;
  job->arrival_time = time;
  job->priority = priority;
 
  //determine which core must take the arrived job
  set_schedulable_core();
  
  
  int queue_size = priqueue_size(&priqueues[global_state.schedulable_core]);
  int schedulable_core = global_state.schedulable_core;
  int empty_queue = queue_size == 0;
  int preempt = 0;

  //update the rem time of the active job on the target core (if exists)
  update_remaining_time(((job_t*)priqueue_peek(&priqueues[schedulable_core])), time);
  
  //schedule the job on the schedulable core
  switch(global_state.scheme){
    case FCFS:
      priqueue_offer(&priqueues[schedulable_core], job); //push to the queue. the comparator function should take care of the location within the queue.
      break;
    case PSJF:
    case PPRI:
      if(priqueue_offer(&priqueues[schedulable_core], job) == queue_size) //if insertion was done at the tail
      {
        preempt = 1;
      }
      break;
    case SJF:
    case PRI:
      if(priqueue_offer(&priqueues[schedulable_core], job) == queue_size) //if insertion was done at the tail. nore: queue_size has increased by 1 upon calling offer()
      {
        //we must let the current job (at the tail) finish.
        //having entered the body of this if block, offer() will have placed the new job at the tail of the queue, though it should be placed one step next to the tail, which is what this code does
        if(queue_size >= 1) //implies a size of 2 or more 
          swap(&priqueues[schedulable_core], queue_size-1, queue_size);
      }
      break;
    case RR:
      //TODO: Round Robin Scheduling
    break;
    default:{} 
  }
   
  //if this core's queue was empty (available), return this core, for it will start running this job right now
  if(empty_queue || preempt){
    return schedulable_core;
  }
	return -1;
}

//transfers a job from origin queue to dest queue. only the job first in line in the queue (next to the tail) is transferred.
//for this program, it is expected that dest_queue is an empty queue, but generally it doesn't have to be for this function to work
//returns the job number of the job that was transferred. -1 if the origin queue is smaller than 2 in size
int transfer_job(priqueue_t *origin_queue, priqueue_t *dest_queue){
  int origin_size = priqueue_size(origin_queue);
  if(origin_size > 1){
    job_t *origin_job = priqueue_remove_at(origin_queue, origin_size - 2);
    priqueue_offer(dest_queue, origin_job);
    return origin_job->job_number;
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
  free(priqueue_poll(&priqueues[core_id])); //remove the finished job from this core's queue (tail);
  int next_job_number;
  if(priqueue_size(&priqueues[core_id]) == 0){ //there's no next job on this queue (it's empty)
    //look for outstanding jobs in other queues (starting from core 0). if found, grab the first in line and execute it on this core
    for(int core = 0; core < global_state.cores; ++core){
      if(core != core_id){
        next_job_number = transfer_job(&priqueues[core], &priqueues[core_id]); //transfer the first queued job to this queue, which is empty
        if(next_job_number != -1)
          return next_job_number;      
      }
    }
    //if no other filled queues on other cores, idle this core
    return -1; 
  }
  next_job_number = ((job_t*)priqueue_peek(&priqueues[core_id]))->job_number; //get the next job on this queue
	return next_job_number;
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
      printf("%d [%d,%d;%d] ; ", ((job_t*)node->data)->job_number, ((job_t*)node->data)->remaining_time, ((job_t*)node->data)->running_time, ((job_t*)node->data)->priority);
    }
    printf("\n");
  }

}
