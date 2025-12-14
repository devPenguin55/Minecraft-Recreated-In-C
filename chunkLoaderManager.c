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

void writeHashmapEntry(uint64_t key, Chunk *chunk, int exists) {
    Bucket *bucket = &(chunkLoaderManager.hashmap.buckets[key % HASHMAP_AMOUNT_BUCKETS]);

    // traverse the linked list and gets the last value
    BucketEntry *currentNode = bucket->head;
    while (currentNode->next != NULL) {
        if (currentNode->key == key) {
            // no duplicates!
            return;
        }
        currentNode = currentNode->next;
    }

    if (currentNode->key == key) {
        // no duplicates for the last entry too!
        return;
    }

    currentNode->next = malloc(sizeof(BucketEntry));
    currentNode->next->key        = key;
    currentNode->next->chunkEntry = chunk;
    currentNode->next->exists     = exists;
    currentNode->next->next       = NULL;
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
}

void loadChunks(GLfloat playerCoords[2]) {
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