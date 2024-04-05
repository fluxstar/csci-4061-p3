#include "../include/server.h"

// /********************* [ Helpful Global Variables ] **********************/
int num_dispatcher = 0; // Global integer to indicate the number of dispatcher threads   
int num_worker = 0;  //Global integer to indicate the number of worker threads
FILE *logfile;  //Global file pointer to the log file
int queue_len = 0; //Global integer to indicate the length of the queue

/* LOGAN ADDITIONS */
pthread_t dispatcher_thread[MAX_THREADS]; // array of pthreads for dispatcher threads
pthread_t worker_thread[MAX_THREADS]; // array of pthreads for worker threads
request_t request_queue[MAX_QUEUE_LEN]; // array of request_t for the request queue
database_entry_t database[100]; // array of database_entry_t for the database
int num_images_in_database = 0; // indicate the number of images in the database

pthread_mutex_t request_queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Global mutex for the request queue
pthread_cond_t request_queue_not_full = PTHREAD_COND_INITIALIZER; // Global condition variable for the request queue not full
pthread_cond_t request_queue_not_empty = PTHREAD_COND_INITIALIZER; // Global condition variable for the request queue not empty

int request_queue_head = 0;
int request_queue_tail = 0;
/* LOGAN ADDITIONS */

/* TODO: Intermediate Submission
  TODO: Add any global variables that you may need to track the requests and threads
  [multiple funct]  --> How will you track the p_thread's that you create for workers?
  [multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
  [multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
  What kind of locks will you need to make everything thread safe? [Hint you need multiple]
  What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
  How will you track the number of images in the database?
  How will you track the requests globally between threads? How will you ensure this is thread safe? Example: request_t req_entries[MAX_QUEUE_LEN]; 
  [multiple funct]  --> How will you update and utilize the current number of requests in the request queue?
  [worker()]        --> How will you track which index in the request queue to remove next? 
  [dispatcher()]    --> How will you know where to insert the next request received into the request queue?
  [multiple funct]  --> How will you track the p_thread's that you create for workers? TODO
  How will you store the database of images? What data structure will you use? Example: database_entry_t database[100]; 
*/


//TODO: Implement this function
/**********************************************
 * image_match
   - parameters:
      - input_image is the image data to compare
      - size is the size of the image data
   - returns:
       - database_entry_t that is the closest match to the input_image
************************************************/
//just uncomment out when you are ready to implement this function
database_entry_t image_match(char *input_image, int size)
{
/* LOGAN ADDITIONS */
  const char *closest_file     = NULL;
	int         closest_distance = INT_MAX;
  int closest_index = 0;
    
  for (int i = 0; i < num_images_in_database; i++) {
		const char *current_file = database[i].buffer;
		int result = memcmp(input_image, current_file, size);
    // printf("Compared to %s -- Result: %d\n", database[i].file_name, result);
		if(result == 0)
		{
			return database[i];
		}

		else if(result < closest_distance)
		{
			closest_distance = result;
			closest_file     = current_file;
      closest_index = i;
		}
	}

	if(closest_file != NULL)
	{
    return database[closest_index];
	}
  else
  {
    return database[closest_index];
  }
/* LOGAN ADDITIONS */
}

//TODO: Implement this function
/**********************************************
 * LogPrettyPrint
   - parameters:
      - to_write is expected to be an open file pointer, or it 
        can be NULL which means that the output is printed to the terminal
      - All other inputs are self explanatory or specified in the writeup
   - returns:
       - no return value
************************************************/
void LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, char * file_name, int file_size){
  /* LOGAN ADDITIONS */
  // [threadId][reqNum][fd][Request string][bytes/error]
  if (to_write == NULL) {
      printf("[%d][%d][%d][%s][%d]\n", threadId, requestNumber, 666, file_name, file_size); // 666 is a placeholder for the file descriptor
  } else {
      fprintf(to_write, "[%d][%d][%d][%s][%d]\n", threadId, requestNumber, 666, file_name, file_size); // 666 is a placeholder for the file descriptor
  }
  /* LOGAN ADDITIONS */
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
  /* LOGAN ADDITIONS */
  DIR *dir;
  struct dirent *entry;

  dir = opendir(path);
  if (dir == NULL) {
    fprintf(stderr, "Error opening directory: %s\n", path);
    return;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG) {
      char file_path[1028];
      sprintf(file_path, "%s/%s", path, entry->d_name);

      FILE *fd = fopen(file_path, "rb");
      if (fd == NULL) {
        fprintf(stderr, "Error opening file: %s\n", file_path);
        return;
      }

      fseek(fd, 0, SEEK_END);
      long file_size = ftell(fd);
      fseek(fd, 0, SEEK_SET);

      char *buffer = (char *)malloc(file_size);
      fread(buffer, 1, file_size, fd);
      fclose(fd);

      database[num_images_in_database].file_name = strdup(entry->d_name); //strdup allocates memory for the string (filename, no path)
      database[num_images_in_database].file_size = file_size;
      database[num_images_in_database].buffer = buffer;

      num_images_in_database++;
    }
  }
  closedir(dir);
  /* LOGAN ADDITIONS */
}


