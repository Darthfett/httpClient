#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define HOST_ADDRESS "127.0.0.1"
#define PORT 8000

int sockfd;
struct sockaddr_in address;

int main(int argc, char *argv[]) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Unable to open socket\n"); 
        exit(1);
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(HOST_ADDRESS);
    address.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        printf("Unable to connect to host\n");
        exit(1);
        return 1;
    }
}

void print_usage() {
    printf("Usage: ./httpget http://address/file.extension\n");
}

char* split_path(char *path, char **file_out) {
    char *pos = strrchr(path, '/');
    if (pos == NULL) {
        
    }
    int len_addr = pos - path;
    int len_file = &path[strlen(path)-1] - ++pos;
    
    char *addr = (char*) malloc(len_addr + 1);
    char *file = (char*) malloc(len_file + 1);

    strncpy(addr, path, len_addr);
    addr[len_addr] = '\0';

    strcpy(file, pos);

    *file_out = file;
    return addr;
}

int dontrunme(int argc, char *argv[]) {
    int arg_count = argc - 1;
    if (arg_count < 1 || arg_count > 1) {
        print_usage();
        return 1;
    }

    char *addr_start = argv[1];
    if (strncmp(addr_start, "http://", strlen("http://")) == 0) {
        addr_start += strlen("http://");
    }

    char *file_name;
    char *server_address = split_path(argv[1], &file_name);

    printf("Get file %s at server %s\n", file_name, server_address);

    return 0;
}
