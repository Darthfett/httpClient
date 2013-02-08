void print_usage() {
    printf("Usage: ./httpget http://address/file.extension");
}

int main(int argc, char *argv[]) {
    if (argc < 1 || argc > 1) {
        print_usage();
        return 1;
    }
    return 0;
}