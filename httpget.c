#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

void print_usage() {
    printf("Usage: ./httpget http://address/file.extension\n");
}

char* split_path(char *path, char **file_out) {
    char *pos = strrchr(path, '/');
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

int main(int argc, char *argv[]) {
    int arg_count = argc - 1;
    if (arg_count < 1 || arg_count > 1) {
        print_usage();
        return 1;
    }

    char *file_name;
    char *server_address = split_path(argv[1], &file_name);

    printf("Get file %s at server %s\n", file_name, server_address);

    return 0;
}
