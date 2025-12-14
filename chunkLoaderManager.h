#ifndef CHUNK_LOADER_MANAGER_H
#define CHUNK_LOADER_MANAGER_H
#include "chunks.h"

#define CHUNK_PRELOAD_RADIUS 4
#define CHUNK_RENDER_RADIUS  2


typedef struct LoadedChunks {
    Chunk *loadedChunks;
    int amtLoadedChunks;
    int capacity;
} LoadedChunks;

typedef struct RenderChunks {
    Chunk *renderChunks;
    int amtRenderChunks;
    int capacity;
} RenderChunks;

typedef struct ChunksToUnload {
    Chunk *chunksToUnload;
    int amtChunksToUnload;
    int capacity;
} ChunksToUnload;

typedef struct HashmapValue {
    Chunk *chunkEntry;
    int exists;
} HashmapValue; 

typedef struct ChunkLoaderManager {
    LoadedChunks   loadedChunks;
    RenderChunks   renderChunks;
    ChunksToUnload chunksToUnload;

} ChunkLoaderManager;

extern ChunkLoaderManager chunkLoaderManager;

void initChunkLoaderManager(ChunkLoaderManager *chunkLoaderManager);
void loadChunks(ChunkLoaderManager *chunkLoaderManager, GLfloat playerCoords[2]);

#endif