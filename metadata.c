#include "vcs.h"

/*
The function save_metadata writes the repository's version metadata to versions.meta.
Saves all file versions, their metadata, and repository statistics to disk for later retrieval.
Returns 0 on success, -1 on failure (file write errors).
It takes Repository* repo (the VCS repository to save metadata for).
Creates a human-readable metadata file for easy parsing. This ensures persistence across program runs.
*/
int save_metadata(Repository* repo) {
    // Validate input parameter
    if (!repo) return -1;
    
    // Construct the full path to the metadata file within the VCS directory
    char metadata_path[MAX_PATH_LEN];   // Array to store the path to versions.meta
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s/%s", 
             repo->base_path, VCS_DIR, METADATA_FILE);
    
    // Open metadata file for writing (creates new file or overwrites existing)
    FILE* file = fopen(metadata_path, "w");
    if (!file) return -1;   // Return error if file cannot be opened for writing
    
    // Write header comment and total version count for quick repository statistics
    fprintf(file, "# VCS Metadata File\n");
    fprintf(file, "TOTAL_VERSIONS=%d\n", repo->total_versions);
    fprintf(file, "\n# File Versions\n");
    
    // Traverse the repository's version list and write each version's metadata
    FileVersion* current = repo->version_list;
    while (current) {
        // Write version metadata for easy parsing
        // Format: FILE=name|VERSION=num|TIMESTAMP=time|SIZE=bytes|HASH=sha256|COMMENT=text
        fprintf(file, "FILE=%s|VERSION=%d|TIMESTAMP=%ld|SIZE=%ld|HASH=%s|COMMENT=%s\n",
                current->filename,          // Name of the versioned file
                current->version_number,    // Version number (1, 2, 3, etc.)
                current->timestamp,         // Unix timestamp when version was created
                current->file_size,         // File size in bytes
                current->hash,              // SHA-256 hash for integrity verification
                current->comment);          // User comment describing the changes
        current = current->next;            // Move to next version in linked list
    }
    
    // Close the file to ensure all data is written to disk
    fclose(file);
    return 0;   // Return success
}

