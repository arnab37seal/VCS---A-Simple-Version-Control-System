#include "vcs.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    char current_dir[MAX_PATH_LEN];
    getcwd(current_dir, sizeof(current_dir));
    
    Repository* repo = NULL;
    
    // Handle init command separately
    if (strcmp(argv[1], "init") == 0) {
        if (repository_exists(current_dir)) {
            printf("Repository already exists in this directory.\n");
            return 1;
        }
        
        if (init_repository(current_dir) == 0) {
            printf("Initialized empty repository in %s\n", current_dir);
        } else {
            printf("Failed to initialize repository.\n");
            return 1;
        }
        return 0;
    }
    
    // Check if repository exists for other commands
    if (!repository_exists(current_dir)) {
        printf("No repository found. Use 'init' to create one.\n");
        return 1;
    }
    
    repo = load_repository(current_dir);
    if (!repo) {
        printf("Failed to load repository.\n");
        return 1;
    }
    
    // Handle different commands
    if (strcmp(argv[1], "checkin") == 0) {
        if (argc < 3) {
            printf("Usage: %s checkin <filename> [comment]\n", argv[0]);
            cleanup_repository(repo);
            return 1;
        }
        
        char* comment = (argc >= 4) ? argv[3] : "No comment provided";
        
        if (!file_exists(argv[2])) {
            printf("File '%s' does not exist.\n", argv[2]);
            cleanup_repository(repo);
            return 1;
        }
        
        int version = checkin_file(repo, argv[2], comment);
        if (version > 0) {
            printf("Checked in '%s' as version %d\n", argv[2], version);
        } else {
            printf("Failed to check in file.\n");
        }
    }
    else if (strcmp(argv[1], "checkout") == 0) {
        if (argc < 3) {
            printf("Usage: %s checkout <filename> [version]\n", argv[0]);
            cleanup_repository(repo);
            return 1;
        }
        
        int version = (argc >= 4) ? atoi(argv[3]) : get_latest_version(repo, argv[2]);
        
        if (checkout_file(repo, argv[2], version) == 0) {
            printf("Checked out '%s' version %d\n", argv[2], version);
        } else {
            printf("Failed to check out file.\n");
        }
    }
    else if (strcmp(argv[1], "list") == 0) {
        if (argc < 3) {
            printf("Usage: %s list <filename>\n", argv[0]);
            cleanup_repository(repo);
            return 1;
        }
        
        if (list_versions(repo, argv[2]) != 0) {
            printf("No versions found for '%s'\n", argv[2]);
        }
    }
    else if (strcmp(argv[1], "rollback") == 0) {
        if (argc < 4) {
            printf("Usage: %s rollback <filename> <version>\n", argv[0]);
            cleanup_repository(repo);
            return 1;
        }
        
        int version = atoi(argv[3]);
        if (rollback_to_version(repo, argv[2], version) == 0) {
            printf("Rolled back '%s' to version %d\n", argv[2], version);
        } else {
            printf("Failed to rollback file.\n");
        }
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        print_help();
        cleanup_repository(repo);
        return 1;
    }
    
    cleanup_repository(repo);
    return 0;
}