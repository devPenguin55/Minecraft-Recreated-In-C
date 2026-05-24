#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "worldDiskStorage.h"
#include "chunkLoaderManager.h"
#include "direct.h"

void initWorldDiskStorage() {
    char path[] = "worldChunkData";
    _mkdir(path);
}

void saveChunkToDisk(Chunk *chunk) {
    if (!chunk->isDirty) { return; }

    Chunk copyChunk = *chunk;

    copyChunk.firstQuadIndex = -1;
    copyChunk.lastQuadIndex = -1;
    copyChunk.flag = CHUNK_FLAG_LOADED;
    copyChunk.hasMesh = 0;
    copyChunk.hasVertices = 0;
    copyChunk.hasWaterVertices = 0;
    copyChunk.firstVertex = -1;
    copyChunk.lastVertex = -1;
    copyChunk.firstWaterVertex = -1;
    copyChunk.lastWaterVertex = -1;
    copyChunk.triggerVertexDeletion = 0;
    copyChunk.triggerVertexRecreation = 0;

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", chunk->key);

    // printf("%" PRIu64 " %s\n", chunk->key, buffer);
    FILE *file = fopen(buffer, "wb");
    if (file == NULL) { printf("Error Saving Chunk To Disk!\n"); return; }
    fwrite(&copyChunk, sizeof(Chunk), 1, file);
    printf("Saved chunk %" PRIu64 " to disk\n", chunk->key);
    fclose(file);
}

void fetchChunkFromDisk(int chunkX, int chunkZ, Chunk *writeChunk) {
    uint64_t key = packChunkKey(chunkX, chunkZ);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "worldChunkData/%" PRIu64 ".bin", key);

    FILE *file = fopen(buffer, "rb");
    if (file == NULL) { return; } // technically if there is an error it won't show 
    // but usually if it won't open then it doesn't exist
    fread(writeChunk, sizeof(Chunk), 1, file);
    printf("Loaded chunk %" PRIu64 " from disk\n", key);
    fclose(file);
}
