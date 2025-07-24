#include "vcs.h"

int create_directory(const char* path) {
    // 0755 sets permissions: owner can read/write/execute, group and others can read/execute
    if (mkdir(path, 0755) != 0) {
        // Check if the error is because the directory already exists
        if (errno != EEXIST) {
            // If it's a different error (not "already exists")
            perror("Failed to create file's versions directory");
            return -1;
        }
        // If errno == EEXIST, the directory already exists, which is fine
    }
    return 0; // Success
}

int file_exists(const char* filepath) {
    struct stat st;
    return (stat(filepath, &st) == 0);
}

void print_timestamp(time_t timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    printf("%04d-%02d-%02d %02d:%02d:%02d",
           tm_info->tm_year + 1900,
           tm_info->tm_mon + 1,
           tm_info->tm_mday,
           tm_info->tm_hour,
           tm_info->tm_min,
           tm_info->tm_sec);
}

void print_help() {
    printf("VCS - Simple Version Control System\n\n");
    printf("Usage:\n");
    printf("  vcs init                    - Initialize a new repository\n");
    printf("  vcs checkin <file> [comment] - Check in a file with optional comment\n");
    printf("  vcs checkout <file> [version] - Check out a file (latest version if not specified)\n");
    printf("  vcs list <file>             - List all versions of a file\n");
    printf("  vcs rollback <file> <version> - Rollback a file to a specific version\n");
    printf("\nExamples:\n");
    printf("  vcs init\n");
    printf("  vcs checkin myfile.txt \"Initial version\"\n");
    printf("  vcs checkout myfile.txt 1\n");
    printf("  vcs list myfile.txt\n");
    printf("  vcs rollback myfile.txt 2\n");
}
