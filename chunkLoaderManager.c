#include <stdio.h>
#include "chunkLoaderManager.h"

ChunkLoaderManager chunkLoaderManager;

void initChunkLoaderManager() {
    LoadedChunks *loadedChunks    = &(chunkLoaderManager.loadedChunks);
    loadedChunks->amtLoadedChunks = 0;
    loadedChunks->capacity        = 16;                   
    loadedChunks->loadedChunks    = malloc(sizeof(Chunk)*loadedChunks->capacity);


    RenderChunks *renderChunks    = &(chunkLoaderManager.renderChunks);
    renderChunks->amtRenderChunks = 0;
    renderChunks->capacity        = 16;
    renderChunks->renderChunks    = malloc(sizeof(Chunk)*renderChunks->capacity);


    ChunksToUnload *chunksToUnload    = &(chunkLoaderManager.chunksToUnload);
    chunksToUnload->amtChunksToUnload = 0;
    chunksToUnload->capacity          = 16;
    chunksToUnload->chunksToUnload    = malloc(sizeof(Chunk)*chunksToUnload->capacity);

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
    uint64_t xPart = ((uint64_t )(uint32_t)chunkX) << 32;
    uint64_t zPart = (uint64_t )(uint32_t)chunkZ;
    return xPart | zPart;
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
        createChunk(bucket->head->chunkEntry, chunkX * ChunkWidthX, chunkZ * ChunkLengthZ, 1, CHUNK_FLAG_LOADED);
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
    createChunk(currentNode->chunkEntry, chunkX * ChunkWidthX, chunkZ * ChunkLengthZ, 1, CHUNK_FLAG_LOADED);
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
    int playerChunkX = playerCoords[0] / (ChunkWidthX * BlockWidthX);
    int playerChunkZ = playerCoords[1] / (ChunkLengthZ * BlockLengthZ);
    
    // preload radius -> get all of the chunk data loaded first
    for (int dx = -CHUNK_PRELOAD_RADIUS; dx < CHUNK_PRELOAD_RADIUS+1; dx++) {
        for (int dz = -CHUNK_PRELOAD_RADIUS; dz < CHUNK_PRELOAD_RADIUS+1; dz++) {
            int chunkX, chunkZ;
            chunkX = playerChunkX + dx; 
            chunkZ = playerChunkZ + dz; 

            uint64_t chunkKey  = packChunkKey(chunkX, chunkZ);
            BucketEntry *result = getHashmapEntry(chunkKey);
            if (result == NULL) {
                printf("Loading chunks for %d %d!\n", chunkX, chunkZ);
                // * the chunk has not been loaded yet, must load it in and add to the hashmap
                // ? chunks that are loaded are just skipped since their data is already there
                writeHashmapEntry(chunkKey, chunkX, chunkZ, 1);
                printf("Done loading chunks!\n");
            }
        }
    }
}