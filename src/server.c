#include "../include/server.h"

// /********************* [ Helpful Global Variables ] **********************/
int num_dispatcher =
    0;  // Global integer to indicate the number of dispatcher threads
int num_worker = 0;  // Global integer to indicate the number of worker threads
FILE *logfile;       // Global file pointer to the log file
int queue_len = 0;   // Global integer to indicate the length of the queue
// keeps track of fd of client connection from accept_connection a
// worker is fulfilling a request for. indexed by worker id
int *working_connection_fd = NULL;
#define MAX_FN_SIZE 256

/********************* [ Database Code ] **********************/
#define DEFAULT_DB_SIZE 100

// Holds relevant database info
typedef struct {
    database_entry_t *entries;
    size_t size;
    size_t capacity;
} database_t;

// Single maintained database
static database_t database;

// Creates an empty database. Will write to stderr if fails to allocate space
// for entries
void init_database() {
    database.entries = malloc(sizeof(database_entry_t) * DEFAULT_DB_SIZE);
    database.size = 0;
    database.capacity = DEFAULT_DB_SIZE;

    if (database.entries == NULL) {
        fprintf(stderr, "failed to init database");
        return;
    }
}

/**********************************************
 * add_database_entry
   - parameters:
      - fn is the name of the image
      - size is the size of the image data
      - buffer is the contents of the image (its bytes)
   - returns:
      - 0 on success, -1 on failure to add an entry
************************************************/
int add_database_entry(char *fn, int size, char *buffer) {
    if (database.size >= database.capacity) {
        fprintf(stderr,
                "attempted to write more entries than capacity for (%s)\n", fn);
        return -1;
    }
    int idx = database.size;

    database.entries[idx].file_name = malloc(sizeof(char) * BUFFER_SIZE);
    if (database.entries[idx].file_name == NULL) return -1;
    strncpy(database.entries[idx].file_name, fn, BUFFER_SIZE);

    database.entries[idx].file_size = size;

    database.entries[idx].buffer = malloc(sizeof(char) * BUFFER_SIZE);
    if (database.entries[idx].buffer == NULL) return -1;
    memcpy(database.entries[idx].buffer, buffer, BUFFER_SIZE);

    ++database.size;
    return 0;
}

// clear database to be empty
void clear_database() {
    for (int i = 0; i < database.size; ++i) {
        free(database.entries[i].file_name);
        free(database.entries[i].buffer);
    }
    free(database.entries);
    database.capacity = DEFAULT_DB_SIZE;
    database.size = 0;
}

/********************* [ Queue Code ] **********************/
// node for the queue
typedef struct queue_node {
    request_t request;
    struct queue_node *next;
} queue_node_t;

// hold relevant info on queue state
typedef struct {
    queue_node_t *head;
    queue_node_t *tail;
    size_t size;
} queue_t;

// Only maintain single queue
static queue_t queue;

// create and return an empty queue
void init_queue() {
    queue.head = NULL;
    queue.tail = NULL;
    queue.size = 0;
}

// return 1 if empty, else 0
#define QUEUE_EMPTY(queue) (queue.size == 0)

// returns 1 ifr full, else 0
#define QUEUE_FULL(queue) (queue.size == MAX_QUEUE_LEN)

/**********************************************
 * enqueue
   - parameters:
      - size is the size of buffer passed in
      - fd is the file descriptor/socket of the client whose
        request this belongs to
      - buffer holds the contents (bytes) of the requested image
   - returns:
      - 0 on success, 1 if queue full, -1 on failure
************************************************/
int enqueue(int size, int fd, char *buffer) {
    if (QUEUE_FULL(queue)) return 1;

    // create queue node, deep copy buffer
    queue_node_t *node = malloc(sizeof(queue_node_t));
    if (node == NULL) return -1;
    node->request.file_size = size;
    node->request.file_descriptor = fd;
    node->request.buffer = malloc(sizeof(char) * BUFFER_SIZE);
    if (node->request.buffer == NULL) return -1;
    memcpy(node->request.buffer, buffer, BUFFER_SIZE);

    // shouldn't happen with propper synchronization
    if (QUEUE_EMPTY(queue)) {
        queue.head = node;
        queue.tail = queue.head;
        ++queue.size;
        return 0;
    }

    queue.tail->next = node;
    queue.tail = node;
    ++queue.size;
    return 0;
}

