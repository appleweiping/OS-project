/* 
 * Operating Systems (2INCO) Practical Assignment
 * Interprocess Communication
 *
 * Contains functions that are used by the clients
 *
 */

#include "request.h"

// Array of requests
//const Request requests[] = { {1, 26, 1}, {2, 5, 2}, {3, 10, 2}, {5, 13, 1}, {4, 3, 1} };
//const Request requests[] = { }; //For Test 1
//const Request requests[] = { {1, 10, 1}, {2, 11, 1}, {3, 12, 1}, {4, 13, 1}, {5, 14, 1}, {6, 15, 1}, {7, 16, 1}, {8, 17, 1}, {9, 18, 1}, {10, 19, 1}, {11, 20, 1}, {12, 21, 1}, {13, 22, 1}, {14, 23, 1}, {15, 24, 1}, {16, 25, 1}, {17, 26, 1}, {18, 27, 1}, {19, 28, 1}, {20, 29, 1} }; //For Test 2 
//const Request requests[] = { {1, 10, 1}, {2, 5, 2}, {3, 12, 1}, {4, 6, 2}, {5, 14, 1}, {6, 7, 2}, {7, 16, 1}, {8, 8, 2}, {9, 18, 1}, {10, 9, 2}, {11, 20, 1}, {12, 10, 2}, {13, 22, 1}, {14, 11, 2}, {15, 24, 1}, {16, 12, 2}, {17, 26, 1}, {18, 13, 2}, {19, 28, 1}, {20, 14, 2} }; //For Test 3
const Request requests[] = {{1, 10, 1}, {2, 11, 1}, {3, 12, 1}, {4, 13, 1}, {5, 14, 1},{6, 15, 1}, {7, 16, 1}, {8, 17, 1}, {9, 18, 1}, {10, 19, 1},{11, 20, 1}, {12, 21, 1}, {13, 22, 1}, {14, 23, 1}, {15, 24, 1},
{16, 5, 2}, {17, 6, 2}, {18, 7, 2}, {19, 8, 2}, {20, 9, 2},{21, 10, 2}, {22, 11, 2}, {23, 12, 2}, {24, 13, 2}, {25, 14, 2},{26, 15, 2}, {27, 16, 2}, {28, 17, 2}, {29, 18, 2}, {30, 19, 2}};//For Test 4
// Places the information of the next request in the parameters sent by reference.
// Returns NO_REQ if there is no request to make.
// Returns NO_ERR otherwise.
int getNextRequest(int* jobID, int* data, int* serviceID) {
	static int i = 0;
	static int N_REQUESTS = sizeof(requests) / sizeof(Request);

	if (i >= N_REQUESTS) 
		return NO_REQ;

	*jobID = requests[i].job;
	*data = requests[i].data;
	*serviceID = requests[i].service;		
	++i;
	return NO_ERR;
		
}
