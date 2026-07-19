#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include "worldDiskStorage.h"
#include "chunkLoaderManager.h"
#include "direct.h"

int remove_directory(const char *path) {
    // credit to https://www.codegenes.net/blog/removing-a-non-empty-directory-programmatically-in-c-or-c/ for this code
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir failed");
        return -1;
    }
 
    struct dirent *entry;
    int result = 0; // 0 = success, -1 = failure
 
    // Read all entries in the directory
    while ((entry = readdir(dir)) != NULL && !result) {
        // Skip "." and ".." to avoid recursion loops
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
 
        // Build full path to the entry (e.g., "dir/file.txt")
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (strlen(full_path) >= PATH_MAX) {
            errno = ENAMETOOLONG;
            result = -1;
            break;
        }
 
        // Get entry metadata to check type (file/directory)
        struct stat entry_stat;
        if (stat(full_path, &entry_stat) == -1) {
            perror("stat failed");
            result = -1;
            break;
        }
 
        // Delete the entry
        if (S_ISDIR(entry_stat.st_mode)) {
            // Recursively delete subdirectories
            result = remove_directory(full_path);
        } else {
            // Delete regular files, symlinks, etc.
            if (unlink(full_path) == -1) {
                perror("unlink failed");
                result = -1;
            }
        }
    }
 
    closedir(dir); // Close the directory handle
 
    // If all contents were deleted, remove the now-empty directory
    if (!result && rmdir(path) == -1) {
        perror("rmdir failed");
        result = -1;
    }
 
    return result;
}

void initWorldDiskStorage() {
    const char *path = "worldChunkData";
    _mkdir(path);
    remove_directory(path);
    _mkdir(path);
}

void saveChunkToDisk(Chunk *chunk) {
    if (!chunk->isDirty) { return; }


    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", chunk->key);

    FILE *file = fopen(buffer, "wb");
    if (file == NULL) { printf("Error Saving Chunk To Disk!\n"); return; }

    int voxelCount = ChunkWidthX * ChunkLengthZ * ChunkHeightY;
    fwrite(chunk->blocks, sizeof(Block), voxelCount, file);

    fclose(file);
    chunk->isDirty = 0;
}

void fetchChunkFromDisk(int chunkX, int chunkZ, Chunk *writeChunk) {
    uint64_t key = packChunkKey(chunkX, chunkZ);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", key);

    FILE *file = fopen(buffer, "rb");
    if (file == NULL) { 
        writeChunk->isInitialLightCreated = 0; // this means that the chunk is to be created for the first time
        return; 
    }


    int voxelCount = ChunkWidthX * ChunkLengthZ * ChunkHeightY;
    fread(writeChunk->blocks, sizeof(Block), voxelCount, file);

    writeChunk->isInitialLightCreated = 0;
    fclose(file); 
}