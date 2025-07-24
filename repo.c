#include "vcs.h"

/*
The function init_repository initializes a new VCS repository at the specified path by creating the .vcs directory,
its subdirectories (versions and temp), and an initial metadata file (versions.meta).
Returns -1 if the .vcs directory creation fails.
Returns 0 to indicate successful initialization.
*/
int init_repository(const char* path) {
    // Array to construct the full path to the .vcs directory (e.g., /home/rahul/project/.vcs)
    char vcs_path[MAX_PATH_LEN];    // MAX_PATH_LEN is defined in vcs.h as 512
    
    // We use snprintf (safe string formatting) to construct the path to the .vcs directory by combining the input path (e.g., /home/rahul/project) with VCS_DIR (defined as ".vcs" in vcs.h) into vcs_path (e.g., /home/rahul/project/.vcs)
    // sizeof(vcs_path) ensures the string doesn’t exceed the buffer size, preventing overflow
    // snprintf() will always null-terminate the output (as long as size > 0)
    snprintf(vcs_path, sizeof(vcs_path), "%s/%s", path, VCS_DIR);

    // Create the .vcs directory with permissions 0755 (read/write/execute for owner, read/execute for group and others)
    // If this fails (e.g., directory already exists or insufficient permissions), it prints an error and returns -1
    // mkdir(vcs_path, 0755)
    if (mkdir(vcs_path, 0755) != 0) {
        perror("Failed to create VCS directory");
        return -1;
    }
    
    // Create subdirectories

    // Arrays to store paths for subdirectories: versions (to store versioned files) and temp (for temporary files during operations)
    char versions_path[MAX_PATH_LEN];
    char temp_path[MAX_PATH_LEN];

    snprintf(versions_path, sizeof(versions_path), "%s/versions", vcs_path);    // Constructs the path to the versions subdirectory
    snprintf(temp_path, sizeof(temp_path), "%s/temp", vcs_path);    // Constructs the path to the temp subdirectory
    
    // Creates the versions and temp subdirectories with 0755 permissions (read/write/execute for owner, read/execute for group and others)
    // mkdir(versions_path, 0755);
    // mkdir(temp_path, 0755);
    if (mkdir(versions_path, 0755) != 0) {
        perror("Failed to create versions subdirectory");
        return -1;
    }
    if (mkdir(temp_path, 0755) != 0) {
        perror("Failed to create temp subdirectory");
        return -1;
    }

    // Create initial metadata file
    
    char metadata_path[MAX_PATH_LEN];   // array for the path to the metadata file (e.g., /home/rahul/project/.vcs/versions.meta)
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s", vcs_path, METADATA_FILE);   // Constructs the path to versions.meta (METADATA_FILE is defined as "versions.meta" in vcs.h)
    
    // Opens versions.meta in write mode ("w"), creating it if it doesn’t exist or overwriting it if it does.
    // Returns a FILE* pointer or NULL if the operation fails (e.g., due to permissions)
    FILE* metadata_file = fopen(metadata_path, "w");

    //Checks if the file was opened successfully
    if (metadata_file) {    
        fprintf(metadata_file, "# VCS Metadata File\n");    // We write a comment to versions.meta to indicate it’s a VCS metadata file
        fprintf(metadata_file, "TOTAL_VERSIONS=0\n");       // Set initial total versions count (0, since no files are versioned yet)
        fclose(metadata_file);      // Closes the metadata file to ensure changes are saved
    }
    
    return 0;   // Returns 0 to indicate successful initialization
}

/*
The function load_repository loads an existing repository’s state from the specified path into a Repository structure, initializing its fields and loading metadata from versions.meta.
It takes a const char* path (the repository’s root directory) parameter and returns a Repository* (pointer to a Repository structure or NULL on failure).
*/
Repository* load_repository(const char* path) {
    Repository* repo = malloc(sizeof(Repository));  // Allocates memory for a Repository structure
    if (!repo) return NULL;     // Checks if memory allocation failed. If so, returns NULL to indicate failure.
    
    // Copy the input path into repo->base_path. base_path stores the root directory path of the repository
    strncpy(repo->base_path, path, MAX_PATH_LEN - 1);
    repo->base_path[MAX_PATH_LEN - 1] = '\0';   // Ensures the base_path is null-terminated
    repo->total_versions = 0;
    repo->version_list = NULL;
    
    // Call load_metadata (defined in metadata.c) to read metadata from versions.meta and update repo->version_list and repo->total_versions
    // If load_metadata returns non-zero (indicating failure), free the allocated repo memory and returns NULL to indicate failure
    if (load_metadata(repo) != 0) {
        free(repo);
        return NULL;
    }
    
    return repo;    // Returns the initialized Repository pointer if all steps succeed
}


/*
The function cleanup_repository frees all memory associated with a Repository structure, including its version_list linked list, to prevent memory leaks.
It takes a Repository* repo parameter and returns void (no return value).
*/
void cleanup_repository(Repository* repo) {
    if (!repo) return;  // If repo is NULL (e.g., if load_repository failed or repo was already freed), exit
    
    // Initializing current to point to the head of the version_list linked list, which contains FileVersion nodes
    FileVersion* current = repo->version_list;
    // Loops through the linked list until current is NULL (end of the list)
    while (current) {
        FileVersion* next = current->next;
        free(current);
        current = next; 
    }
    
    free(repo); // Frees the memory for the Repository structure after all FileVersion nodes are freed
}


/*
The function repository_exists checks if a VCS repository exists at the specified path by verifying the presence of the .vcs directory.
It takes a const char* path (the directory to check) and returns an int (1 if the repository exists, 0 otherwise).
*/
int repository_exists(const char* path) {
    char vcs_path[MAX_PATH_LEN];    // Array to store the path to the .vcs directory
    snprintf(vcs_path, sizeof(vcs_path), "%s/%s", path, VCS_DIR);
    
    // struct stat is a special structure defined in the <sys/stat.h> that holds details about a file or directory:
    // such as whether it exists, it's size, permissions, creation time, whether it’s a file, directory, or something else, etc.
    struct stat st;
    // Calling stat to get information about vcs_path. Returns 0 if the path exists, non-zero if it doesn’t (e.g., no .vcs directory)
    // &st is a pointer to a struct stat variable, where the result will be stored
    // S_ISDIR(st.st_mode) checks if the path is a directory (not a file or other type).
    return (stat(vcs_path, &st) == 0 && S_ISDIR(st.st_mode));   // This expression evaluates to 1 (true) if .vcs exists and is a directory, 0 (false) otherwise.
}