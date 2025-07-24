/*
ifndef is a preprocessor directive which means 'if not defined'.
It checks whether the macro VCS_H has already been defined in the current compilation process.
If VCS_H is not defined, the preprocessor includes all the code between #ifndef VCS_H and the corresponding #endif (at the end of this file). 
If VCS_H is already defined, the preprocessor skips everything until #endif, effectively ignoring the contents of this header file.
Since multiple source files include vcs.h, and the source files are compiled together, the compiler might process vcs.h multiple times.
Without the include guard #ifndef VCS_H, this could lead to redefinition errors—where the same struct, constant, or function prototype is defined
multiple times, causing the compiler to fail.
The #ifndef VCS_H ensures that the contents of vcs.h are processed only once during compilation, preventing these errors.
*/
#ifndef VCS_H
#define VCS_H

#include <stdio.h>      // Provides standard input/output library functions
#include <stdlib.h>     // Provides memory management functions (malloc, free, exit, etc.)
#include <string.h>     // Provides string manipulation functions like strcpy, strcmp, strlen, etc., for handling file names, paths, and comments
#include <time.h>       // Provides time-related functions (time, ctime) for timestamping file versions
#include <sys/stat.h>   // Provides functions and structures for file status (e.g., stat to check file size, permissions, etc.)
#include <dirent.h>     // Provides functions for directory operations to manage the .vcs directory
#include <unistd.h>     // Provides POSIX system calls
#include <errno.h>      // Provides access to errno and error code macros (e.g., EEXIST) for error handling after system calls 


#define MAX_PATH_LEN 512        // Maximum length for file paths
#define MAX_FILENAME_LEN 256    // Maximum length for file names
#define MAX_COMMENT_LEN 512     // Maximum length for version comments
#define MAX_HASH_LEN 64         // Maximum length for file hashes (Hashes uniquely identify file contents)
#define VCS_DIR ".vcs"          // Name of the hidden directory (.vcs) where the VCS stores versioned files and metadata
#define METADATA_FILE "versions.meta"   // Name of the file that stores metadata about all versions
#define CURRENT_FILE "current.info"     // Name of the file that tracks the current state of the repository, such as which files are being tracked

// Structure to represent a file version. It stores metadata about a specific version of a file.
typedef struct FileVersion {
    char filename[MAX_FILENAME_LEN];
    char hash[MAX_HASH_LEN];
    int version_number;
    time_t timestamp;
    char comment[MAX_COMMENT_LEN];
    long file_size;
    struct FileVersion* next;   // Pointer to the next FileVersion in linked list format. So, multiple versions of a file are chained together.
} FileVersion;

// Structure to represent repository state. It manages the overall state of the VCS.
typedef struct Repository {
    char base_path[MAX_PATH_LEN];   // Stores the root directory path of the repository
    int total_versions;             // Tracks the total number of versions across all files in the repository
    FileVersion* version_list;      // Pointer to the head of a linked list of FileVersion structures, representing all versions of all files in the repository
} Repository;

// Function declarations

// Repository management (repo.c)
int init_repository(const char* path);          // Initializes a new VCS repository at the specified path
Repository* load_repository(const char* path);  // Loads an existing repository from the specified path
void cleanup_repository(Repository* repo);      // Frees memory allocated for the Repository structure and its associated FileVersion linked list
int repository_exists(const char* path);        // Checks if a VCS repository exists at the specified path

// File operations (fileops.c)  
char* generate_file_hash(const char* filepath);                 // Generates a hash (djb2) for the file at filepath
int copy_file(const char* source, const char* dest);            // Copies a file from source to dest within the .vcs directory
int create_version_file(const char* filepath, int version);     // Creates a versioned copy of a file in the .vcs directory
int restore_version_file(const char* filename, int version);    // Restores a specific version of a file to the working directory

// Version management (version.c)
int checkin_file(Repository* repo, const char* filename, const char* comment);  // Commits a new version of a file to the repository
int checkout_file(Repository* repo, const char* filename, int version);         // Retrieves a specific version of a file from the repository to the working directory
int list_versions(Repository* repo, const char* filename);                      // Lists all versions of a file, including version numbers, timestamps, and comments
int rollback_to_version(Repository* repo, const char* filename, int version);   // Reverts a file to a specific version, potentially discarding newer versions

// Metadata operations (metadata.c)
int save_metadata(Repository* repo);    // Saves the repository's metadata to versions.meta
int load_metadata(Repository* repo);    // Loads metadata from versions.meta into the Repository structure
FileVersion* find_file_version(Repository* repo, const char* filename, int version);    // Finds a specific version of a file in the repository
int get_latest_version(Repository* repo, const char* filename);     // Retrieves the latest version number for a file

// Utility functions (utils.c)
int create_directory(const char* path);    // Creates a directory at the specified path
int file_exists(const char* filepath);      // Checks if a file exists at the specified filepath
void print_timestamp(time_t timestamp);     // Prints a time_t timestamp in readable format
void print_help();                          // Prints usage instructions or help text for the VCS program

/*
Marks the end of the include guard started by #ifndef VCS_H.
If VCS_H was already defined, the preprocessor skips everything between #ifndef and this line.
*/
#endif