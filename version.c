#include "vcs.h"

/*
The function checkin_file creates a new version of a file in the VCS repository.
Records file metadata, creates a physical copy, and updates the repository's version list.
Returns the new version number on success, -1 on failure.
It takes Repository* repo (the VCS repository), const char* filename (file to check in),
and const char* comment (user comment describing the changes).
This is the primary function for adding new file versions to version control.
*/
int checkin_file(Repository* repo, const char* filename, const char* comment) {
    // Validate input parameters - both repo and filename must be non-NULL
    if (!repo || !filename) return -1;
    
    // Get next version number for this file by finding the latest version and incrementing
    int next_version = get_latest_version(repo, filename) + 1;
    
    // Create physical version file in the VCS directory structure
    // This copies the current file to .vcs/versions/filename/vN
    if (create_version_file(filename, next_version) != 0) {
        return -1;  // Return error if file creation fails
    }
    
    // Allocate memory for new version metadata entry
    FileVersion* new_version = malloc(sizeof(FileVersion));
    if (!new_version) return -1;    // Return error if memory allocation fails
    
    // Copy filename to the version entry, ensuring null termination
    strncpy(new_version->filename, filename, MAX_FILENAME_LEN - 1);
    new_version->filename[MAX_FILENAME_LEN - 1] = '\0';
    
    // Generate and store the file hash for integrity checking
    char* hash = generate_file_hash(filename);
    if (hash) {
        // Copy the generated hash, ensuring null termination
        strncpy(new_version->hash, hash, MAX_HASH_LEN - 1);
        new_version->hash[MAX_HASH_LEN - 1] = '\0';
        free(hash);     // Free the dynamically allocated hash string
    } else {
        // If hash generation fails, use a default value
        strcpy(new_version->hash, "unknown");
    }
    
    // Set the version number for this entry
    new_version->version_number = next_version;
    // Record the current timestamp when this version was created
    new_version->timestamp = time(NULL);
    
    // Copy the user's comment, ensuring null termination
    strncpy(new_version->comment, comment, MAX_COMMENT_LEN - 1);
    new_version->comment[MAX_COMMENT_LEN - 1] = '\0';
    
    // Get and store the file size using stat system call
    struct stat st;
    if (stat(filename, &st) == 0) {
        new_version->file_size = st.st_size;    // Store actual file size
    } else {
        new_version->file_size = 0;             // Default to 0 if stat fails
    }
    
    // Add the new version to the front of the repository's linked list
    new_version->next = repo->version_list;     // Point to current head
    repo->version_list = new_version;          // Make this the new head
    repo->total_versions++;                    // Increment total version count
    
    // Persist the updated metadata to disk
    save_metadata(repo);
    
    // Return the new version number to indicate success
    return next_version;
}

/*
The function checkout_file restores a specific version of a file from the VCS to the working directory.
Finds the requested version and restores it, overwriting the current file.
Returns 0 on success, -1 on failure (version not found or restore failed).
It takes Repository* repo (the VCS repository), const char* filename (file to checkout),
and int version (specific version number to restore).
This allows users to retrieve any previously checked-in version of a file.
*/
int checkout_file(Repository* repo, const char* filename, int version) {
    // Validate input parameters
    if (!repo || !filename) return -1;
    
    // Search for the specific version in the repository's version list
    FileVersion* file_version = find_file_version(repo, filename, version);
    if (!file_version) {
        return -1;  // Return error if the requested version doesn't exist
    }
    
    // Restore the file from the VCS storage to the working directory
    // This copies .vcs/versions/filename/vN back to filename
    return restore_version_file(filename, version);
}

/*
The function list_versions displays all versions of a specific file in a formatted table.
Shows version numbers, timestamps, file sizes, hashes, and comments for each version.
Returns 0 if versions found and displayed, -1 if no versions exist.
It takes Repository* repo (the VCS repository) and const char* filename (file to list versions for).
Provides a comprehensive view of a file's version history for users.
*/
int list_versions(Repository* repo, const char* filename) {
    // Validate input parameters
    if (!repo || !filename) return -1;
    
    // Print header information and table column headers
    printf("\nVersions for file: %s\n", filename);
    printf("%-8s %-20s %-10s %-12s %s\n", "Version", "Timestamp", "Size", "Hash", "Comment");
    printf("%-8s %-20s %-10s %-12s %s\n", "-------", "----------", "----", "----", "-------");
    
    // Start traversing the repository's version list from the head
    FileVersion* current = repo->version_list;
    int found = 0;  // Flag to track if any versions were found
    
    // Iterate through all versions in the repository
    while (current) {
        // Check if this version entry matches the requested filename
        if (strcmp(current->filename, filename) == 0) {
            // Print version number in 8-character field
            printf("%-8d ", current->version_number);
            
            // Format and print timestamp in human-readable format
            char time_str[20];
            struct tm* tm_info = localtime(&current->timestamp);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
            printf("%-20s ", time_str);
            
            // Print file size in 10-character field
            printf("%-10ld ", current->file_size);
            // Print first 12 characters of hash for identification
            printf("%-12.12s ", current->hash);
            // Print the full comment
            printf("%s\n", current->comment);
            
            found = 1;  // Mark that we found at least one version
        }
        current = current->next;    // Move to next version in the list
    }
    
    // If no versions were found for this file, inform the user
    if (!found) {
        printf("No versions found.\n");
        return -1;
    }
    
    return 0;   // Return success
}

/*
The function rollback_to_version restores a file to a previous version and creates a new version entry.
This is different from checkout as it creates a permanent new version record of the rollback.
Returns 0 on success, -1 on failure (version not found or rollback failed).
It takes Repository* repo (the VCS repository), const char* filename (file to rollback),
and int version (version number to rollback to).
Creates a new version entry with an automatic comment indicating the rollback operation.
*/
int rollback_to_version(Repository* repo, const char* filename, int version) {
    // Validate input parameters
    if (!repo || !filename) return -1;
    
    // Verify that the target version exists in the repository
    FileVersion* file_version = find_file_version(repo, filename, version);
    if (!file_version) {
        printf("Version %d not found for file %s\n", version, filename);
        return -1;
    }
    
    // Restore the file from the VCS storage to the working directory
    if (restore_version_file(filename, version) != 0) {
        return -1;  // Return error if file restoration fails
    }
    
    // Create an automatic comment for the rollback operation
    char rollback_comment[MAX_COMMENT_LEN];
    snprintf(rollback_comment, sizeof(rollback_comment), 
             "Rollback to version %d", version);
    
    // Create a new version entry for the rollback
    // This ensures the rollback is permanently recorded in version history
    int new_version = checkin_file(repo, filename, rollback_comment);
    
    // Return success (0) if new version was created, error (-1) otherwise
    return (new_version > 0) ? 0 : -1;
}