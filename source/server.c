/*
    Include files
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*
    Macros
*/

#define SOCK_PORT   (8848)
#define BUFFER_SIZE (4096)
#define MAX_CLIENTS (10)
#define UPLOAD_DIR  "upload/"

#define LOG(fmt, ...)   printf("\033[33m"fmt"\033[0m\r\n", ##__VA_ARGS__);

/*
    Typedefs
*/

/*
    Variables
*/

static int connect_count = 0;

/*
    Declaration
*/

void* client_hdl(void *args);

int main()
{
    int server_fd = 0;
    struct sockaddr_in address = {};
    socklen_t addrlen = sizeof(address);
    int connect_fd = 0;
    pthread_t tid = 0;
    const char *file_dir = UPLOAD_DIR;
    mode_t mode = 0755;

    /* create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd <= 0)
    {
        perror("socket() failed");
        goto ERROR;
    }

    /* bind interface */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SOCK_PORT);
    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind() failed");
        goto ERROR;
    }

    /* listening */
    if(listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        goto ERROR;
    }

    /* mkdir for file_storage */
    mkdir(file_dir, mode);

    LOG("Server start listening on port %d...", SOCK_PORT);

    while(1)
    {
        connect_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if(connect_fd <= 0)
        {
            perror("accept() failed");
            goto ERROR;
        }

        printf("Connection add, total %d\r\n", ++ connect_count);

        /* create thread for handling client connect */
        pthread_create(&tid, NULL, client_hdl, (void*)&connect_fd);
    }

    return 0;

ERROR:
    if(server_fd > 0)   close(server_fd);
    rmdir(file_dir);
    exit(EXIT_FAILURE);
}

int receive_file(int connect_fd, const char* filename, int size)
{
    char filepath[256] = {};
    FILE *file = NULL;
    int remaining = 0;
    char buffer[BUFFER_SIZE] = {};

    LOG("upload filename %s, filesize %d", filename, size);

    snprintf(filepath, sizeof(filepath), "%s%s", UPLOAD_DIR, filename);

    file = fopen(filepath, "wb");
    if(!file)
    {
        LOG("Server open file fail\r\n");
        send(connect_fd, "ERROR 500 Failed to save file\n", 32, 0);
        return -1;
    }

    remaining = size;
    while(remaining > 0)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(connect_fd, buffer, BUFFER_SIZE, 0);
        if(bytes <= 0)  break;
        fwrite(buffer, 1, bytes, file);
        remaining -= bytes;
        printf("Upload (%d/%d) ...\r\n", size-remaining, size);
    }

    send(connect_fd, "OK 200", 16, 0);

    LOG("Upload [%s] done....", filepath);
    fclose(file);

    return 0;
}

void send_file(int connect_fd, char *filename)
{
    char filepath[256] = {};
    FILE *file = NULL;
    int filesize = 0;
    char buffer[BUFFER_SIZE] = {};
    int remaining = 0;

    snprintf(filepath, sizeof(filepath), "%s%s", UPLOAD_DIR, filename);
    file = fopen(filepath, "rb");
    if(!file)
    {
        send(connect_fd, "ERROR 404 File not found\n", 32, 0);
        LOG("File %s not found", filepath);
        return;
    }
    
    send(connect_fd, "OK 200\r\n", 16, 0);

    /* send file size */
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    send(connect_fd, &filesize, sizeof(filesize), 0);

    /* send file content */
    while(1)
    {
        int bytes = fread(buffer, 1, BUFFER_SIZE, file);
        if(bytes <= 0)  break;
        send(connect_fd, buffer, bytes, 0);
        remaining += bytes;
        printf("Download (%d/%d) ...\r\n", remaining, filesize);
    }

    fclose(file);
    LOG("Send file [%s] done", filepath);
}

void* client_hdl(void *args)
{
    int connect_fd = *(int*)args;
    char buffer[BUFFER_SIZE] = {};
    int bytes_recv = 0;

    while(1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_recv = recv(connect_fd, buffer, BUFFER_SIZE, 0);
        if(bytes_recv <= 0)     // connection shutdown
        {
            break;
        }

        char *command = strtok(buffer, " \n");
        printf("Server receive command: %s\r\n", command);
        if(strcmp(command, "upload") == 0)
        {
            char *filename = strtok(NULL, " ");
            char *size_str = strtok(NULL, " ");
            int size = atoi(size_str);

            if(!filename || !size_str)
            {
                send(connect_fd, "ERROR 400 Bad Request\r\n", 32, 0);
                continue;
            }

            if(0 == receive_file(connect_fd, filename, size))
            {
                send(connect_fd, "OK 200 File Upload", 32, 0);
            }
            else
            {
                send(connect_fd, "ERROR 600 Upload fail", 32, 0);
            }
        }
        else if(strcmp(command, "download") == 0)
        {
            char *filename = strtok(NULL, "\n");
            if(!filename)
            {
                send(connect_fd, "ERROR 400 Bad Request\r\n", 32, 0);
                continue;
            }

            send_file(connect_fd, filename);
        }
        else
        {
            LOG("Receive Unknown command: %s");
            send(connect_fd, "ERROR 405 Method not allowed\r\n", 32, 0);
        }
    }

    printf("Connection sub, total %d\r\n", -- connect_count);
    close(connect_fd);
    return NULL;
}