void *dispatch(void *arg) 
{   
  /* LOGAN ADDITIONS */

  int thread_id = *(int *)arg;
  fprintf(stderr, "Dispatch ID: %d\n", thread_id);
  free(arg);

  // while (1) { // WE ONLY WANT THIS TO RUN ONCE FOR THE INTERMEDIATE SUBMISSION
    size_t file_size = 0;
    request_detials_t request_details;

    /* TODO: Intermediate Submission
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void)
    */
    int client_fd = accept_connection();
    if (client_fd < 0) {
      fprintf(stderr, "Error accepting connection\n");
      // continue; INTERMEDIATE SUBMISSION
      return NULL;
    }
    
    /* TODO: Intermediate Submission
    *    Description:      Get request from client
    *    Utility Function: char * get_request_server(int fd, size_t *filelength)
    */

    char* request = get_request_server(client_fd, &file_size);
    if (request == NULL) {
      fprintf(stderr, "Null dispatcher request, continuing...\n");
      close(client_fd);
      // continue; INTERMEDIATE SUBMISSION
      return NULL;
    }

   /* TODO
    *    Description:      Add the request into the queue    */
        //(1) Copy the filename from get_request_server into allocated memory to put on request queue
      strncpy(request_details.buffer, request, sizeof(request_details.buffer) - 1);
      request_details.buffer[sizeof(request_details.buffer) - 1] = '\0'; // Ensure null-termination
      //(2) Request thread safe access to the request queue
      pthread_mutex_lock(&request_queue_mutex);
      //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull
      while (((request_queue_head + 1) % MAX_QUEUE_LEN) == request_queue_tail) {
          pthread_cond_wait(&request_queue_not_full, &request_queue_mutex);
      }
      //(4) Insert the request into the queue
      fprintf(stderr, "Request file size: %ld\n", file_size);
      request_queue[request_queue_head].file_size = file_size;
      request_queue[request_queue_head].file_descriptor = client_fd;
      request_queue[request_queue_head].buffer = request_details.buffer;
      //(5) Update the queue index in a circular fashion
      request_queue_head = (request_queue_head + 1) % MAX_QUEUE_LEN;
      //(6) Release the lock on the request queue and signal that the queue is not empty anymore
      pthread_cond_signal(&request_queue_not_empty);
      pthread_mutex_unlock(&request_queue_mutex);
  // }
    return NULL;
  /* LOGAN ADDITIONS */
}

