#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "chunkLoaderManager.h"

ChunkLoaderManager chunkLoaderManager;

void initChunkLoaderManager() { 
    LoadedChunks *loadedChunks    = &(chunkLoaderManager.loadedChunks);
    loadedChunks->amtLoadedChunks = 0;
    loadedChunks->capacity        = 16;                   
    loadedChunks->loadedChunks    = malloc(loadedChunks->capacity*sizeof(Chunk *));


    RenderChunks *renderChunks    = &(chunkLoaderManager.renderChunks);
    renderChunks->amtRenderChunks = 0;
    renderChunks->capacity        = 16;
    renderChunks->renderChunks    = malloc(renderChunks->capacity*sizeof(Chunk *));


    ChunksToUnload *chunksToUnload    = &(chunkLoaderManager.chunksToUnload);
    chunksToUnload->amtChunksToUnload = 0;
    chunksToUnload->capacity          = 16;
    chunksToUnload->chunksToUnload    = malloc(chunksToUnload->capacity*sizeof(Chunk *));

    for (int i = 0; i < HASHMAP_AMOUNT_BUCKETS; i++) {
        chunkLoaderManager.hashmap.buckets[i].head = NULL;
    }
}   

// bucket->head = malloc(sizeof(BucketEntry));
// bucket->head->exists     = 0;
// bucket->head->key        = NULL;
// bucket->head->chunkEntry = NULL;
// bucket->head->next       = NULL;

uint64_t packChunkKey(int64_t chunkX, int64_t chunkZ){
    return ((uint64_t)chunkX << 32) | ((uint64_t)chunkZ & 0xFFFFFFFF);
}


void unpackChunkKey(uint64_t key, int64_t* chunkX, int64_t* chunkZ){
    *chunkX = (int32_t)(key >> 32);
    *chunkZ = (int32_t)(key & 0xFFFFFFFF);
}
   
BucketEntry *getHashmapEntry(uint64_t key) {
    Bucket *bucket = &(chunkLoaderManager.hashmap.buckets[key % HASHMAP_AMOUNT_BUCKETS]);
 
    // traverse the linked list
    BucketEntry *currentNode = bucket->head;
    while (currentNode != NULL) {
        if (currentNode->key == key) {
            if (currentNode->chunkEntry == NULL) {
            }
            return currentNode;
        }
        currentNode = currentNode->next;
    }
   
    return NULL; // not found 
}

void writeHashmapEntry(uint64_t key, int chunkX, int chunkZ, int exists) {
    Bucket *bucket = &(chunkLoaderManager.hashmap.buckets[key % HASHMAP_AMOUNT_BUCKETS]);

    if (bucket->head == NULL) {
        bucket->head = malloc(sizeof(BucketEntry));
        bucket->head->key = key;
        bucket->head->chunkEntry = malloc(sizeof(Chunk)); // allocate the chunk
        createChunk(bucket->head->chunkEntry, chunkX * (ChunkWidthX * BlockWidthX), chunkZ * (ChunkLengthZ * BlockLengthZ), 1, CHUNK_FLAG_LOADED, key);
        bucket->head->exists = exists;
        bucket->head->next = NULL;
        return;
    }   

    BucketEntry *currentNode = bucket->head;
    while (currentNode != NULL) {
        if (currentNode->key == key) {
            return;
        }
        if (currentNode->next == NULL) {
            break; 
        }
        currentNode = currentNode->next;
    }
    currentNode->next = malloc(sizeof(BucketEntry));
    currentNode = currentNode->next; // move to the new node
    currentNode->key = key;
    currentNode->chunkEntry = malloc(sizeof(Chunk)); // allocate the chunk
    createChunk(currentNode->chunkEntry, chunkX * ChunkWidthX, chunkZ * ChunkLengthZ, 1, CHUNK_FLAG_LOADED, key);
    currentNode->exists = exists;
    currentNode->next = NULL;
}

  
void deleteHashmapEntry(uint64_t key) {
    Bucket *bucket = &(chunkLoaderManager.hashmap.buckets[key % HASHMAP_AMOUNT_BUCKETS]);

    // traverse the linked list
    BucketEntry *currentNode = bucket->head;
    BucketEntry *prevNode    = NULL;
    while (currentNode != NULL) {
        if (currentNode->key == key) {
            break;
        }
        prevNode    = currentNode;
        currentNode = currentNode->next;
    }
    
    if (currentNode == NULL) return; // key not found in bucket
    
    if (prevNode == NULL) {
        bucket->head = currentNode->next;
    } else {
        prevNode->next = currentNode->next;
    }
    free(currentNode->chunkEntry);
    free(currentNode);
}

