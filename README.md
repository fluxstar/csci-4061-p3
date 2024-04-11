Created by Group 31:
- abuka032 (Abdulwahid Abukar)    
    - assisted with opening the local image file in binary mode, calculating its size, establishing a connection to the server, and sending 
      the file through the established network socket.
    - assisted receiving images from the server and ensuring no rescource leaks.
    - debugging in client.c
- krant115 (Logan Krant)
    - Will implement / iterate upon image_match & image buffer handling
    - Figure out how to include file descriptor in LogPrettyPrint 
        (replace 666 placeholder)
- phimp003 (Jimmy Phimpadabsee)
    - Will implement new queue
    - Investigate/fix general synchronization issues (locks, condition
        variables, server/client interaction)
    - help investigate image matching issues

Code primarily tested on csel-kh1260-02.cselabs.umn.edu
    
Changes to existing files: None

"Plan on how you are going to construct the worker threads and how you will make use of mutex locks and condition variables"  
- Instead of creating/destroying threads for each request, we'll create a pool of worker/dispatcher threads to use/reuse when the server is initialized
    - threads chosen by thread system to run to produce/consume request in controled random
    order using locks/CV (block and only produce to queue when empty, only consume when full)
- add defined operations to queue to control dispatcher/worker access/operations and
ensure correct usage
- Implement thread-safe queue using locks and condition variables to prevent race conditions
- Use mutex locks to sync access to the queue allowing only one thread to modify the queue at a time.
- Will use condition variables to sync work between dispatcher and worker threads: one condition variable signals worker threads when a new request is available, and the other signals the dispatcher when there is available space in the queue.


//////////////////NEW//////////////////////
System Design and Thread Management

Transitioned from on-demand thread creation to a persistent thread pool model for both dispatcher and worker threads, initializing this pool at server startup to optimize resource management and processing efficiency.

Thread Pool Usage:Threads can now manange to handle incoming requests, which significantly reducing the overhead of thread creation and destruction.

Queue Management: Implemented a thread-safe queue controlled via mutex locks and condition variables to manage the task flow between dispatcher and worker threads efficiently.

Mutex Locks: Ensured exclusive access to the queue, which prevents concurrent access issues.

Condition Variables: Facilitated synchronization between dispatcher and worker threads; one variable signals the availability of new tasks, while another indicates free space in the queue for incoming requests.
