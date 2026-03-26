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
#include "request.h"

static void rsleep (int t);


int main (int argc, char * argv[])
{
    // TODO:
    // (see message_queue_test() in interprocess_basic.c)
    //  * open the message queue (whose name is provided in the
    //    arguments)
    //  * repeatingly:
    //      - get the next job request 
    //      - send the request to the Req message queue
    //    until there are no more requests to send
    //  * close the message queue
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <Req_queue_name>\n", argv[0]);
        return 1;
    }

    const char *req_name = argv[1];

    // open request queue for writing
    mqd_t req_q = mq_open(req_name, O_WRONLY);
    if (req_q == (mqd_t)-1) {
        perror("client: mq_open");
        return 1;
    }

    while (true) {
        int jobID, data, serviceID;

        int rc = getNextRequest(&jobID, &data, &serviceID);

        if (rc == NO_REQ)
            break;

        if (rc != NO_ERR) {
            fprintf(stderr, "client: getNextRequest error\n");
            break;
        }

        MQ_Message msg;
        msg.request_id = jobID;
        msg.service_id = serviceID;
        msg.data       = data;

        if (mq_send(req_q, (const char*)&msg, sizeof(MQ_Message), 0) == -1) {
            perror("client: mq_send");
            break;
        }
    }

    mq_close(req_q);
    return 0;
}
