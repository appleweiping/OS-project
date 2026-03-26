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
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>    
#include <unistd.h>    // for execlp
#include <mqueue.h>    // for mq

#include <signal.h>    // We use this to terminate the worker processes


#include "settings.h"  
#include "messages.h"

char client2dealer_name[30];
char dealer2worker1_name[30];
char dealer2worker2_name[30];
char worker2dealer_name[30];

int main (int argc, char * argv[])
{
    if (argc != 1)
    {
      fprintf (stderr, "%s: invalid arguments\n", argv[0]);
    }

    pid_t client_pid;
    pid_t worker1_pids[N_SERV1];
    pid_t worker2_pids[N_SERV2];

    pid_t my_pid = getpid();
    // 77 is our group number, and we append the process ID to ensure uniqueness of the queue names
    sprintf(client2dealer_name, "/client2dealer_77_%d", my_pid);
    sprintf(dealer2worker1_name, "/dealer2worker1_77_%d", my_pid);
    sprintf(dealer2worker2_name, "/dealer2worker2_77_%d", my_pid);
    sprintf(worker2dealer_name, "/worker2dealer_77_%d", my_pid);
    
    // Configure the queue attributes
    struct mq_attr attr;
    attr.mq_flags = 0; 
    attr.mq_maxmsg = MQ_MAX_MESSAGES; // Maximum number of messages in the queue (defined in settings.h)
    attr.mq_msgsize = sizeof(MQ_Message); // Size of each message (defined in messages.h)
    attr.mq_curmsgs = 0; // Number of messages currently in the queue

    // Create the message queues
    mqd_t req_q = mq_open(client2dealer_name, O_CREAT | O_RDWR, 0600, &attr); // 0600 grants permission to read/write for the owner only
    mqd_t s1_q = mq_open(dealer2worker1_name, O_CREAT | O_RDWR, 0600, &attr);
    mqd_t s2_q = mq_open(dealer2worker2_name, O_CREAT | O_RDWR, 0600, &attr);
    mqd_t rsp_q = mq_open(worker2dealer_name, O_CREAT | O_RDWR, 0600, &attr);

    // Check if the queues were created successfully
    if (req_q == (mqd_t) -1 || s1_q == (mqd_t) -1 || s2_q == (mqd_t) -1 || rsp_q == (mqd_t) -1) {
        perror("Failed to create message queues");
        exit(1);
    }

    // Create the client process
    client_pid = fork();
    if (client_pid == 0) {
        // This executes in the child process (client)
        execlp("./client", "client", client2dealer_name, NULL);
        // If execlp returns, it means there was an error
        perror("Failed to start client process");
        exit(1);
    }

    // Create worker processes for service 1
    for (int i = 0; i < N_SERV1; i++) {
        worker1_pids[i] = fork();
        if (worker1_pids[i] == 0) {
          // The worker proccesses need the rsp queue and their respective Sx queue
            execlp("./worker_s1", "worker_s1", dealer2worker1_name, worker2dealer_name, NULL);
            perror("Failed to start worker_s1 process");
            exit(1);
        }
    }

    // Create worker processes for service 2
    for (int i = 0; i < N_SERV2; i++) {
        worker2_pids[i] = fork();
        if (worker2_pids[i] == 0) {
            // The worker proccesses need the rsp queue and their respective Sx queue
            execlp("./worker_s2", "worker_s2", dealer2worker2_name, worker2dealer_name, NULL);
            perror("Failed to start worker_s2 process");
            exit(1);
        }
    }

    // Wait for the client process to finish
    bool client_alive = true;
    int pending_jobs = 0;
    struct mq_attr target_attr; // Used to check the specific worker queue
    MQ_Message msg;
    int status;

    bool has_buffered_req = false;
    MQ_Message buffered_req;

    while (client_alive || pending_jobs > 0) {
        
        // Check if the Client is still alive
        if (client_alive) {
            pid_t result = waitpid(client_pid, &status, WNOHANG);
            if (result == client_pid || result == -1) {
                client_alive = false; // Client has finished, 
                // but still need to process pending jobs so we don't exit the loop yet
            }
        }

        // We first drain the reponse queue to keep the rsp queue as empty as possible, 
        // which allows workers to send responses without blocking, fixing part of the deadlock issue
        mq_getattr(rsp_q, &attr);
        while (attr.mq_curmsgs > 0) {
            if (mq_receive(rsp_q, (char*)&msg, sizeof(MQ_Message), NULL) != -1) {
                printf("%d -> %d\n", msg.request_id, msg.data); // Print the result
                fflush(stdout);
                pending_jobs--;
            }
            mq_getattr(rsp_q, &attr); // Check if another response arrived while printing
        }

        // If we have a buffered request we try to send it before reading any new requests
        if (has_buffered_req) {
            mqd_t target_q = (buffered_req.service_id == 1) ? s1_q : s2_q;
            
            mq_getattr(target_q, &target_attr);
            // Check if the worker queue is not full
            if (target_attr.mq_curmsgs < target_attr.mq_maxmsg) {
                mq_send(target_q, (const char*)&buffered_req, sizeof(MQ_Message), 0);
                has_buffered_req = false; // Buffer is empty, so we can read from client again
            }
        }

        // We only read requests if we don't have a buffered request,
        // otherwise we might end up with multiple buffered requests and the Router would never go to sleep
        // which is the main cause of the deadlock issue
        if (!has_buffered_req) {
            mq_getattr(req_q, &attr);
            if (attr.mq_curmsgs > 0) {
                if (mq_receive(req_q, (char*)&msg, sizeof(MQ_Message), NULL) != -1) {
                    pending_jobs++;
                    
                    mqd_t target_q = (msg.service_id == 1) ? s1_q : s2_q;
                    mq_getattr(target_q, &target_attr);
                    
                    // Check if the worker queue is not full before sending the request
                    if (target_attr.mq_curmsgs < target_attr.mq_maxmsg) {
                        // Safe to send immediately without blocking
                        mq_send(target_q, (const char*)&msg, sizeof(MQ_Message), 0);
                    } else {
                        // Queue is full so we add it to the buffer and try to send it in the next iteration
                        buffered_req = msg;
                        has_buffered_req = true;
                    }
                }
            }
        }
    }

    // Terminate all worker processes
    for (int i = 0; i < N_SERV1; i++) {
        kill(worker1_pids[i], SIGTERM); // Send signal to terminate
        waitpid(worker1_pids[i], NULL, 0); // Wait for worker to actually terminate
    }
    
    for (int i = 0; i < N_SERV2; i++) {
        kill(worker2_pids[i], SIGTERM);
        waitpid(worker2_pids[i], NULL, 0);
    }

    // Clean up message queues before exiting
    mq_close(req_q); // Disconnect queues from processes
    mq_close(s1_q);
    mq_close(s2_q);
    mq_close(rsp_q);
    mq_unlink(client2dealer_name); // Delete the queues
    mq_unlink(dealer2worker1_name);
    mq_unlink(dealer2worker2_name);
    mq_unlink(worker2dealer_name);
  
  return (0);
}