/**********************************************
 * dequeue
   - parameters:
      - none
   - returns:
       - request_t at top of queue, else request with file_size/file_descriptor
         of -1 (INVALID) and buffer NULL if queue empty
************************************************/
request_t dequeue() {
    request_t request;

    // Shouldn't happen with propper synchronization
    if (QUEUE_EMPTY(queue)) {
        request.file_size = INVALID;
        request.file_descriptor = INVALID;
        request.buffer = NULL;
        return request;
    }

    // remove node from queue and copy request to non ptr
    queue_node_t *node = queue.head;
    request.file_size = node->request.file_size;
    request.file_descriptor = node->request.file_descriptor;
    request.buffer = node->request.buffer;

    queue.head = node->next;
    free(node);
    --queue.size;

    if (QUEUE_EMPTY(queue)) {  // queue now empty
        queue.tail = NULL;
    }

    return request;
}

// remove all elements from a queue
void clear_queue() {
    queue_node_t *curr_node = queue.head;
    queue_node_t *next_node = NULL;

    while (curr_node != NULL) {
        next_node = curr_node->next;
        free(curr_node->request.buffer);
        free(curr_node);
        curr_node = next_node;
    }
    queue.size = 0;
}

/********************* [ Locks and Condition Variables ] **********************/
static pthread_mutex_t queue_access = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t log_file_access = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_has_content = PTHREAD_COND_INITIALIZER;
static pthread_cond_t queue_slot_free = PTHREAD_COND_INITIALIZER;

/* TODO: Intermediate Submission
  TODO: Add any global variables that you may need to track the requests and
  threads [multiple funct]  --> How will you track the p_thread's that you
  create for workers? [multiple funct]  --> How will you track the p_thread's
  that you create for dispatchers? [multiple funct]  --> Might be helpful to
  track the ID's of your threads in a global array What kind of locks will you
  need to make everything thread safe? [Hint you need multiple] What kind of CVs
  will you need  (i.e. queue full, queue empty) [Hint you need multiple] How
  will you track the number of images in the database? How will you track the
  requests globally between threads? How will you ensure this is thread safe?
  Example: request_t req_entries[MAX_QUEUE_LEN]; [multiple funct]  --> How will
  you update and utilize the current number of requests in the request queue?
  [worker()]        --> How will you track which index in the request queue to
  remove next? [dispatcher()]    --> How will you know where to insert the next
  request received into the request queue? [multiple funct]  --> How will you
  track the p_thread's that you create for workers? TODO How will you store the
  database of images? What data structure will you use? Example:
  database_entry_t database[100];
*/

// TODO: Implement this function
/**********************************************
 * image_match
   - parameters:
      - input_image is the image data to compare
      - size is the size of the image data
   - returns:
       - database_entry_t that is the closest match to the input_image
************************************************/
// just uncomment out when you are ready to implement this function
database_entry_t image_match(char *input_image, int size) {
    int closest_distance = INT_MAX;
    int closest_index = 0;
    int closest_file_size = INT_MAX;  // ADD THIS VARIABLE

    // get the closest matching image from the database to the provided image
    for (int i = 0; i < database.size; i++) {
        const char *current_file = database.entries[i].buffer;
        int result = memcmp(input_image, current_file, size);

        if (result == 0) {
            return database.entries[i];
        }

        if (result < closest_distance ||
            (result == closest_distance &&
             database.entries[i].file_size < closest_file_size)) {
            closest_distance = result;
            closest_file = current_file;
            closest_index = i;
            closest_file_size = database.entries[i].file_size;
        }
    }

    return database.entries[closest_index];
}

// TODO: Implement this function
/**********************************************
 * LogPrettyPrint
   - must update working_connection_fd[threadID] to fd thread/worker is
     working on before calling
   - parameters:
      - to_write is expected to be an open file pointer, or it
        can be NULL which means that the output is printed to the terminal
      - All other inputs are self explanatory or specified in the writeup
   - returns:
       - no return value
************************************************/
void LogPrettyPrint(FILE *to_write, int threadId, int requestNumber,
                    char *file_name, int file_size) {
    // get file descriptor a given thread is working on
    int fd = working_connection_fd[threadId];  // MUST BE UPDATED BEFORE CALLING
                                               // PRETTY PRINT
    if (fd < 0) {
        fprintf(stderr, "bad file ptr provided. Not open or does not exist\n");
        return;
    }

    if (to_write == NULL) {
        printf("[%d][%d][%d][%s][%d]\n", threadId, requestNumber, fd, file_name,
               file_size);
        return;
    }

    pthread_mutex_lock(&log_file_access);
    fprintf(to_write, "[%d][%d][%d][%s][%d]\n", threadId, requestNumber, fd,
            file_name, file_size);
    fflush(to_write);  // Flush the file
    pthread_mutex_unlock(&log_file_access);
}

