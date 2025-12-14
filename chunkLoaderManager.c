#include "chunkLoaderManager.h"

ChunkLoaderManager chunkLoaderManager;

void initChunkLoaderManager(ChunkLoaderManager *chunkLoaderManager) {
    LoadedChunks *loadedChunks    = &(chunkLoaderManager->loadedChunks);
    loadedChunks->amtLoadedChunks = 0;
    loadedChunks->capacity        = 16;                   
    loadedChunks->loadedChunks    = malloc(sizeof(Chunk)*loadedChunks->capacity);


    RenderChunks *renderChunks    = &(chunkLoaderManager->renderChunks);
    renderChunks->amtRenderChunks = 0;
    renderChunks->capacity        = 16;
    renderChunks->renderChunks    = malloc(sizeof(Chunk)*renderChunks->capacity);


    ChunksToUnload *chunksToUnload    = &(chunkLoaderManager->chunksToUnload);
    chunksToUnload->amtChunksToUnload = 0;
    chunksToUnload->capacity          = 16;
    chunksToUnload->chunksToUnload    = malloc(sizeof(Chunk)*chunksToUnload->capacity);
}   

// need hashmap for storing chunk data

void loadChunks(ChunkLoaderManager *chunkLoaderManager, GLfloat playerCoords[2]) {
    int playerChunkX = playerCoords[0] / (ChunkWidthX * BlockWidthX);
    int playerChunkZ = playerCoords[1] / (ChunkLengthZ * BlockLengthZ);

    for (int dx = -CHUNK_PRELOAD_RADIUS; dx < CHUNK_PRELOAD_RADIUS+1; dx++) {
        for (int dz = -CHUNK_PRELOAD_RADIUS; dz < CHUNK_PRELOAD_RADIUS+1; dz++) {
            int chunkX, chunkZ;
            chunkX = playerChunkX + dx; 
            chunkZ = playerChunkZ + dz; 

            // now lookup in hash table
        }
    }
}