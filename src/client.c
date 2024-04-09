#include "../include/client.h"

int port = 0;

pthread_t worker_thread[100];
int worker_thread_id = 0;
char output_path[1018];

processing_args_t req_entries[100];

/* TODO: implement the request_handle function to send the image to the server and recieve the processed image
* 1. Open the file in the read-binary mode - Intermediate Submission
* 2. Get the file length using the fseek and ftell functions - Intermediate Submission
* 3. set up the connection with the server using the setup_connection(int port) function - Intermediate Submission
* 4. Send the file to the server using the send_file_to_server(int socket, FILE *fd, size_t size) function - Intermediate Submission
* 5. Receive the processed image from the server using the receive_file_from_server(int socket, char *file_path) function
* 6. receive_file_from_server saves the processed image in the output directory, so pass in the right directory path
* 7. Close the file and the socket
*/
void *request_handle(void *args) {
    processing_args_t *req_entries = (processing_args_t *)args;
    if (req_entries == NULL){
        fprintf(stderr, "Request args are null\n");
        return NULL;
    } 

    char *file_path = req_entries->file_name;
    
    //Opening file and reading in binary mode 
    FILE *fp = fopen(file_path, "rb"); 
    if (fp == NULL) {
        fprintf(stderr, "Error opening file: %s\n", file_path);
        return NULL;
    }
     
    // Get the file length using the fseek and ftell functions
    fseek(fp, 0, SEEK_END);
    long filelength = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int sockfd = setup_connection(port);
    printf("Connection established with server\n");
    if (sockfd < 0) {
        fprintf(stderr, "Error setting up connection\n");
        fclose(fp);
        return NULL;
    }
    
    // Send the file to the server 
    printf("Sending file %s to server\n", file_path);
    
    if (send_file_to_server(sockfd, fp, filelength) != 0) {
        fprintf(stderr, "Error sending file to server\n");
        fclose(fp);
        return NULL; 
    }

    // Receive the processed image from the server
    char output_file[BUFFER_SIZE + 256];
    char *file_name = strrchr(file_path, '/');
    file_name++; // Skip the '/'
    printf("file_name: %s | file_path: %s\n", file_name, file_path);
    snprintf(output_file, sizeof(output_file), "%s/%s", output_path, file_name);
    printf("Receiving processed file %s from server\n", output_file);
    if (receive_file_from_server(sockfd, output_file) != 0) {
        fprintf(stderr, "Error receiving file from server\n");
        fclose(fp);
        return NULL;
    }  

    // Close the file and the socket
    fclose(fp);
    close(sockfd);

    return NULL;
}

/* TODO: Intermediate Submission
* implement the directory_trav function to traverse the directory and send the images to the server 
* 1. Open the directory
* 2. Read the directory entries
* 3. If the entry is a file, create a new thread to invoke the request_handle function which takes the file path as an argument
* 4. Join all the threads
* Note: Make sure to avoid any race conditions when creating the threads and passing the file path to the request_handle function. 
* use the req_entries array to store the file path and pass the index of the array to the thread. 
*/
void directory_trav(char *args) {
    DIR *dir;
    struct dirent *entry;
    struct stat info;

    dir = opendir(args);
    if (dir == NULL) {
        fprintf(stderr, "could not find or open directory: %s\n", args);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char *fn = malloc(sizeof(char) * BUFF_SIZE);
        snprintf(fn, BUFFER_SIZE, "%s/%s", args, entry->d_name);
         
        // might break. fn may only need to be name and not path
        if (stat(fn, &info) != 0) {
            fprintf(stderr, "could not get stats for file: %s\n", fn);
            continue;
        }
        if (!S_ISREG(info.st_mode)) continue;

        req_entries[worker_thread_id].file_name = fn;
        req_entries[worker_thread_id].number_worker = worker_thread_id;

        if (pthread_create(&worker_thread[worker_thread_id], NULL, 
            (void *) request_handle, (void *) &req_entries[worker_thread_id]) != 0) {
            fprintf(stderr, "error starting thread %d\n", worker_thread_id);
        }

        ++worker_thread_id;
    }
    
    closedir(dir);
    for (int i = 0; i < worker_thread_id; ++i) {
        pthread_join(worker_thread[i], NULL);
        free(req_entries[i].file_name);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: ./client <directory path> <Server Port> <output path>\n");
        exit(-1);
    }
    /*TODO:  Intermediate Submission
    * 1. Get the input args --> (1) directory path (2) Server Port (3) output path
    */
    char *dir_path = argv[1];
    port = atoi(argv[2]);
    strcpy(output_path, argv[3]);
    
    // printf("params received: %s %d %s\n", dir_path, port, output_path);

    /*TODO: Intermediate Submission
    * Call the directory_trav function to traverse the directory and send the images to the server
    */
    directory_trav(dir_path);

    return 0;  
}