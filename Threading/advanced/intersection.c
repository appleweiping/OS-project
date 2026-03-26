#define _GNU_SOURCE
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

#define NUM_SIDES 4
#define NUM_DIRECTIONS 3
#define NUM_MOVEMENTS 12
#define MAX_ARRIVALS_PER_LANE 20

/*
 * curr_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred.
 * The first two indices determine the entry lane:
 * first index is Side, second index is Direction.
 */
static Arrival curr_arrivals[NUM_SIDES][NUM_DIRECTIONS][MAX_ARRIVALS_PER_LANE];

/*
 * semaphores[][]
 *
 * One semaphore per entry lane.
 */
static sem_t semaphores[NUM_SIDES][NUM_DIRECTIONS];

/*
 * pair_mutexes[a][b]
 *
 * For every pair of conflicting movements (a,b) with a < b,
 * we create exactly one mutex.
 *
 * Two movements only block each other if they directly conflict.
 * This avoids the over-synchronization problem from the previous version.
 */
static pthread_mutex_t pair_mutexes[NUM_MOVEMENTS][NUM_MOVEMENTS];

/* Only used to prevent interleaved printf output */
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
  Side side;
  Direction direction;
} LightArgs;

/*
 * Movement index mapping:
 *   0  = NORTH LEFT
 *   1  = NORTH STRAIGHT
 *   2  = NORTH RIGHT
 *   3  = EAST  LEFT
 *   4  = EAST  STRAIGHT
 *   5  = EAST  RIGHT
 *   6  = SOUTH LEFT
 *   7  = SOUTH STRAIGHT
 *   8  = SOUTH RIGHT
 *   9  = WEST  LEFT
 *   10 = WEST  STRAIGHT
 *   11 = WEST  RIGHT
 *
 * conflict_matrix[a][b] == 1 means movements a and b may not
 * be in the intersection at the same time.
 *
 * Important:
 * diagonal entries are 0 because a traffic light thread already
 * processes its own lane sequentially, so it does not need to
 * block itself with an extra mutex.
 */
static const int conflict_matrix[NUM_MOVEMENTS][NUM_MOVEMENTS] =
{
    /* NL NS NR  EL ES ER  SL SS SR  WL WS WR */
    { 0, 0, 0,  1, 1, 0,  0, 1, 1,  1, 1, 0 }, /* NL (0)  */
    { 0, 0, 0,  1, 1, 0,  1, 0, 0,  1, 1, 1 }, /* NS (1)  */
    { 0, 0, 0,  0, 1, 0,  1, 0, 0,  0, 0, 0 }, /* NR (2)  */

    { 1, 1, 0,  0, 0, 0,  1, 1, 0,  0, 1, 1 }, /* EL (3)  */
    { 1, 1, 1,  0, 0, 0,  1, 1, 0,  1, 0, 0 }, /* ES (4)  */
    { 0, 0, 0,  0, 0, 0,  0, 1, 0,  1, 0, 0 }, /* ER (5)  */

    { 0, 1, 1,  1, 1, 0,  0, 0, 0,  1, 1, 0 }, /* SL (6)  */
    { 1, 0, 0,  1, 1, 1,  0, 0, 0,  1, 1, 0 }, /* SS (7)  */
    { 1, 0, 0,  0, 0, 0,  0, 0, 0,  0, 1, 0 }, /* SR (8)  */

    { 1, 1, 0,  0, 1, 1,  1, 1, 0,  0, 0, 0 }, /* WL (9)  */
    { 1, 1, 0,  1, 0, 0,  1, 1, 1,  0, 0, 0 }, /* WS (10) */
    { 0, 1, 0,  1, 0, 0,  0, 0, 0,  0, 0, 0 }  /* WR (11) */
};

static int movement_index(Side side, Direction direction)
{
  return ((int)side) * NUM_DIRECTIONS + (int)direction;
}

/*
 * Locks all pairwise conflict mutexes needed by this movement.
 *
 * We always lock in increasing order of "other movement" index.
 * Therefore, every thread uses a global lock order and deadlock is avoided.
 */
static void lock_required_mutexes(int movement)
{
  for (int other = 0; other < NUM_MOVEMENTS; other++)
  {
    if (other == movement)
    {
      continue;
    }

    if (conflict_matrix[movement][other])
    {
      int a = movement < other ? movement : other;
      int b = movement < other ? other : movement;
      pthread_mutex_lock(&pair_mutexes[a][b]);
    }
  }
}