void * worker(void *arg) {
  /* LOGAN ADDITIONS */
  int num_request = 0;                                    //Integer for tracking each request for printing into the log file
  int fileSize    = 0;                                    //Integer to hold the size of the file being requested
  // void *memory    = NULL;                              //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char *mybuf;                                  //String to hold the contents of the file being requested


  /* TODO : Intermediate Submission 
  *    Description:      Get the id as an input argument from arg, set it to ID
  */
  int thread_id = *(int *)arg;
  fprintf(stderr, "Worker ID: %d\n", thread_id);
  free(arg); // free the memory allocated for the argument (NECESSARY?)
    
  // while (1) { // WE ONLY WANT THIS TO RUN ONCE FOR THE INTERMEDIATE SUBMISSION
    /* TODO
    *    Description:      Get the request from the queue and do as follows
      //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock */
      pthread_mutex_lock(&request_queue_mutex);
      //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
      while (request_queue_head == request_queue_tail) {
        pthread_cond_wait(&request_queue_not_empty, &request_queue_mutex);
      }
      //(3) Now that you have the lock AND the queue is not empty, read from the request queue
      fd = request_queue[request_queue_tail].file_descriptor;
      fileSize = request_queue[request_queue_tail].file_size;
      mybuf = request_queue[request_queue_tail].buffer;
      //(4) Update the request queue remove index in a circular fashion
      request_queue_tail = (request_queue_tail + 1) % MAX_QUEUE_LEN;
      //(5) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock  
      pthread_cond_signal(&request_queue_not_full);
      pthread_mutex_unlock(&request_queue_mutex);
      
    /* TODO
    *    Description:       Call image_match with the request buffer and file size
    *    store the result into a typeof database_entry_t
    *    send the file to the client using send_file_to_client(int socket, char * buffer, int size)              
    */
    // printf("Testing image with fd: %d and file size: %d \n", fd, fileSize);
    database_entry_t result = image_match(mybuf, fileSize); // the mix of snake_case and camelCase is a bit confusing
    // printf("Sending file to client\n");
    if (send_file_to_client(fd, result.buffer, result.file_size) == -1) {
      fprintf(stderr, "Error sending file to client\n");
    }
    close(fd);

    num_request++;
    LogPrettyPrint(logfile, thread_id, num_request, result.file_name, result.file_size);

    // free(mybuf);
  // }
  return NULL;
  /* LOGAN ADDITIONS */
}

int main(int argc , char *argv[])
{
   if(argc != 6){
    printf("usage: %s port path num_dispatcher num_workers queue_length \n", argv[0]);
    return -1;
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
  

  int port            = -1;
  char path[BUFF_SIZE] = "no path set\0";
  num_dispatcher      = -1;                               //global variable
  num_worker          = -1;                               //global variable
  queue_len           = -1;                               //global variable
 

  /* TODO: Intermediate Submission
  *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length
  */
  /* LOGAN ADDITIONS */
  port = atoi(argv[1]);
  strcpy(path, argv[2]); // assuming the path is null-terminated
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);
  /* LOGAN ADDITIONS */

  /* TODO: Intermediate Submission
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use "server_log" as the name, what open flags do you want?
  */
  /* LOGAN ADDITIONS */
  logfile = fopen(LOG_FILE_NAME, "w");
  if (logfile == NULL) {
    fprintf(stderr, "Error opening log file\n");
    return -1;
  }
  /* LOGAN ADDITIONS */

  /* TODO: Intermediate Submission
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */

  /* LOGAN ADDITIONS */
  init(port);
  /* LOGAN ADDITIONS */

  /* TODO : Intermediate Submission
  *    Description:      Load the database
  */

  /* LOGAN ADDITIONS */
  loadDatabase(path);
  /* LOGAN ADDITIONS */
 
  /* TODO: Intermediate Submission
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */

  /* LOGAN ADDITIONS */
  for (int i = 0; i < num_dispatcher; i++) { 
    int *thread_id = (int *)malloc(sizeof(int));
    *thread_id = i;
    // printf("Creating dispatcher thread, passing in threadID arg: %d\n", *thread_id);
    pthread_create(&dispatcher_thread[i], NULL, (void *)dispatch, (void *)thread_id);
  }

  for (int i = 0; i < num_worker; i++) {
    int *thread_id = (int *)malloc(sizeof(int));
    *thread_id = i;
    // printf("Creating worker thread, passing in threadID arg: %d\n", *thread_id);
    pthread_create(&worker_thread[i], NULL, (void *)worker, (void *)thread_id);
    // free(thread_id); // free the memory allocated for the thread_id
  }
/* LOGAN ADDITIONS */

  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  int i;
  for(i = 0; i < num_dispatcher; i++){
    fprintf(stderr, "JOINING DISPATCHER %d \n",i);
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){ 
      printf("ERROR : Fail to join dispatcher thread %d.\n", i);
    }
  }
  for(i = 0; i < num_worker; i++){
   fprintf(stderr, "JOINING WORKER %d \n",i);
    if((pthread_join(worker_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join worker thread %d.\n", i);
    }
  }
  fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
  fclose(logfile);//closing the log files

  return 0;
}