void loadChunks(GLfloat playerCoords[2]) {
    int playerChunkX = (int)floor(playerCoords[0] / (ChunkWidthX * BlockWidthX));
    int playerChunkZ = (int)floor(playerCoords[1] / (ChunkLengthZ * BlockLengthZ));

    
    // preload radius -> get all of the chunk data loaded first
    // also find the chunks to render and allocate those for the next mesh loop
    LoadedChunks *loadedChunks        = &(chunkLoaderManager.loadedChunks);
    RenderChunks *renderChunks        = &(chunkLoaderManager.renderChunks);
    ChunksToUnload *chunksToUnload    = &(chunkLoaderManager.chunksToUnload);
    renderChunks->amtRenderChunks     = 0;
    chunksToUnload->amtChunksToUnload = 0;

    int alreadyPreloadedOnce = 0;
    for (int dx = -CHUNK_PRELOAD_RADIUS; dx < CHUNK_PRELOAD_RADIUS+1; dx++) {
        for (int dz = -CHUNK_PRELOAD_RADIUS; dz < CHUNK_PRELOAD_RADIUS+1; dz++) {
            int chunkX, chunkZ;
            chunkX = playerChunkX + dx; 
            chunkZ = playerChunkZ + dz; 

            uint64_t chunkKey  = packChunkKey(chunkX, chunkZ);
            BucketEntry *result = getHashmapEntry(chunkKey);
            if (result == NULL && !alreadyPreloadedOnce) {
                alreadyPreloadedOnce = 1;
                // if (chunkX == -1 && chunkZ == 0) {printf("Loading chunks for %d %d %d %d!\n", chunkX, chunkZ, dx, dz);}
                // * the chunk has not been loaded yet, must load it in and add to the hashmap
                // ? chunks that are loaded are just skipped since their data is already there
                writeHashmapEntry(chunkKey, chunkX, chunkZ, 1);
                // if (chunkX == -1 && chunkZ == 0) {printf("Done loading chunks!\n");}
                result = getHashmapEntry(chunkKey);

                if (loadedChunks->amtLoadedChunks >= loadedChunks->capacity) {
                    loadedChunks->capacity *= 2;
                    loadedChunks->loadedChunks = realloc(loadedChunks->loadedChunks, loadedChunks->capacity*sizeof(Chunk *));
                }
    
                loadedChunks->loadedChunks[(loadedChunks->amtLoadedChunks)++] = result->chunkEntry;
            }

            if (alreadyPreloadedOnce) { break; }
 
            // see if a chunk is in the render radius
            if ((dx <= CHUNK_RENDER_RADIUS && dx >= -CHUNK_RENDER_RADIUS) && (dz <= CHUNK_RENDER_RADIUS && dz >= -CHUNK_RENDER_RADIUS)) {
                if (result->chunkEntry->flag != CHUNK_FLAG_RENDERED_AND_LOADED) { alreadyPreloadedOnce = 1; }
                // if (chunkX == -1 && chunkZ == 0) {printf("rendering chunks for %d %d %d %d!\n", chunkX, chunkZ, dx, dz);printf("chunkKey = %" PRIu64 "\n", chunkKey);}
                // if it is, allocate it
                if (renderChunks->amtRenderChunks >= renderChunks->capacity) {
                    renderChunks->capacity *= 2;
                    renderChunks->renderChunks = realloc(renderChunks->renderChunks, renderChunks->capacity*sizeof(Chunk *));
                }

                renderChunks->renderChunks[(renderChunks->amtRenderChunks)++] = result->chunkEntry;
            }
            
        }
    }

    for (int renderChunkIdx = 0; renderChunkIdx<(renderChunks->amtRenderChunks); renderChunkIdx++) {
        Chunk *curChunkToRender = renderChunks->renderChunks[renderChunkIdx];
        if (curChunkToRender->flag == CHUNK_FLAG_RENDERED_AND_LOADED) { continue; }
        curChunkToRender->hasMesh = 1;
        generateChunkMesh(curChunkToRender);
    }        

    int64_t loadedChunkX, loadedChunkZ;
    for (int loadedChunkIdx = 0; loadedChunkIdx < loadedChunks->amtLoadedChunks; loadedChunkIdx++) {
        Chunk *curChunk = loadedChunks->loadedChunks[loadedChunkIdx];
        unpackChunkKey(curChunk->key, &loadedChunkX, &loadedChunkZ);

        if ( 
            (loadedChunkX > (playerChunkX + CHUNK_RENDER_RADIUS) || (loadedChunkX < (playerChunkX - CHUNK_RENDER_RADIUS))) ||
            (loadedChunkZ > (playerChunkZ + CHUNK_RENDER_RADIUS) || (loadedChunkZ < (playerChunkZ - CHUNK_RENDER_RADIUS)))) {
            if (curChunk->hasMesh) {
                // printf("deleting a chunk mesh!\n");
                // printf("chunk coordinates are at %d %d\n", loadedChunkX, loadedChunkZ);
                deleteChunkMesh(curChunk);
            }
        }


        if ((loadedChunkX > (playerChunkX + CHUNK_PRELOAD_RADIUS)) || (loadedChunkX < (playerChunkX - CHUNK_PRELOAD_RADIUS)) ||
            (loadedChunkZ > (playerChunkZ + CHUNK_PRELOAD_RADIUS)) || (loadedChunkZ < (playerChunkZ - CHUNK_PRELOAD_RADIUS))) {

            if (chunksToUnload->amtChunksToUnload >= chunksToUnload->capacity) {
                chunksToUnload->capacity *= 2;
                chunksToUnload->chunksToUnload = realloc(chunksToUnload->chunksToUnload, chunksToUnload->capacity*sizeof(Chunk *));
            } 
            
            chunksToUnload->chunksToUnload[chunksToUnload->amtChunksToUnload++] = curChunk;
        }
    }

    for (int chunkIdx = 0; chunkIdx < chunksToUnload->amtChunksToUnload; chunkIdx++) {
        Chunk *curChunk = chunksToUnload->chunksToUnload[chunkIdx];
        for (int loadedChunkIdx = 0; loadedChunkIdx < loadedChunks->amtLoadedChunks; loadedChunkIdx++) {
            if (curChunk == loadedChunks->loadedChunks[loadedChunkIdx]) {
                // this is a match of pointers to a chunk
                deleteHashmapEntry(curChunk->key); 
                loadedChunks->loadedChunks[loadedChunkIdx] = loadedChunks->loadedChunks[loadedChunks->amtLoadedChunks-1];
                loadedChunks->amtLoadedChunks--;
                break;
            }
        }       
    }
}

void triggerRenderChunkRebuild (Chunk *chunk) {
    if (chunk->hasMesh) {
        deleteChunkMesh(chunk);   
    }
}