/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"

int agg_job_wait_time=0;
int agg_job_response_time=0;
int agg_job_completed=0;
int agg_turnaround_time=0;

priqueue_t priqueue;

/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
  int job_number;
  int running_time; //total time for the job
  int remaining_time; //time left until job finishes
  int last_active_time; //the last time we updated remaining time
  int active_time;  //total time the job has been active (running)
  int arrival_time; //time at which the job arrived
  int priority;
  int rr_arrival_time; //time at which a RR job arrived
  int last_queue_time; //the last time when this job was enqueued 
  int wait_time;  //total time that the job has waited in the queue
} job_t;



struct _global_state
{
  int cores;
  int* core_states;
  job_t** active_jobs;
  scheme_t scheme;

} global_state;


//return <= 0 job1 > job 2
int comparer_FCFS(const void* job1, const void* job2){  //insertion when compare(new_val,existing_val) <= 0
  return ((job_t*)job2)->job_number - ((job_t*)job1)->job_number;  //when job1 # is greater than the existing job2 #, insert
}

int comparer_SJF(const void* job1, const void* job2){
  int result = ((job_t*)job2)->running_time - ((job_t*)job1)->running_time;
  if(result == 0)
    return ((job_t*)job2)->arrival_time - ((job_t*)job1)->arrival_time;
  return result;
}
int comparer_PSJF(const void* job1, const void* job2){
  int result = ((job_t*)job2)->remaining_time - ((job_t*)job1)->remaining_time;
  if(result == 0)
    return ((job_t*)job2)->arrival_time - ((job_t*)job1)->arrival_time;
  return result;
}
int comparer_PRI(const void* job1, const void* job2){
  int result = ((job_t*)job2)->priority - ((job_t*)job1)->priority;
  if(result == 0)
    return ((job_t*)job2)->arrival_time - ((job_t*)job1)->arrival_time;
  return result;
}
int comparer_PPRI(const void* job1, const void* job2){
  return comparer_PRI(job1,job2);
}
int comparer_RR(const void* job1, const void* job2){
  return ((job_t*)job2)->arrival_time - ((job_t*)job1)->arrival_time;
}

int (*comparers[6]) (const void*,const void*) = {comparer_FCFS,
                                                comparer_SJF,
                                                comparer_PSJF,
                                                comparer_PRI,
                                                comparer_PPRI,
                                                comparer_RR};



void show_queue2(){
    
      for(node_t *node = priqueue.tail; node != NULL; node = node->prev){
      printf("  %d[%d,%d;%d] ; ", ((job_t*)node->data)->job_number, ((job_t*)node->data)->remaining_time, ((job_t*)node->data)->running_time, ((job_t*)node->data)->priority);
    }
    
}

//called also when an active job is about to terminate
void update_active_job_timings(job_t* active_job, int time){
      active_job->active_time += time - active_job->last_active_time;
      active_job->last_active_time = time;
      active_job->remaining_time = active_job->running_time - active_job->active_time;
}

void update_all_active_jobs_timings(int current_time){
  job_t* active_job;
  for(int core = 0; core < global_state.cores; ++core){
    if((active_job = global_state.active_jobs[core])){//update the job's rem time only if one exists on the core!
      update_active_job_timings(active_job, current_time);
    }
  }
}


//called when the scheme is preemptive. called when a 'superior' job arrives and must preempt the currently running one (push it back in the queue)
//target_core must have a value of -1
void preemptive_offer(job_t *new_job, int *target_core, int current_time){  //this function will be reached only when all cores are active, so the running job can never be null
    job_t* running_job = global_state.active_jobs[0];
    int comparison = 0;
    int maximum_comparison = priqueue.comparer(new_job , running_job); //maximum_comparsion used to find the maximum_comparsion rem time or priority value among the currently active jobs
                                                            //initialised to core 0's comparsion result
    //preempt core 0 if necessary.
    if(maximum_comparison >= 0)
      *target_core = 0;
    //in each core, starting from core 1, look for an "inferior" running job. if found, prempt it.
    for(int core = 1; core < global_state.cores; ++core){
      running_job = global_state.active_jobs[core];
      if((comparison = priqueue.comparer(new_job , running_job)) > 0 && comparison > maximum_comparison){ //we shall premept the running job in this case
        *target_core = core;
        maximum_comparison = comparison;
      }
    }
    if(*target_core == -1){
      priqueue_offer(&priqueue, new_job); //simply enqueue this new job, since no cores had to be preempted
      new_job->last_queue_time = current_time;
    }
    else{
      running_job = global_state.active_jobs[*target_core];
      update_active_job_timings(running_job, current_time);
      priqueue_offer(&priqueue, running_job); //a core was preempted, thus swap its running job with the new one, and enqueue that job
      running_job->last_active_time = current_time;
      running_job->last_queue_time = current_time;
      global_state.active_jobs[*target_core] = new_job;
      printf("\n\n******job %d preempted job %d*******\n\n", new_job->job_number, running_job->job_number);
    }
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
  global_state.core_states = (int*)calloc(cores,sizeof(int));
  global_state.active_jobs = (job_t**)calloc(cores,sizeof(job_t*));
  global_state.scheme = scheme;
  priqueue_init(&priqueue, comparers[scheme]);
}