/*
The function load_metadata reads repository metadata from disk and rebuilds the in-memory version list.
Parses the metadata file and reconstructs FileVersion entries in the repository's linked list.
Returns 0 on success (including when no metadata file exists), -1 on failure.
It takes Repository* repo (the VCS repository to load metadata into).
Handles both repository statistics and individual file version records.
This restores version history when the VCS program starts up.
*/
int load_metadata(Repository* repo) {
    // Validate input parameter
    if (!repo) return -1;
    
    // Construct the full path to the metadata file
    char metadata_path[MAX_PATH_LEN];       // Array for the versions.meta path
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s/%s", 
             repo->base_path, VCS_DIR, METADATA_FILE);
    
    // Open metadata file for reading
    FILE* file = fopen(metadata_path, "r");
    if (!file) return 0; // Return success if no metadata file exists yet (new repository)
    
    // Buffer for reading lines from the metadata file versions.meta
    char line[1024];
    
    // Read the metadata file line by line
    while (fgets(line, sizeof(line), file)) {
        // Skip comment lines (starting with #) and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse total versions count line. Checks if the line starts with "TOTAL_VERSIONS="" using strncmp()
        if (strncmp(line, "TOTAL_VERSIONS=", 15) == 0) {
            // Converts the string after TOTAL_VERSIONS= to an integer using atoi, storing it in repo->total_versions
            repo->total_versions = atoi(line + 15);
        }
        // Parse file version record lines. Checks if the line starts with "FILE=", indicating a version record
        else if (strncmp(line, "FILE=", 5) == 0) {
            // Allocate memory for new version entry
            FileVersion* version = malloc(sizeof(FileVersion));
            if (!version) continue;     // Skip this entry if allocation fails
            
            // Parse filename field (first token after FILE=)
            char* token = strtok(line + 5, "|");    // Skip "FILE=" and split by |
            if (token) {
                strncpy(version->filename, token, MAX_FILENAME_LEN - 1);
                version->filename[MAX_FILENAME_LEN - 1] = '\0';     // Ensure null termination
            }
            
            // Parse version number field
            token = strtok(NULL, "|");      // Get next token
            if (token && strncmp(token, "VERSION=", 8) == 0) {
                version->version_number = atoi(token + 8);  // Skip "VERSION=" and convert
            }
            
            // Parse timestamp field
            token = strtok(NULL, "|");
            if (token && strncmp(token, "TIMESTAMP=", 10) == 0) {
                version->timestamp = atol(token + 10);      // Skip "TIMESTAMP=" and convert to long
            }
            
            // Parse file size field
            token = strtok(NULL, "|");
            if (token && strncmp(token, "SIZE=", 5) == 0) {
                version->file_size = atol(token + 5);       // Skip "SIZE=" and convert to long
            }
            
            // Parse hash field
            token = strtok(NULL, "|");
            if (token && strncmp(token, "HASH=", 5) == 0) {
                strncpy(version->hash, token + 5, MAX_HASH_LEN - 1);
                version->hash[MAX_HASH_LEN - 1] = '\0';     // Ensure null termination
            }
            
            // Parse comment field (last field, may contain the line's newline)
            token = strtok(NULL, "|");
            if (token && strncmp(token, "COMMENT=", 8) == 0) {
                strncpy(version->comment, token + 8, MAX_COMMENT_LEN - 1);
                version->comment[MAX_COMMENT_LEN - 1] = '\0';
                
                // Remove trailing newline character if present
                char* newline = strchr(version->comment, '\n');
                if (newline) *newline = '\0';
            }
            
            // Add the loaded version to the front of the repository's linked list
            version->next = repo->version_list;     // Point to current head
            repo->version_list = version;          // Make this the new head
        }
    }
    
    // Close the metadata file
    fclose(file);
    return 0;   // Return success
}

/*
The function find_file_version searches for a specific version of a file in the repository.
Traverses the version list to find an exact match for both filename and version number.
Returns pointer to the FileVersion if found, NULL if not found or on error.
It takes Repository* repo (the VCS repository to search), const char* filename (file to find),
and int version (specific version number to locate).
Used by checkout, rollback, and other operations that need to verify version existence.
*/
FileVersion* find_file_version(Repository* repo, const char* filename, int version) {
    // Validate input parameters
    if (!repo || !filename) return NULL;
    
    // Start traversing from the head of the repository's version list
    FileVersion* current = repo->version_list;
    while (current) {
        // Check if both filename and version number match exactly
        if (strcmp(current->filename, filename) == 0 && 
            current->version_number == version) {
            return current;     // Return pointer to the matching version
        }
        current = current->next;    // Move to next version in the list
    }
    
    // Return NULL if no matching version was found
    return NULL;
}

/*
The function get_latest_version finds the highest version number for a specific file.
Searches through all versions of a file to determine the most recent version number.
Returns the latest version number (0 if no versions exist), always >= 0.
It takes Repository* repo (the VCS repository to search) and const char* filename (file to check).
Used when checking in new versions to determine the next version number.
Essential for maintaining sequential version numbering.
*/
int get_latest_version(Repository* repo, const char* filename) {
    // Validate input parameters
    if (!repo || !filename) return 0;
    
    // Initialize latest version counter
    int latest = 0;
    FileVersion* current = repo->version_list;
    
    // Traverse all versions in the repository
    while (current) {
        // Check if this version belongs to the requested file
        if (strcmp(current->filename, filename) == 0) {
            // Update latest if this version number is higher
            if (current->version_number > latest) {
                latest = current->version_number;
            }
        }
        current = current->next;    // Move to next version in the list
    }
    
    // Return the highest version number found (0 if no versions exist)
    return latest;
}