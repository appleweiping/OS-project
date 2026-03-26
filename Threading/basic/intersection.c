#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

/* 
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Arrival curr_arrivals[4][3][20];

/*
 * semaphores[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t semaphores[4][3];

// Single mutex lock for the basic solution
static pthread_mutex_t basic_mutex = PTHREAD_MUTEX_INITIALIZER;

// Structure to pass arguments to the traffic light threads
typedef struct {
    Side side;
    Direction direction;
} LightArgs;

/*
 * supply_arrivals()
 *
 * A function for supplying arrivals to the intersection
 * This should be executed by a separate thread
 */
static void* supply_arrivals()
{
  int num_curr_arrivals[4][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_arrivals)/sizeof(Arrival); i++)
  {
    // get the next arrival in the list
    Arrival arrival = input_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep_until_arrival(arrival.time);
    // store the new arrival in curr_arrivals
    curr_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return(0);
}


/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void* arg)
{
  // TODO:
  // while it is not END_TIME yet, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)

  LightArgs* my_args = (LightArgs*)arg;
  Side s = my_args->side;
  Direction d = my_args->direction;
  int car_index = 0; 

  // Loop until we hit END_TIME
  while (get_time_passed() < END_TIME) 
  {
    // sem_trywait checks if the semaphore can be decremented without blocking
    // It returns 0 if a car is there, or -1 if the lane is empty
    if (sem_trywait(&semaphores[s][d]) == 0) 
    {
      // Lock the intersection for the basic solution
      pthread_mutex_lock(&basic_mutex);

      // Cross the intersection
      int car_id = curr_arrivals[s][d][car_index].id;
      printf("traffic light %d %d turns green at time %d for car %d\n", s, d, get_time_passed(), car_id);
      
      sleep(CROSS_TIME);
      
      printf("traffic light %d %d turns red at time %d\n", s, d, get_time_passed());

      // Release the intersection
      pthread_mutex_unlock(&basic_mutex);
      
      car_index++; 
    }
    else 
    {
      // If there's no car waiting we sleep for a short time to avoid busy waiting
      usleep(100000); 
    }
  }

  return(0);
}


int main(int argc, char * argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  // start the timer
  start_time();
  
  pthread_t supplier_thread;
  pthread_t light_threads[4][3]; // 12 thread ID's
  LightArgs args[4][3];          // 12 argument packages

  // create a thread per traffic light that executes manage_light
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      // Set the arguments for this traffic light thread
      args[i][j].side = i;
      args[i][j].direction = j;
      
      // Spawn the thread
      pthread_create(&light_threads[i][j], NULL, manage_light, (void*)&args[i][j]);
    }
  }

  // create a thread that executes supply_arrivals
  // We pass NULL for the argument because supply_arrivals doesn't need any parameters
  pthread_create(&supplier_thread, NULL, supply_arrivals, NULL);

  // wait for all threads to finish
  // We loop over all threads and call pthread_join so they have to finish before the main thread can exit
  pthread_join(supplier_thread, NULL);
  
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      pthread_join(light_threads[i][j], NULL);
    }
  }

  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }
}