static void unlock_required_mutexes(int movement)
{
  for (int other = NUM_MOVEMENTS - 1; other >= 0; other--)
  {
    if (other == movement)
    {
      continue;
    }

    if (conflict_matrix[movement][other])
    {
      int a = movement < other ? movement : other;
      int b = movement < other ? other : movement;
      pthread_mutex_unlock(&pair_mutexes[a][b]);
    }
  }
}

/*
 * Wait for a car to arrive on a semaphore until END_TIME.
 * Returns 1 if a car arrived, 0 if the traffic light should terminate.
 */
static int wait_for_arrival_until_end(sem_t *sem)
{
  while (1)
  {
    int now = get_time_passed();
    if (now >= END_TIME)
    {
      return 0;
    }

    int remaining = END_TIME - now;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += remaining;

    if (sem_timedwait(sem, &ts) == 0)
    {
      return 1;
    }

    if (errno == ETIMEDOUT)
    {
      return 0;
    }

    if (errno != EINTR)
    {
      return 0;
    }
  }
}

/*
 * supply_arrivals()
 *
 * Supplies arrivals to the intersection.
 * This should be executed by a separate thread.
 */
static void* supply_arrivals()
{
  int num_curr_arrivals[NUM_SIDES][NUM_DIRECTIONS] =
  {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0}
  };

  for (int i = 0; i < (int)(sizeof(input_arrivals) / sizeof(Arrival)); i++)
  {
    Arrival arrival = input_arrivals[i];

    sleep_until_arrival(arrival.time);

    curr_arrivals[arrival.side][arrival.direction]
                 [num_curr_arrivals[arrival.side][arrival.direction]] = arrival;

    num_curr_arrivals[arrival.side][arrival.direction] += 1;

    sem_post(&semaphores[arrival.side][arrival.direction]);
  }

  return NULL;
}

/*
 * manage_light()
 *
 * Behaviour of one traffic light.
 */
static void* manage_light(void* arg)
{
  LightArgs* my_args = (LightArgs*)arg;
  Side side = my_args->side;
  Direction direction = my_args->direction;

  int lane_car_index = 0;
  int my_movement = movement_index(side, direction);

  while (1)
  {
    int has_arrival = wait_for_arrival_until_end(&semaphores[side][direction]);
    if (!has_arrival)
    {
      break;
    }

    lock_required_mutexes(my_movement);

    Arrival current = curr_arrivals[side][direction][lane_car_index];
    lane_car_index++;

    pthread_mutex_lock(&print_mutex);
    printf("traffic light %d %d turns green at time %d for car %d\n",
           side, direction, get_time_passed(), current.id);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);

    sleep(CROSS_TIME);

    pthread_mutex_lock(&print_mutex);
    printf("traffic light %d %d turns red at time %d\n",
           side, direction, get_time_passed());
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);

    unlock_required_mutexes(my_movement);
  }

  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t supplier_thread;
  pthread_t light_threads[NUM_SIDES][NUM_DIRECTIONS];
  LightArgs args[NUM_SIDES][NUM_DIRECTIONS];

  for (int i = 0; i < NUM_SIDES; i++)
  {
    for (int j = 0; j < NUM_DIRECTIONS; j++)
    {
      sem_init(&semaphores[i][j], 0, 0);
    }
  }

  for (int i = 0; i < NUM_MOVEMENTS; i++)
  {
    for (int j = 0; j < NUM_MOVEMENTS; j++)
    {
      pthread_mutex_init(&pair_mutexes[i][j], NULL);
    }
  }

  start_time();

  for (int i = 0; i < NUM_SIDES; i++)
  {
    for (int j = 0; j < NUM_DIRECTIONS; j++)
    {
      args[i][j].side = (Side)i;
      args[i][j].direction = (Direction)j;
      pthread_create(&light_threads[i][j], NULL, manage_light, &args[i][j]);
    }
  }

  pthread_create(&supplier_thread, NULL, supply_arrivals, NULL);

  pthread_join(supplier_thread, NULL);

  for (int i = 0; i < NUM_SIDES; i++)
  {
    for (int j = 0; j < NUM_DIRECTIONS; j++)
    {
      pthread_join(light_threads[i][j], NULL);
    }
  }

  for (int i = 0; i < NUM_SIDES; i++)
  {
    for (int j = 0; j < NUM_DIRECTIONS; j++)
    {
      sem_destroy(&semaphores[i][j]);
    }
  }

  for (int i = 0; i < NUM_MOVEMENTS; i++)
  {
    for (int j = 0; j < NUM_MOVEMENTS; j++)
    {
      pthread_mutex_destroy(&pair_mutexes[i][j]);
    }
  }

  pthread_mutex_destroy(&print_mutex);

  return 0;
}