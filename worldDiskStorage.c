#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "worldDiskStorage.h"
#include "chunkLoaderManager.h"
#include "direct.h"

void initWorldDiskStorage() {
    char path[] = "worldChunkData";
    _rmdir(path);
    // _mkdir(path);
}

void saveChunkToDisk(Chunk *chunk) {
    if (!chunk->isDirty) { return; }

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", chunk->key);

    FILE *file = fopen(buffer, "wb");
    if (file == NULL) { printf("Error Saving Chunk To Disk!\n"); return; }

    int voxelCount = ChunkWidthX * ChunkLengthZ * ChunkHeightY;
    fwrite(chunk->blocks, sizeof(Block), voxelCount, file);
    fwrite(chunk->lightData, sizeof(uint8_t), voxelCount, file);

    fclose(file);
    chunk->isDirty = 0;
}

void fetchChunkFromDisk(int chunkX, int chunkZ, Chunk *writeChunk) {
    uint64_t key = packChunkKey(chunkX, chunkZ);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", key);

    FILE *file = fopen(buffer, "rb");
    if (file == NULL) { return; }

    int voxelCount = ChunkWidthX * ChunkLengthZ * ChunkHeightY;
    fread(writeChunk->blocks, sizeof(Block), voxelCount, file);
    fread(writeChunk->lightData, sizeof(uint8_t), voxelCount, file);

    writeChunk->isInitialLightCreated = 0;
    fclose(file); 
}