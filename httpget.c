#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#define HOST_ADDRESS "127.0.0.1"
#define PORT 80
#define MAX_RETRIES 5

#define TRUE 1
#define FALSE 0

int sockfd;
struct sockaddr_in address;

int content_length = -1;
char location[1024];
int header_err_flag = FALSE;

int read_line(int fd, char *buffer, int size) {
    char next = '\0';
    char err;
    int i = 0;
    while ((i < (size - 1)) && (next != '\n')) {
        err = read(fd, &next, 1);

        if (err <= 0) break;

        if (next == '\r') {
            err = recv(fd, &next, 1, MSG_PEEK);
            if (err > 0 && next == '\n') {
                read(fd, &next, 1);
            } else {
                next = '\n';
            }
        }
        buffer[i] = next;
        i++;
    }
    buffer[i] = '\0';
    return i;
}

int read_socket(int fd, char *buffer, int size) {
    int bytes_recvd = 0;
    int retries = 0;
    int total_recvd = 0;

    while (retries < MAX_RETRIES && size > 0 && strstr(buffer, ">") == NULL) {
        bytes_recvd = read(fd, buffer, size);

        if (bytes_recvd > 0) {
            buffer += bytes_recvd;
            size -= bytes_recvd;
            total_recvd += bytes_recvd;
        } else {
            retries++;
        }
    }

    if (bytes_recvd >= 0) {
        // Last read was not an error, return how many bytes were recvd
        return total_recvd;
    }
    // Last read was an error, return error code
    return -1;
}

int write_socket(int fd, char *msg, int size) {
    int bytes_sent = 0;
    int retries = 0;
    int total_sent = 0;

    while (retries < MAX_RETRIES && size > 0) {
        bytes_sent = write(fd, msg, size);

        if (bytes_sent > 0) {
            msg += bytes_sent;
            size -= bytes_sent;
            total_sent += bytes_sent;
        } else {
            retries++;
        }
    }

    if (bytes_sent >= 0) {
        // Last write was not an error, return how many bytes were sent
        return total_sent;
    }
    // Last write was an error, return error code
    return -1;
}

void read_headers() {
    // printf("\n--READ HEADERS--\n\n");
    while(1) {
        char header[8096];
        int len;
        char next;
        int err;
        char *header_value_start;
        len = read_line(sockfd, header, sizeof(header));

        if (len <= 0) {
            // Error in reading from socket
            header_err_flag = TRUE;
            continue;
        }

        // printf("%s", header);

        if (strcmp(header, "\n") == 0) {
            // Empty line signals end of HTTP Headers
            return;
        }

        // If the next line begins with a space or tab, it is a continuation of the previous line.
        err = recv(sockfd, &next, 1, MSG_PEEK);
        while (isspace(next) && next != '\n' && next != '\r') {
            if (err) {
                printf("header space/tab continuation check err\n");
                // Not sure what to do in this scenario
            }
            // Read the space/tab and get rid of it 
            read(sockfd, &next, 1);
            
            // Concatenate the next line to the current running header line
            len = len + read_line(sockfd, header + len, sizeof(header) - len);
            err = recv(sockfd, &next, 1, MSG_PEEK);
        }

        // Find first occurence of colon, to split by header type and value
        header_value_start = strchr(header, ':');
        if (header_value_start == NULL) {
            // Invalid header, not sure what to do in this scenario
            printf("invalid header\n");
            header_err_flag = TRUE;
            continue;
        }
        int header_type_len = header_value_start - header;

        // Increment header value start past colon
        header_value_start++;
        // Increment header value start to first non-space character
        while (isspace(*header_value_start) && (*header_value_start != '\n') && (*header_value_start != '\r')) {
            header_value_start++;
        }
        int header_value_len = len - (header_value_start - header);

        // We only care about Content-Length
        if (strncasecmp(header, "Content-Length", header_type_len) == 0) {
            content_length = atoi(header_value_start);
        } else if (strncasecmp(header, "Location", header_type_len) == 0) {
            strncpy(location, header_value_start, header_value_len);
        }
    }
}

void print_usage() {
    printf("Usage: ./httpget http://address/file.extension\n");
}

char* split_path(char *path, char **file_out) {
    char *pos = strchr(path, '/');
    if (pos == NULL) {
        char *addr = (char*) malloc(strlen(path) + 1);
        char *file = (char*) malloc(2);
        strncpy(addr, path, strlen(path));
        strncpy(file, "/", 1);
        *file_out = file;
        return addr;
    }
    int len_addr = pos - path;
    int len_file = strlen(path) - (pos - path);
    
    char *addr = (char*) malloc(len_addr + 1);
    char *file = (char*) malloc(len_file + 1);

    strncpy(addr, path, len_addr);
    addr[len_addr] = '\0';

    strcpy(file, pos);

    *file_out = file;
    return addr;
}

int split_port(char *addr) {
    char *pos = strrchr(addr, ':');
    if (pos == NULL) {
        return PORT;
    }
    int port = atoi(pos + 1);
    *pos = '\0';
    return port;
}

