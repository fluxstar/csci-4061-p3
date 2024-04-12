Created by Group 31:
- abuka032 (Abdulwahid Abukar)    
    - assisted with opening the local image file in binary mode, calculating its size, establishing a connection to the server, and sending 
      the file through the established network socket.
    - assisted receiving images from the server and ensuring no rescource leaks.
    - debugging in client.c

- krant115 (Logan Krant)
    - Implemented intermediate submission for server.c using a circular fixed array as the data structure
    - Extensive debugging for image buffer corruption when passed into image_match (strncpy vs memcpy was the root cause)
    - Assisted with the final implementation of request_handle in client.c based on drafts
    - Created the first drafts of LogPrettyPrint, worker/dispatcher, database initialization, directory-trav, and request_handle

- phimp003 (Jimmy Phimpadabsee)
    - Implemented queue and database structs and functions
    - Implemented prettyLog
    - Helped debug image match not working
    - wrote second draft of server/client, taking the best of the 2 drafts

Code primarily tested on csel-kh1260-02.cselabs.umn.edu
    
Changes to existing files: None

How could you enable your program to make EACH individual request parallelized? (high-level
pseudocode would be acceptable/preferred for this part)

We can enable our program to parallelize requests by maintaining a pool of threads which constantly run according to the thread system on all the CPU's cores and pick up available requests when they run via a queue; ensuring request are fulfilled and that more CPU time is used fulfilling requests. We let other threads know when a thread has produced or added content in the queue and signal them to run time soon if they are waiting for a space or a request to take; encouraging the thread system to run threads with actual work to do across the CPU cores.

lock l;
request_queue q;

dispatcher():
    while(1):
        get client request
        lock(l)
        while (queue_full(q)):
            wait for queue to have free space
        enqueue(q, client_request)
        let any waiting threads know there's a request in queue
        unlock(l);

worker():
    while(1):
        lock(l)
        while (queue_empty(q)):
            wait for queue to have request(s)
        request = dequeue(q)
        let any waiting threads know there's a free slot in queue
        unlock(l)
        do work on request

main():
    create n dispatcher threads
    create m worker threads
    join threads when done


System Design and Thread Management

Transitioned from on-demand thread creation to a persistent thread pool model for both dispatcher and worker threads, initializing this pool at server startup to optimize resource management and processing efficiency.

Thread Pool Usage: Threads can now manange to handle incoming requests, which significantly reducing the overhead of thread creation and destruction.

Queue Management: Implemented a thread-safe queue controlled via mutex locks and condition variables to manage the task flow between dispatcher and worker threads efficiently.

Mutex Locks: Ensured exclusive access to the queue, which prevents concurrent access issues.

Condition Variables: Facilitated synchronization between dispatcher and worker threads; one variable signals the availability of new tasks, while another indicates free space in the queue for incoming requests.
