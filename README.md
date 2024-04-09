Created by Group 31:
- abuka032 (Abdulwahid Abukar)
    - Will implement/assist image matching logic
    - help enhance synchronization 
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