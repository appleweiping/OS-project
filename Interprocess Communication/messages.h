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

#ifndef MESSAGES_H
#define MESSAGES_H

typedef struct {
    int request_id; // Unique identifier for the request (from getNextRequest)
    int service_id; // either 1 or 2
    int data;       // The input data or result from the worker process
} MQ_Message;

#endif