int main(int argc, char *argv[]) {
    int arg_count = argc - 1;
    if (arg_count < 1 || arg_count > 1) {
        print_usage();
        return 1;
    }

    strcpy(location, "\0");
    
    // The host address and file we wish to GET, e.g. http://127.0.0.1/blah.txt
    char *addr_start = argv[1];

    // Skip past http:// if it's there
    if (strncasecmp(addr_start, "http://", strlen("http://")) == 0) {
        addr_start += strlen("http://");
    }

    // Split the host address into a web address and a file name/path, 
    // e.g. 127.0.0.1/blah.txt into 127.0.0.1 and /blah.txt
    char *file_name;
    char *server_address = split_path(addr_start, &file_name);
    int port = split_port(server_address);
    struct hostent *host_entity = gethostbyname(server_address);

    printf("Get file %s at server %s on port %d\n", file_name, server_address, port);

    // Open up a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Unable to open socket\n"); 
        exit(1);
        return 1;
    }

    // Try to connect to server_address on port PORT
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr, host_entity->h_addr_list[0], host_entity->h_length);
    // address.sin_addr.s_addr = inet_addr(server_address);
    address.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        printf("Unable to connect to host\n");
        exit(1);
        return 1;
    }

    printf("Connected with host\n");

    /*******************************************************/
    /*             Begin sending GET request               */
    /*******************************************************/
    char buffer[8096];
    sprintf(buffer, "GET %s HTTP/1.1\r\n", file_name);
    write_socket(sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Host: %s:%d\r\n", server_address, port);
    write_socket(sockfd, buffer, strlen(buffer));
    sprintf(buffer, "User-Agent: Test HTTP Client\r\n");
    write_socket(sockfd, buffer, strlen(buffer));
    sprintf(buffer, "Connection: close\r\n");
    write_socket(sockfd, buffer, strlen(buffer));

    // Signal end of headers with empty line
    sprintf(buffer, "\r\n");
    write_socket(sockfd, buffer, strlen(buffer));

    /*******************************************************/
    /*             Begin reading response                  */
    /*******************************************************/

    // Read first line and print to console
    int len = read_line(sockfd, buffer, sizeof(buffer));

    if (len <= 0) {
        printf("No response received from server\n");
        goto out;
    } else {
        char time_buffer[512];
        time_t raw_time;
        struct tm *current_time;
        time(&raw_time);
        current_time = localtime(&raw_time);
        strftime(time_buffer, 512, "%a, %d %b %Y %T %Z", current_time);
        printf("[%s] Response received from server\n", time_buffer);
    }

    // printf("%s\n", buffer);
    char version[16];
    char response_code[16];
    char response_reason[256];
    int i, j;

    // Read version
    for(i = 0; i < sizeof(version) - 1 && !isspace(buffer[i]); i++) {
        version[i] = buffer[i];
    }
    version[i] = '\0';

    // Skip over spaces
    for (; isspace(buffer[i]) && i < sizeof(buffer); i++);

    // Read response code
    for (j = 0; i < sizeof(buffer) && j < sizeof(response_code) - 1 && !isspace(buffer[i]); i++, j++) {
        response_code[j] = buffer[i];
    }
    response_code[j] = '\0';

    // Skip over spaces
    for (; isspace(buffer[i]) && i < sizeof(buffer); i++);

    // Read response reason
    for (j = 0; i < sizeof(buffer) && j < sizeof(response_reason) - 1 && buffer[i] != '\n'; i++, j++) {
        response_reason[j] = buffer[i];
    }
    response_reason[j] = '\0';

    // printf("Version: %s\n", version);
    // printf("Response Code: %s\n", response_code);
    // printf("Response Reason: %s\n", response_reason);

    read_headers();

    if (strcmp(response_code, "200") != 0) {
        if (strcmp(response_code, "301") == 0) {
            printf("%s Error: %s to %s\n", response_code, response_reason, location);
        } else {
            printf("%s Error: %s\n", response_code, response_reason);
        }
        goto out;
    }

    if (header_err_flag) {
        printf("Error reading headers\n");
        goto out;
    }

    if (content_length <= 0) {
        printf("No content received from server\n"); 
        goto out;
    }

    char *result = (char*) malloc(content_length + 1);
    len = read_socket(sockfd, result, content_length);
    if (len <= 0) goto out;
    result[len] = '\0';

    char *out_file;
    char *file_start = strrchr(file_name, '/') + 1;

    if (strcmp(file_start, "") == 0) {
        out_file = "out/index.html";
    } else {
        out_file = (char*) malloc(strlen(file_start) + 5);
        strcpy(out_file, "out/");
        strcpy(out_file + 4, file_start);
    }

    // printf("%s\n", result); 

    FILE *out = fopen(out_file, "w");
    if (out == NULL) {
        printf("Unable to open file %s\n", out_file);
        goto out;
    }

    fprintf(out, "%s", result);

    fclose(out);

    printf("Result saved in %s\n", out_file);

out:
    // Close the connection down
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return 0;
}
