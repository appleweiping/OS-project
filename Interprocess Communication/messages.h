/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 *
 * [teammate] ([student number removed])
 * [teammate] ([student number removed])
 * Weiping Yan ([student number removed])
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