//returns the core to schedule the new job on. If no cores are idle, it will return -1
int schedulable_core(){
  //first look for idle cores
  for(int core = 0; core < global_state.cores; ++core){
    if(global_state.core_states[core] == 0){
      global_state.core_states[core] = 1; //means that this core is now active
      return core;
    }
  }
  return -1;
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
  job->rr_arrival_time=time;
  job->running_time = running_time;
  job->remaining_time = running_time;
  job->active_time = 0;
  job->arrival_time = time;
  job->priority = priority; 
  job->wait_time = 0;
  job->last_active_time = time;

  int target_core = schedulable_core();
  int currently_schedulable = target_core != -1;

  //update the rem time of the active jobs
  update_all_active_jobs_timings(time);
  
  //if not currenlty schedulabe, the job must be added to the queue
  if(!currently_schedulable){
    job->last_queue_time = time;
    switch(global_state.scheme){
      case FCFS:
      case SJF:
      case PRI:
      case RR:
        priqueue_offer(&priqueue, job); //push to the queue. the comparator function should take care of the location within the queue.
        break;
      case PSJF:
      case PPRI:
        preemptive_offer(job, &target_core, time);
        break;
      default:{}
    }
  }
  else{
    job->last_active_time = time;
    global_state.active_jobs[target_core] = job;
  }


  //target_core will be -1 if this new job was pushed to the queue. otherwise it will be the core number on which this job will begin execution right away.
  return target_core;
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
  agg_job_completed++;
  job_t* finished_job = global_state.active_jobs[core_id];
  agg_job_response_time += (time - finished_job->arrival_time);
  agg_turnaround_time += (time - finished_job->rr_arrival_time);
  agg_job_wait_time += finished_job->wait_time;
  printf("\nBefore Poll: "); show_queue2();
  job_t* next_job = priqueue_poll_head(&priqueue);
  printf("\nAfter Poll: "); show_queue2(); printf("\n");
  update_active_job_timings(global_state.active_jobs[core_id], time);
  free(global_state.active_jobs[core_id]); //free the currently active job
  global_state.active_jobs[core_id] = NULL;
  if(next_job){
    next_job->wait_time += time - next_job->last_queue_time; //!=-1 means that it has been queued at least once
    global_state.active_jobs[core_id] = next_job;
    global_state.active_jobs[core_id]->last_active_time = time;
    return next_job->job_number;
  }
  global_state.core_states[core_id] = 0;
  return -1;
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
  job_t* curr_job = global_state.active_jobs[core_id];
  curr_job->arrival_time=time;
  priqueue_offer(&priqueue, curr_job);
  curr_job = priqueue_poll_head(&priqueue);
          //global_state.active_jobs[core_id]= curr_job;
  if(!curr_job)
  {
      return -1;
  }
  else
  {
      global_state.active_jobs[core_id]=curr_job;
      return global_state.active_jobs[core_id]->job_number;
  }
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return ((float)agg_job_wait_time/(float)agg_job_completed);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return ((float)agg_turnaround_time/(float)agg_job_completed);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return ((float)agg_job_response_time/(float)agg_job_completed);
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  
  free(global_state.core_states);
  
  job_t* job;
  for(int core = 0; core < global_state.cores; ++core){
    if((job = global_state.active_jobs[core]))
        free(job);
  }

  free(global_state.active_jobs);

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
    
    for(node_t *node = priqueue.tail; node != NULL; node = node->prev){
      printf("  %d[%d,%d;%d] ; ", ((job_t*)node->data)->job_number, ((job_t*)node->data)->remaining_time, ((job_t*)node->data)->running_time, ((job_t*)node->data)->priority);
    }
    printf("\n\n  ---------------\n  Core States\n");
    for(int core = 0; core < global_state.cores; ++core)
      printf("  %d;", global_state.core_states[core]);
    printf("\n\n  ---------------\n  Active Jobs\n");
    job_t* j;
    for(int core = 0; core < global_state.cores; ++core){
      if((j = global_state.active_jobs[core]))
        printf("  %d[%d,%d;%d] ;", j->job_number, j->remaining_time, j->running_time, j->priority);
      else
        printf("  %d[%d,%d;%d] ;", -1,-1,-1,-1);
    }
    printf("\n  ---------------");
    
    
}
