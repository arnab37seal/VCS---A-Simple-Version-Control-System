#include "vcs.h"

/*
The function generate_file_hash generates a unique hash (djb2) for a file based on its contents and size, 
used to detect changes and identify versions in the VCS. Returns a dynamically allocated string or NULL on failure.
It takes a const char* filepath and returns a char* (the hash string).
The hash is stored in a dynamically allocated string, up to MAX_HASH_LEN (64) characters.
*/
char* generate_file_hash(const char* filepath) {
    // Opening the file in binary read mode ("rb") to read its bytes (Binary read mode to ensure consistent reading across platforms)
    FILE* file = fopen(filepath, "rb");
    // Returns a FILE* pointer or NULL if the file can't be opened
    if (!file) return NULL;
    
    // Simple hash function based on file content and size
    unsigned long hash = 5381;
    int c;                  // Stores each byte read from the file
    
    // Reads the file byte-by-byte using fgetc(), which returns the next byte or EOF (end-of-file)
    // Loops until the end of the file
    while ((c = fgetc(file)) != EOF) {
        // Updates the hash using the djb2 algorithm: hash = (hash * 33) + c
        // hash << 5 shifts the hash left by 5 bits (equivalent to multiplying by 32), then adds hash (making it 33) and the current byte c
        hash = ((hash << 5) + hash) + c;
    }
    
    fclose(file);
    
    // Add file size to hash
    struct stat st;
    if (stat(filepath, &st) == 0) {
        hash = ((hash << 5) + hash) + st.st_size;
    }
    
    char* hash_str = malloc(MAX_HASH_LEN);
    if (hash_str) {
        snprintf(hash_str, MAX_HASH_LEN, "%08lx%ld", hash, (long)time(NULL) % 10000);
    }
    
    return hash_str;    // Returning the hash string or NULL if allocation failed
}

/*
The function copy_file copies a file from source path to destination path in binary mode.
Used internally by other VCS functions to create file copies for versioning.
Returns 0 on success, -1 on failure (file open/read/write errors).
It takes const char* source (source file path) and const char* dest (destination file path).
Reads and writes files in 4KB chunks for efficient memory usage.
*/
int copy_file(const char* source, const char* dest) {
    // Open source file in binary read mode
    FILE* src = fopen(source, "rb");
    if (!src) return -1;    // Return error if source file cannot be opened
    
    // Opens the destination file in binary write mode, creating or overwriting it
    FILE* dst = fopen(dest, "wb");
    if (!dst) {
        fclose(src);    // If fopen() fails, close source file before returning error
        return -1;
    }
    
    char buffer[4096];  // 4KB buffer for reading/writing file data
    size_t bytes;       // Variable to store the number of bytes read/written
    
    // Copy file contents in chunks until end of file
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        // Write the read bytes to destination file
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            // On write failure, close both files and returns -1
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    
    // Close both files after successful copy
    fclose(src);
    fclose(dst);
    return 0;   // Return success
}

/*
The function create_version_file creates a versioned copy of a file in the VCS directory structure.
Organizes versions by filename in separate directories under .vcs/versions/filename/vN.
Returns 0 on success, -1 on failure (directory creation or file copy errors).
It takes const char* filepath (path to the file to version) and int version (version number).
Creates necessary directory structure and copies the file to the versioned location.
*/
int create_version_file(const char* filepath, int version) {
    // Buffers for constructing various paths
    char version_dir[MAX_PATH_LEN];     // Array for the directory path for a file's versions (e.g., .vcs/versions/filename)
    char version_file[MAX_PATH_LEN];    // Array for the specific version file path (e.g., .vcs/versions/filename/vN)
    char current_dir[MAX_PATH_LEN];     // Array for current working directory path
    
    // Get the current working directory to build absolute paths
    getcwd(current_dir, sizeof(current_dir));   // Stores the current working directory in current_dir
    
    // Extract filename from the full filepath. strrchr() finds the last occurrence of the character '/' in the path.
    // Returns a pointer to the last occurrence of the character '/' in the string, or NULL if not found
    const char* filename = strrchr(filepath, '/');
    // If '/' is found, move past it to get the filename; otherwise use the entire input filepath as the filename
    filename = filename ? filename + 1 : filepath;

    // Construct the version directory path: current_dir/.vcs/versions/filename
    snprintf(version_dir, sizeof(version_dir), "%s/%s/versions/%s", current_dir, VCS_DIR, filename);
    
    // Create directory for this file's versions if it doesn't exist
    // 0755 sets permissions: owner can read/write/execute, group and others can read/execute
    if (mkdir(version_dir, 0755) != 0) {
        // Check if the error is because the directory already exists
        if (errno != EEXIST) {
            // If it's a different error (not "already exists")
            perror("Failed to create file's versions directory");
            return -1;
        }
        // If errno == EEXIST, the directory already exists, which is fine
    }
    
    // Construct the full path to this specific version file: version_dir/vN
    snprintf(version_file, sizeof(version_file), "%s/v%d", version_dir, version);
    
    // Copy the original file to the versioned location
    return copy_file(filepath, version_file);
}

/*
The function restore_version_file restores a specific version of a file from the VCS back to the working directory.
Replaces the current file with the contents of the specified version.
Returns 0 on success, -1 on failure (version doesn't exist or file copy errors).
It takes const char* filename (name of the file to restore) and int version (version number to restore).
Checks if the version exists before attempting to restore it.
*/
int restore_version_file(const char* filename, int version) {
    // Buffers for constructing paths
    char version_file[MAX_PATH_LEN];    // Array for the version file path (e.g., .vcs/versions/filename/vN)
    char current_dir[MAX_PATH_LEN];     // Array for the current working directory path
    
    // Get the current working directory to build absolute paths
    getcwd(current_dir, sizeof(current_dir));
    
    // Construct the full path to the version file: current_dir/.vcs/versions/filename/vN
    snprintf(version_file, sizeof(version_file), "%s/%s/versions/%s/v%d", 
             current_dir, VCS_DIR, filename, version);
    
    // Check if the requested version file exists
    if (!file_exists(version_file)) {
        return -1;  // Return error if version doesn't exist
    }
    
    // Copy the version file back to the working directory, overwriting the current file
    return copy_file(version_file, filename);
}