/*
  TODO: Implement this function for Intermediate Submission
  * loadDatabase
    - parameters:
        - path is the path to the directory containing the images
    - returns:
        - no return value
    - Description:
        - Traverse the directory and load all the images into the database
          - Load the images from the directory into the database
          - You will need to read the images into memory
          - You will need to store the image data in the database_entry_t struct
          - You will need to store the file name in the database_entry_t struct
          - You will need to store the file size in the database_entry_t struct
          - You will need to store the image data in the database_entry_t struct
          - You will need to increment the number of images in the database
*/
/***********/
void loadDatabase(char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    char *buffer;

    dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "failed to open directory: %s\n", path);
        return;
    }

    // read name, size, and contents of each image in given directory into
    // database
    while ((entry = readdir(dir)) != NULL) {
        char fn[BUFF_SIZE + MAX_FN_SIZE]; // path + fn size
        snprintf(fn, BUFF_SIZE + MAX_FN_SIZE, "%s/%s", path, entry->d_name);

        if (stat(fn, &info) != 0) {
            fprintf(stderr, "could not get stats for file: %s\n", fn);
            continue;
        }
        if (!S_ISREG(info.st_mode)) continue;

        FILE *fp = fopen(fn, "rb");
        if (fp == NULL) {
            fprintf(stderr, "could not open file: %s\n", fn);
            continue;
        }

        buffer = malloc(sizeof(char) * info.st_size);
        if (buffer == NULL) {
            fprintf(stderr, "could not allocate buffer for file contents\n");
            continue;
        }

        if (fread(buffer, info.st_size, 1, fp) != 1) {
            fprintf(stderr, "error reading from file: %s\n", fn);
            continue;
        }

        if (add_database_entry(fn, info.st_size, buffer) == -1) {
            fprintf(stderr, "could not add database entry for file: %s\n", fn);
        }
        free(buffer);
        fclose(fp);
    }
    closedir(dir);
}

void *dispatch(void *arg) {
    while (1) {
        size_t file_size = 0;
        // request_detials_t request_details;

        /* TODO: Intermediate Submission
         *    Description:      Accept client connection
         *    Utility Function: int accept_connection(void)
         */
        int fd = accept_connection();
        if (fd < 0) {
            fprintf(stderr, "client connection not accepted\n");
            continue;
        }

        /* TODO: Intermediate Submission
         *    Description:      Get request from client
         *    Utility Function: char * get_request_server(int fd, size_t
         * *filelength)
         */
        char *request = get_request_server(fd, &file_size);
        if (request == NULL) {
            fprintf(stderr, "could not get client request");
            continue;
        }

        pthread_mutex_lock(&queue_access);
        while (QUEUE_FULL(queue) == 1) {
            pthread_cond_wait(&queue_slot_free, &queue_access);
        }

        enqueue(file_size, fd, request);
        free(request);

        pthread_cond_signal(&queue_has_content);
        pthread_mutex_unlock(&queue_access);

        /* TODO
         *    Description:      Add the request into the queue
             //(1) Copy the filename from get_request_server into allocated
         memory to put on request queue


             //(2) Request thread safe access to the request queue

             //(3) Check for a full queue... wait for an empty one which is
         signaled from req_queue_notfull

             //(4) Insert the request into the queue

             //(5) Update the queue index in a circular fashion

             //(6) Release the lock on the request queue and signal that the
         queue is not empty anymore
        */
    }
    return NULL;
}

