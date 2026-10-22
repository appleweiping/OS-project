/* 
 * Operating Systems  (2INCO)  Practical Assignment
 * Interprocess Communication
 */

#ifndef MESSAGES_H
#define MESSAGES_H

typedef struct {
    int request_id; // Unique identifier for the request (from getNextRequest)
    int service_id; // either 1 or 2
    int data;       // The input data or result from the worker process
} MQ_Message;

#endif