/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * Kaleb de Vries (2154234)
 * Lianhui HUANG (2139936)
 * Weiping Yan (2139367)
 *
 * Grading:
 * Your work will be evaluated based on the following criteria:
 * - Satisfaction of all the specifications
 * - Correctness of the program
 * - Coding style
 * - Report quality
 * - Deadlock analysis
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>      // for perror()
#include <unistd.h>     // for getpid()
#include <mqueue.h>     // for mq-stuff
#include <time.h>       // for time()
#include "messages.h"
#include "service2.h"

static void rsleep (int t);

char* name = "NO_NAME_DEFINED";
mqd_t dealer2worker;
mqd_t worker2dealer;


int main (int argc, char * argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <S2_queue_name> <Rsp_queue_name>\n", argv[0]);
        return 1;
    }

    const char *s2_name  = argv[1];
    const char *rsp_name = argv[2];

    // Open queues: read from S2, write to Rsp
    mqd_t s2_q = mq_open(s2_name, O_RDONLY);
    if (s2_q == (mqd_t)-1) {
        perror("worker_s2: mq_open(S2)");
        return 1;
    }

    mqd_t rsp_q = mq_open(rsp_name, O_WRONLY);
    if (rsp_q == (mqd_t)-1) {
        perror("worker_s2: mq_open(Rsp)");
        mq_close(s2_q);
        return 1;
    }

    MQ_Message msg;

    while (true) {
        // Blocking receive (NO busy waiting)
        ssize_t n = mq_receive(s2_q, (char*)&msg, sizeof(MQ_Message), NULL);
        if (n == -1) {
            if (errno == EINTR) break;
            perror("worker_s2: mq_receive");
            break;
        }

        rsleep(10000);

        // Do the job: service2
        int result = service(msg.data);

        msg.data = result;
        if (mq_send(rsp_q, (const char*)&msg, sizeof(MQ_Message), 0) == -1) {
            if (errno == EINTR) break;
            perror("worker_s2: mq_send");
            break;
        }
    }

    mq_close(s2_q);
    mq_close(rsp_q);
    return 0;
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time
 * between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time (NULL) % getpid ());
        first_call = false;
    }
    usleep (random() % t);
}