void *worker(void *arg) {
    int num_request =
        0;  // Integer for tracking each request for printing into the log file
    // int fileSize = 0;  // Integer to hold the size of the file being
    // requested void *memory = NULL;  // memory pointer where contents being
    // requested are read and stored int fd = INVALID;  // Integer to hold the
    // file descriptor of incoming request char *mybuf;  // String to hold the
    // contents of the file being requested

    /* TODO : Intermediate Submission
     *    Description:      Get the id as an input argument from arg, set it to
     * ID
     */
    const unsigned int id = *((unsigned int *)arg);

    while (1) {
        /* TODO
        *    Description:      Get the request from the queue and do as follows
          //(1) Request thread safe access to the request queue by getting the
        req_queue_mutex lock
          //(2) While the request queue is empty conditionally wait for the
        request queue lock once the not empty signal is raised

          //(3) Now that you have the lock AND the queue is not empty, read from
        the request queue

          //(4) Update the request queue remove index in a circular fashion

          //(5) Fire the request queue not full signal to indicate the queue has
        a slot opened up and release the request queue lock
          */

        pthread_mutex_lock(&queue_access);
        while (QUEUE_EMPTY(queue)) {
            pthread_cond_wait(&queue_has_content, &queue_access);
        }

        request_t request = dequeue();

        pthread_cond_signal(&queue_slot_free);
        pthread_mutex_unlock(&queue_access);

        /* TODO
         *    Description:       Call image_match with the request buffer and
         * file size store the result into a typeof database_entry_t send the
         * file to the client using send_file_to_client(int socket, char *
         * buffer, int size)
         */

        database_entry_t entry = image_match(request.buffer, request.file_size);

        ++num_request;
        working_connection_fd[id] = request.file_descriptor;
        LogPrettyPrint(logfile, id, num_request, entry.file_name,
                       entry.file_size);

        if (send_file_to_client(request.file_descriptor, entry.buffer,
                                entry.file_size) == -1) {
            fprintf(stderr, "failed to send file back to client: %s\n",
                    entry.file_name);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("usage: %s port path num_dispatcher num_workers queue_length \n",
               argv[0]);
        return -1;
    }

    int port = -1;
    char path[BUFF_SIZE] = "no path set\0";
    num_dispatcher = -1;  // global variable
    num_worker = -1;      // global variable
    queue_len = -1;       // global variable

    /* TODO: Intermediate Submission
     *    Description:      Get the input args --> (1) port (2) path (3)
     * num_dispatcher (4) num_workers  (5) queue_length
     */
    port = atoi(argv[1]);
    strncpy(path, argv[2], BUFF_SIZE);
    num_dispatcher = atoi(argv[3]);
    num_worker = atoi(argv[4]);
    queue_len = atoi(argv[5]);

    /* TODO: Intermediate Submission
     *    Description:      Open log file
     *    Hint:             Use Global "File* logfile", use "server_log" as the
     * name, what open flags do you want?
     */
    logfile = fopen(LOG_FILE_NAME, "w");
    if (logfile == NULL) {
        perror("failed to open log file");
        exit(1);
    }

    // Create directory "output"
    if (mkdir("output", 0777) == -1) {
        if (errno != EEXIST) {
            perror("Error creating directory");
            return -1;
        }
    }

    // Create directory "output/img"
    if (mkdir("output/img", 0777) == -1) {
        if (errno != EEXIST) {
            perror("Error creating directory");
            return -1;
        }
    }

    /* TODO: Intermediate Submission
     *    Description:      Start the server
     *    Utility Function: void init(int port); //look in utils.h
     */
    init(port);

    /* TODO : Intermediate Submission
     *    Description:      Load the database
     */
    init_database();  // init database and queue
    init_queue();
    loadDatabase(path);

    /* TODO: Intermediate Submission
     *    Description:      Create dispatcher and worker threads
     *    Hints:            Use pthread_create, you will want to store pthread's
     * globally You will want to initialize some kind of global array to pass in
     * thread ID's How should you track this p_thread so you can terminate it
     * later? [global]
     */
    pthread_t *dispatcher_thread = malloc(sizeof(pthread_t) * num_dispatcher);
    pthread_t *worker_thread = malloc(sizeof(pthread_t) * num_worker);
    unsigned int *worker_id = malloc(sizeof(unsigned int) * num_worker);
    working_connection_fd = malloc(sizeof(int) * num_worker);

    for (int i = 0; i < num_dispatcher; ++i) {
        pthread_create(&dispatcher_thread[i], NULL, (void *)dispatch, NULL);
    }

    for (int i = 0; i < num_worker; ++i) {
        worker_id[i] = (unsigned int)i;
        pthread_create(&worker_thread[i], NULL, (void *)worker,
                       (void *)&worker_id[i]);
    }

    // Wait for each of the threads to complete their work
    // Threads (if created) will not exit (see while loop), but this keeps main
    // from exiting
    int i;
    for (i = 0; i < num_dispatcher; i++) {
        fprintf(stderr, "JOINING DISPATCHER %d \n", i);
        if ((pthread_join(dispatcher_thread[i], NULL)) != 0) {
            printf("ERROR : Fail to join dispatcher thread %d.\n", i);
        }
    }
    for (i = 0; i < num_worker; i++) {
        fprintf(stderr, "JOINING WORKER %d \n",i);
        if ((pthread_join(worker_thread[i], NULL)) != 0) {
            printf("ERROR : Fail to join worker thread %d.\n", i);
        }
    }
    fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
    fclose(logfile);                    // closing the log files

    clear_queue();
    clear_database();
    free(dispatcher_thread);
    free(worker_thread);
    free(worker_id);
}