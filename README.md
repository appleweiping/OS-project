# Operating Systems — Concurrency and IPC

Three practical assignments covering POSIX threading, interprocess communication with ZeroMQ, and condition variable synchronization.

---

## Assignments

### 1. Threading — Intersection Simulation

Simulates vehicles crossing a shared intersection using POSIX threads and mutexes.

| Variant | File | Description |
|---------|------|-------------|
| Basic | `Threading/basic/intersection.c` | Mutex-based mutual exclusion |
| Advanced | `Threading/advanced/intersection.c` | Timed arrivals, priority scheduling |
| Advanced (timed) | `Threading/advanced/intersection_time.c` | Deadline-aware variant |

Build:
```bash
cd Threading/advanced && make
```

---

### 2. Interprocess Communication — Request Router

Client-router-worker pipeline using ZeroMQ (DEALER/ROUTER pattern). Two worker types handle different service requests; the router dispatches based on request type.

| File | Role |
|------|------|
| `client.c` | Generates requests and sends to router |
| `router_dealer.c` | Routes requests to appropriate worker pool |
| `worker_s1.c` | Handles Service 1 requests |
| `worker_s2.c` | Handles Service 2 requests |
| `service1.c/h` | Service 1 logic |
| `service2.c/h` | Service 2 logic |
| `messages.h` | Shared message format |
| `settings.h` | Configurable parameters |

Build:
```bash
cd "Interprocess Communication" && bash cc.sh
```

Requires ZeroMQ (`libzmq`).

---

### 3. Condition Variables

Producer-consumer and synchronization patterns using POSIX condition variables.

| Variant | Directory |
|---------|-----------|
| Basic | `Condition Variables/basic/` |
| Advanced | `Condition Variables/advanced/` |

---

## Prerequisites

- GCC
- POSIX threads (`-lpthread`)
- ZeroMQ (`libzmq`) for the IPC assignment

---

## License

MIT
