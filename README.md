# OS-project

My team's C solutions to the three practical assignments of the TU/e Operating Systems course (2INC0):
a traffic-intersection simulation with POSIX threads and semaphores, a router/worker system that passes
jobs over POSIX message queues, and an ordered producer/consumer buffer built on condition variables.

The code targets Linux (glibc, `-lpthread`, `-lrt`). On Windows I ran it under WSL. You can also
open the repo in VS Code Dev Containers: `.devcontainer/` uses the
`mcr.microsoft.com/devcontainers/cpp:jammy` image, which ships gcc and make, and all three
assignments build and run in it.

```text
$ cd Threading/advanced && make && ./intersection
gcc -Wall -ggdb2 -o intersection intersection.c intersection_time.c -lpthread
traffic light 0 1 turns green at time 0 for car 0
traffic light 0 1 turns red at time 5
traffic light 3 0 turns green at time 5 for car 1
traffic light 3 0 turns red at time 10
traffic light 2 1 turns green at time 10 for car 2
traffic light 2 0 turns green at time 13 for car 3
traffic light 2 1 turns red at time 15
traffic light 2 0 turns red at time 18

$ cd ../../"Interprocess Communication" && make && ./router_dealer > out.txt && head -4 out.txt
gcc -Wall -ggdb2 -o router_dealer router_dealer.c -lrt -lm
gcc -Wall -ggdb2 -o client client.c request.c -lrt -lm
gcc -Wall -ggdb2 -o worker_s1 worker_s1.c service1.c -lrt -lm
gcc -Wall -ggdb2 -o worker_s2 worker_s2.c service2.c -lrt -lm
5 -> 377
4 -> 233
16 -> 11510
17 -> 1151
```

(Captured on Ubuntu 24.04 under WSL2, gcc 13.3. The intersection run takes 40 s because the
simulation runs in real time until `END_TIME`.)

## The three assignments

### 1. Threading: traffic intersection

Each of the 12 entry lanes (4 sides x left/straight/right) has its own traffic-light thread. A
supplier thread releases cars at the times listed in `input.h` and posts the matching light's
semaphore. In the advanced version the light blocks on it with `sem_timedwait`; the basic version
polls it with `sem_trywait` every 100 ms. A car holds the intersection for `CROSS_TIME` = 5 s.

| Version | File | Locking |
|---------|------|---------|
| Basic | `Threading/basic/intersection.c` | One mutex for the whole intersection, so only one car crosses at a time |
| Advanced | `Threading/advanced/intersection.c` | One mutex per pair of conflicting movements (a 12x12 conflict matrix), always locked in increasing index order to avoid deadlock |

With the same input, the basic version lets car 3 (south, left) go at t = 15; the advanced version
lets it go at t = 13 because it does not conflict with car 2 (south, straight), which is still
crossing.

The basic version reuses `arrivals.h`, `input.h` and `intersection_time.c` from `Threading/advanced`.

### 2. Interprocess communication: router and workers

`router_dealer` creates four POSIX message queues (`mq_open`), forks one client and
`N_SERV1` + `N_SERV2` workers (5 + 5 by default, see `settings.h`), and then routes messages:

```text
client --req--> router_dealer --S1--> worker_s1 x5  (service 1: n-th Fibonacci number)
                              --S2--> worker_s2 x5  (service 2: a Collatz-style iteration)
                router_dealer <--rsp-- workers
```

The client reads its jobs from `request.c` (30 jobs in the current test set). The router prints one
`job -> result` line per finished job, so the order changes from run to run. When all responses are
in, it shuts the workers down and removes the queues.

Don't pipe it straight into `head`: once `head` exits, the router's next `printf` gets SIGPIPE and
kills it before the cleanup, leaving ten blocked workers and four queues in `/dev/mqueue`. Write
the output to a file first (`./router_dealer > out.txt`), as in the example above.

Queue names include the router's PID, so two runs on the same machine do not collide. Note that
`MQ_MAX_MESSAGES` must not exceed `/proc/sys/fs/mqueue/msg_max` (10 on most systems).

### 3. Condition variables: ordered producer/consumer

`Condition Variables/basic/prodcons.c`: 10 producer threads take job numbers 0..1999 in random order
and must put them into a 5-slot circular buffer in ascending order; one consumer prints them. Two
condition variables (`can_produce`, `can_consume`) and one mutex do the synchronisation. The output
is exactly `0` to `1999`, one per line.

## Build and run

Each folder has a Makefile. From the repo root (each line runs in its own subshell, so the block can be
pasted as is):

```bash
(cd "Condition Variables/basic" && make && ./prodcons | tail -3)
(cd "Interprocess Communication" && make && ./router_dealer > out.txt && wc -l out.txt)
(cd Threading/basic && make && ./intersection)
(cd Threading/advanced && make && ./intersection)
```

GitHub Actions builds all four programs on Ubuntu and runs them
([`.github/workflows/ci.yml`](.github/workflows/ci.yml)). It checks that `prodcons` prints
0..1999 in order and that `router_dealer` answers all 30 jobs.

## Which code is ours

The course handed out a skeleton for each assignment: `arrivals.h`, `input.h`,
`intersection_time.c/.h`, `prodcons.h`, `request.c/.h`, `service1.c/.h`, `service2.c/.h` and
`settings.h` come from it, and the `// TODO` comments in our files are the course's instructions.
The synchronisation and queue logic in `intersection.c`, `prodcons.c`, `router_dealer.c`,
`client.c`, `worker_s1.c`, `worker_s2.c` and `messages.h` is the team's work. We were a team of
three students.

## License

MIT for the team's code (see [LICENSE](LICENSE)). The course skeleton files belong to the course.
