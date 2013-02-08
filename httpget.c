#include <stdio.h>

void print_usage() {
    printf("Usage: ./httpget http://address/file.extension\n");
}

int main(int argc, char *argv[]) {
    char **args = &argv[1];
    int arg_count = argc - 1;
    if (arg_count < 1 || arg_count > 1) {
        print_usage();
        return 1;
    }
    return 0;
}
