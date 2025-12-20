#ifndef CHUNK_LOADER_MANAGER_H
#define CHUNK_LOADER_MANAGER_H
#include <stdint.h>
#include "chunks.h"

#define CHUNK_PRELOAD_RADIUS 2
#define CHUNK_RENDER_RADIUS  1

#define HASHMAP_AMOUNT_BUCKETS 2048

typedef struct LoadedChunks {
    Chunk **loadedChunks;
    int amtLoadedChunks;
    int        capacity;
} LoadedChunks;

typedef struct RenderChunks {
    Chunk **renderChunks;
    int amtRenderChunks;
    int        capacity;
} RenderChunks;

typedef struct ChunksToUnload {
    Chunk **chunksToUnload;
    int amtChunksToUnload;
    int          capacity;
} ChunksToUnload;

typedef struct RenderChunksToUnload {
    Chunk **renderChunksToUnload;
    int  amtRenderChunksToUnload;
    int                 capacity;
} RenderChunksToUnload;

typedef struct BucketEntry {
    uint64_t             key;
    Chunk        *chunkEntry;
    int               exists;
    struct BucketEntry *next;
} BucketEntry; 

typedef struct Bucket {
    BucketEntry *head;
} Bucket;

typedef struct Hashmap {
    Bucket buckets[HASHMAP_AMOUNT_BUCKETS];
} Hashmap;

typedef struct ChunkLoaderManager {
    LoadedChunks                 loadedChunks;
    RenderChunks                 renderChunks;
    ChunksToUnload             chunksToUnload;
    RenderChunksToUnload renderChunksToUnload;
    Hashmap                           hashmap;
} ChunkLoaderManager;

extern ChunkLoaderManager chunkLoaderManager;

void initChunkLoaderManager();
uint64_t packChunkKey(int64_t chunkX, int64_t chunkZ);
BucketEntry *getHashmapEntry(uint64_t key);
void unpackChunkKey(uint64_t key, int64_t* chunkX, int64_t* chunkZ);
void writeHashmapEntry(uint64_t key, int chunkX, int chunkZ, int exists);
void deleteHashmapEntry(uint64_t key);
void loadChunks(GLfloat playerCoords[2]);

#endif