/*
    Include files
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>


/*
    Macros
*/

#define SOCK_PORT   (8848)
#define BUFFER_SIZE (4096)
#define LOG(fmt, ...)   printf("\033[33m"fmt"\033[0m\r\n", ##__VA_ARGS__);

void send_file(int connect_fd, char *filename)
{
    FILE* file = NULL;
    int filesize = 0;
    char command[BUFFER_SIZE] = {};
    char buffer[BUFFER_SIZE] = {};
    char response[BUFFER_SIZE] = {};
    int sended = 0;

    file = fopen(filename, "rb");
    if(!file)
    {
        printf("Error: file not found\r\n");
        return;
    }

    /* get file size */
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* send command */
    snprintf(command, sizeof(command), "upload %s %d\n", filename, filesize);
    send(connect_fd, command, strlen(command), 0);

    /* send file content */
    while(1)
    {
        int bytes = fread(buffer, 1, BUFFER_SIZE, file);
        if(bytes <= 0)  break;
        send(connect_fd, buffer, bytes, 0);
        sended += bytes;
        printf("Upload (%d/%d) ...\r\n", sended, filesize);
    }

    fclose(file);

    /* receive response */
    recv(connect_fd, response, sizeof(response), 0);
    LOG("Server response: %s", response);
}

void recv_file(int connect_fd, char *filename)
{
    char command[BUFFER_SIZE] = {};
    char response[BUFFER_SIZE] = {};
    int filesize = 0;
    char buffer[BUFFER_SIZE] = {};
    int remaining = 0;
    FILE *file = NULL;

    /* send command to server */
    snprintf(command, sizeof(command), "download %s\n", filename);
    send(connect_fd, command, strlen(command), 0);

    /* receive response from server */
    recv(connect_fd, response, sizeof(response), 0);
    if(strncmp(response, "OK", 2) != 0)
    {
        LOG("Error: %s\r\n", response);
        return;
    }
    printf("Server response: %s", response);

    /* receive file size */
    recv(connect_fd, &filesize, sizeof(filesize), 0);
    printf("File size: %d\r\n", filesize);

    /* receive file content */
    file = fopen(filename, "wb");
    if(!file)
    {
        LOG("Open file %s fail", filename);
        return;
    }

    remaining = filesize;
    while(remaining > 0)
    {
        int bytes = recv(connect_fd, buffer, BUFFER_SIZE, 0);
        if(bytes <= 0)  break;
        fwrite(buffer, 1, bytes, file);
        remaining -= bytes;
        printf("Download (%d/%d) ...\r\n", filesize-remaining, filesize);
    }

    fclose(file);

    LOG("Download file [%s] done", filename);
}

int main(int argc, char *argv[])
{
    int socket_fd = 0;
    struct sockaddr_in address = {};
    char command[256] = {};

    if(argc != 2)
    {
        LOG("Error: input param count");
        exit(EXIT_FAILURE);
    }

    if(1 != inet_pton(AF_INET, argv[1], &address.sin_addr))
    {
        LOG("Error: input server ip is invalid");
        exit(EXIT_FAILURE);
    }

    /* create socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd <= 0)
    {
        perror("socket() failed");
        goto ERROR;
    }

    /* connect */
    address.sin_family = AF_INET;
    address.sin_port = htons(SOCK_PORT);

    if(connect(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("connect() failed");
        goto ERROR;
    }

    while(1)
    {
        printf("Enter command (upload <filename> | download <filename>): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = 0;        // delete \n

        if(strncmp(command, "upload", strlen("upload")) == 0)
        {
            char *filename = strtok(command+strlen("upload "), " ");
            if(!filename)
            {
                printf("Error: please enter filename.\r\n");
                continue;
            }
            send_file(socket_fd, filename);
        }
        else if(strncmp(command, "download", strlen("download")) == 0)
        {
            char *filename = strtok(command+strlen("download "), " ");
            if(!filename)
            {
                printf("Error: please enter filename.\r\n");
                continue;
            }
            recv_file(socket_fd, filename);
        }
        else
        {
            printf("Error: unsupport command, only support \"upload <filename>\" and \"download <filename>\"\r\n");
        }
    }

    return 0;

ERROR:
    if(socket_fd > 0)   close(socket_fd);
    exit(EXIT_FAILURE);
}