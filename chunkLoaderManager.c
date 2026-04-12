#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "chunkLoaderManager.h"
#include "render.h"


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

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
    createChunk(currentNode->chunkEntry, chunkX * ChunkWidthX * BlockWidthX, chunkZ * ChunkLengthZ * BlockLengthZ, 1, CHUNK_FLAG_LOADED, key);
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
int initialAmtLoadedChunksUntilMeshing = 0;
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
    renderChunks->capacity            = 16;
    chunksToUnload->capacity          = 16;
    
    int alreadyPreloadedOnce = 0;
    for (int dx = -CHUNK_PRELOAD_RADIUS; dx < CHUNK_PRELOAD_RADIUS+1; dx++) {
        if (alreadyPreloadedOnce) { break; }
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
                if (initialAmtLoadedChunksUntilMeshing < (2*CHUNK_PRELOAD_RADIUS+1)*(2*CHUNK_PRELOAD_RADIUS+1)) { initialAmtLoadedChunksUntilMeshing++; }
            }

            
            if (alreadyPreloadedOnce) { break; }

            if (result == NULL) continue;
            
            if (initialAmtLoadedChunksUntilMeshing < (2*CHUNK_PRELOAD_RADIUS+1)*(2*CHUNK_PRELOAD_RADIUS+1)) { continue; }
            // see if a chunk is in the render radius
            if ((dx <= CHUNK_RENDER_RADIUS && dx >= -CHUNK_RENDER_RADIUS) && (dz <= CHUNK_RENDER_RADIUS && dz >= -CHUNK_RENDER_RADIUS)) {
                if (
                    getHashmapEntry(packChunkKey(chunkX+1, chunkZ)) == NULL ||
                    getHashmapEntry(packChunkKey(chunkX-1, chunkZ)) == NULL ||
                    getHashmapEntry(packChunkKey(chunkX, chunkZ+1)) == NULL ||
                    getHashmapEntry(packChunkKey(chunkX, chunkZ-1)) == NULL
                ) {
                    continue;
                }
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
                curChunk->triggerVertexDeletion = 1;
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

    for (int chunkIdx = 0; chunkIdx < loadedChunks->amtLoadedChunks; chunkIdx++) {
        Chunk *chunk = loadedChunks->loadedChunks[chunkIdx];
        if (chunk->triggerVertexRecreation) {
            chunk->triggerVertexRecreation = 0;
            if (chunk->hasMesh) {
                deleteChunkMesh(chunk);   
                chunk->hasMesh = 0;
                


                int amtVerticesInChunk = chunk->lastVertex - chunk->firstVertex + 1;
                if (chunk->lastVertex != -1) {
                    int start = chunk->firstVertex;
                    int end = chunk->lastVertex + 1;

                    int moveCount = worldVertexCount - end;

                    if (moveCount > 0)
                    {
                        memmove(&worldVertices[start],
                                &worldVertices[end],
                                moveCount * sizeof(Vertex));
                    }
                
                    worldVertexCount -= amtVerticesInChunk;
                }

                int amtWaterVerticesInChunk = chunk->lastWaterVertex - chunk->firstWaterVertex + 1;
                if (chunk->lastWaterVertex != -1) {
                    int start = chunk->firstWaterVertex;
                    int end = chunk->lastWaterVertex + 1;

                    int moveCount = waterVertexCount - end;

                    if (moveCount > 0)
                    {
                        memmove(&waterVertices[start],
                                &waterVertices[end],
                                moveCount * sizeof(Vertex));
                    }

                    waterVertexCount -= amtWaterVerticesInChunk;
                }
            

                for (int otherChunkIdx = 0; otherChunkIdx < chunkLoaderManager.loadedChunks.amtLoadedChunks; otherChunkIdx++) {
                    Chunk *otherChunk = (chunkLoaderManager.loadedChunks.loadedChunks[otherChunkIdx]);
                    if (otherChunk->hasVertices && otherChunk->lastVertex != -1 && chunk->lastVertex != -1) {
                        if (otherChunk->firstVertex > chunk->firstVertex) {
                            otherChunk->firstVertex -= amtVerticesInChunk;
                            otherChunk->lastVertex  -= amtVerticesInChunk;
                        }
                    }
                    if (otherChunk->hasWaterVertices && otherChunk->lastWaterVertex != -1 && chunk->lastWaterVertex != -1) {
                        if (otherChunk->firstWaterVertex > chunk->firstWaterVertex) {
                            otherChunk->firstWaterVertex -= amtWaterVerticesInChunk;
                            otherChunk->lastWaterVertex  -= amtWaterVerticesInChunk;
                        }
                    }
                }

                chunk->firstVertex = -1;
                chunk->lastVertex  = -1;
                chunk->firstWaterVertex = -1;
                chunk->lastWaterVertex  = -1;
                chunk->hasVertices = 0;
                chunk->hasWaterVertices = 0;

                chunk->hasVertices = 1;
                chunk->hasWaterVertices = 0;
                chunk->triggerVertexDeletion = 0;


                // build vertices

                generateChunkMesh(chunk);
                chunk->hasMesh = 1;



                int firstQuadIndex = chunk->firstQuadIndex;
                int lastQuadIndex = chunk->lastQuadIndex;
            

                chunk->firstVertex = worldVertexCount;
                for (int i = firstQuadIndex; i < (lastQuadIndex+1); i++)
                {
                    MeshQuad *q = &chunkMeshQuads.quads[i];
                    if (q->blockType == BLOCK_TYPE_WATER) { 
                        continue;
                    }

                    if (worldVertices == NULL) {
                        worldVertexCapacity = 1024;
                        worldVertices = malloc(sizeof(Vertex) * worldVertexCapacity);
                    } else {
                        if ((worldVertexCount + 6) > worldVertexCapacity)
                        {
                            worldVertexCapacity = worldVertexCapacity * 2 + 1024;
                            worldVertices = realloc(worldVertices, sizeof(Vertex) * worldVertexCapacity);
                        }
                    }

                    float x = q->x;
                    float y = q->y;
                    float z = q->z;

                    float w = q->width;
                    float h = q->height;

                    Vertex v0, v1, v2, v3;
                    GLfloat Vertices[8][3] = {
                        // front face
                        {-0.5, 0.5, 0.5},
                        {0.5, 0.5, 0.5},
                        {0.5, -0.5, 0.5},
                        {-0.5, -0.5, 0.5},
                        // back face
                        {-0.5, 0.5, -0.5},
                        {0.5, 0.5, -0.5},
                        {0.5, -0.5, -0.5},
                        {-0.5, -0.5, -0.5},
                    };
                    switch(q->faceType)
                    {
                        case FACE_FRONT:
                            v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                            v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                            v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                            v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                            break;

                        case FACE_BACK:
                            v0 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], 0, 0};
                            v1 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], w, 0};
                            v2 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], w, h};
                            v3 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], 0, h};
                            break;

                        case FACE_LEFT:
                            v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                            v1 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], w, 0};
                            v2 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], w, h};
                            v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                            break;

                        case FACE_RIGHT:
                            v0 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], 0, 0};
                            v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                            v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                            v3 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], 0, h};
                            break;
                        case FACE_TOP:
                            v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                            v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                            v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                            v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                            break;

                        case FACE_BOTTOM:
                            v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                            v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                            v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                            v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                            break;
                    }

                    float minX = MIN4(v0.x,v1.x, v2.x, v3.x);
                    float maxX = MAX4(v0.x,v1.x, v2.x, v3.x);
                    float minY = MIN4(v0.y,v1.y, v2.y, v3.y);
                    float maxY = MAX4(v0.y,v1.y, v2.y, v3.y);
                    float minZ = MIN4(v0.z,v1.z, v2.z, v3.z);
                    float maxZ = MAX4(v0.z,v1.z, v2.z, v3.z);

                    float amtDx = maxX - minX;
                    float amtDy = maxY - minY;
                    float amtDz = maxZ - minZ;

                    float size[2] = {w, h};
                    // scale along face axes only

                    int sideTextureIndex;
                    int topTextureIndex;
                    int bottomTextureIndex;

                    switch (q->blockType)
                    {
                    case BLOCK_TYPE_GRASS:
                        sideTextureIndex = GRASS_SIDE_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = GRASS_TOP_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                        break;
                    case BLOCK_TYPE_DIRT:
                        sideTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                        break;
                    case BLOCK_TYPE_STONE:
                        sideTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                        break;
                    case BLOCK_TYPE_SAND:
                        sideTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                        break;
                    case BLOCK_TYPE_OAK:
                        sideTextureIndex = OAK_SIDE_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = OAK_TOP_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = OAK_TOP_TEXTURE_ARRAY_INDEX;
                        break;
                    case BLOCK_TYPE_LEAVES:
                        sideTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                        topTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                        bottomTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                        break;
                    default:
                        printf("No correct block type entered!\n");
                        break;
                    }


                    if (amtDy == 0.0f)
                    {
                        // X–Z face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                                                    : i == 2   ? &v2
                                                               : &v3);
                            v->x = (v->x + 0.5f) * size[0] - 0.5f;
                            v->z = (v->z + 0.5f) * size[1] - 0.5f;


                            v->u = v->x + 0.5f;
                            v->v = v->z + 0.5f;

                            v->layer = (q->faceType == FACE_TOP) ? topTextureIndex : bottomTextureIndex;          
                        }

                    }
                    else if (amtDx == 0.0f)
                    {
                        // Y–Z face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                            : i == 2   ? &v2
                                       : &v3);
                            v->y = (v->y + 0.5f) * size[0] - 0.5f;
                            v->z = (v->z + 0.5f) * size[1] - 0.5f;

                            v->u = v->z + 0.5f;
                            v->v = 1.0 - (v->y + 0.5f);

                            v->layer = sideTextureIndex;  
                        }
                    }
                    else
                    {
                        // X–Y face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                            : i == 2   ? &v2
                                       : &v3);
                            v->x = (v->x + 0.5f) * size[0] - 0.5f;
                            v->y = (v->y + 0.5f) * size[1] - 0.5f;

                            v->u = v->x + 0.5f;
                            v->v = 1.0 - (v->y + 0.5f);

                            v->layer = sideTextureIndex; 
                        } 
                    }


                    for (int i = 0; i < 4; i++)
                    {
                        Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                        : i == 2   ? &v2
                                   : &v3);

                        v->x += x;
                        v->y += y;
                        v->z += z;
                    }

                    worldVertices[worldVertexCount++] = v0;
                    worldVertices[worldVertexCount++] = v1;
                    worldVertices[worldVertexCount++] = v2;
                    worldVertices[worldVertexCount++] = v0;
                    worldVertices[worldVertexCount++] = v2;
                    worldVertices[worldVertexCount++] = v3;
                }
                chunk->lastVertex = worldVertexCount-1;

                chunk->hasWaterVertices = 1;
                chunk->triggerVertexDeletion = 0;

                firstQuadIndex = chunk->firstQuadIndex;
                lastQuadIndex = chunk->lastQuadIndex;


             

                chunk->firstWaterVertex = waterVertexCount;
                for (int i = firstQuadIndex; i < (lastQuadIndex+1); i++)
                {
                    MeshQuad *q = &chunkMeshQuads.quads[i];
                    if (q->blockType != BLOCK_TYPE_WATER) { 
                        continue;
                    }

                    if ((q->y < (float)SEA_LEVEL-1)) {
                        continue;
                     }

                    if (waterVertices == NULL) {
                        waterVertexCapacity = 1024;
                        waterVertices = malloc(sizeof(Vertex) * waterVertexCapacity);
                    } else {
                        if (waterVertexCount + 6 > waterVertexCapacity)
                        {
                            waterVertexCapacity = waterVertexCapacity * 2 + 1024;
                            waterVertices = realloc(waterVertices, sizeof(Vertex) * waterVertexCapacity);
                        }
                    }
                
                    float x = q->x;
                    float y = q->y;
                    float z = q->z;

                    float w = q->width;
                    float h = q->height;

                    Vertex v0, v1, v2, v3;
                    GLfloat Vertices[8][3] = {
                        // front face
                        {-0.5, 0.5, 0.5},
                        {0.5, 0.5, 0.5},
                        {0.5, -0.5, 0.5},
                        {-0.5, -0.5, 0.5},
                        // back face
                        {-0.5, 0.5, -0.5},
                        {0.5, 0.5, -0.5},
                        {0.5, -0.5, -0.5},
                        {-0.5, -0.5, -0.5},
                    };

                    switch(q->faceType)
                    {
                        case FACE_FRONT:
                            v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                            v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                            v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                            v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                            break;

                        case FACE_BACK:
                            v0 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], 0, 0};
                            v1 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], w, 0};
                            v2 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], w, h};
                            v3 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], 0, h};
                            break;

                        case FACE_LEFT:
                            v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                            v1 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], w, 0};
                            v2 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], w, h};
                            v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                            break;

                        case FACE_RIGHT:
                            v0 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], 0, 0};
                            v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                            v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                            v3 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], 0, h};
                            break;
                        case FACE_TOP:
                            v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                            v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                            v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                            v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                            break;

                        case FACE_BOTTOM:
                            v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                            v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                            v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                            v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                            break;
                    }

                    float minX = MIN4(v0.x,v1.x, v2.x, v3.x);
                    float maxX = MAX4(v0.x,v1.x, v2.x, v3.x);
                    float minY = MIN4(v0.y,v1.y, v2.y, v3.y);
                    float maxY = MAX4(v0.y,v1.y, v2.y, v3.y);
                    float minZ = MIN4(v0.z,v1.z, v2.z, v3.z);
                    float maxZ = MAX4(v0.z,v1.z, v2.z, v3.z);

                 

                    float amtDx = maxX - minX;
                    float amtDy = maxY - minY;
                    float amtDz = maxZ - minZ;

                    float size[2] = {w, h};
                    // scale along face axes only

                    int sideTextureIndex   = WATER_TEXTURE_ARRAY_INDEX;
                    int topTextureIndex    = WATER_TEXTURE_ARRAY_INDEX;
                    int bottomTextureIndex = WATER_TEXTURE_ARRAY_INDEX;

                    if (amtDy == 0.0f)
                    {
                        // X–Z face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                                                    : i == 2   ? &v2
                                                               : &v3);
                            v->x = (v->x + 0.5f) * size[0] - 0.5f;
                            v->z = (v->z + 0.5f) * size[1] - 0.5f;


                            v->u = v->x + 0.5f;
                            v->v = v->z + 0.5f;

                            v->layer = (q->faceType == FACE_TOP) ? topTextureIndex : bottomTextureIndex;          
                        }

                    }
                    else if (amtDx == 0.0f)
                    {
                        // Y–Z face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                            : i == 2   ? &v2
                                       : &v3);
                            v->y = (v->y + 0.5f) * size[0] - 0.5f;
                            v->z = (v->z + 0.5f) * size[1] - 0.5f;

                            v->u = v->z + 0.5f;
                            v->v = 1.0 - (v->y + 0.5f);

                            v->layer = sideTextureIndex;  
                        }
                    }
                    else
                    {
                        // X–Y face
                        for (int i = 0; i < 4; i++)
                        {
                            Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                            : i == 2   ? &v2
                                       : &v3);
                            v->x = (v->x + 0.5f) * size[0] - 0.5f;
                            v->y = (v->y + 0.5f) * size[1] - 0.5f;

                            v->u = v->x + 0.5f;
                            v->v = 1.0 - (v->y + 0.5f);

                            v->layer = sideTextureIndex; 
                        } 
                    }


                    for (int i = 0; i < 4; i++)
                    {
                        Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                        : i == 2   ? &v2
                                   : &v3);

                        v->x += x;
                        v->y += y;
                        v->z += z;


                        if (v->y > 0.0f) // top vertices of cube
                        {
                            v->y -= 0.1;
                        }
                    }

                    waterVertices[waterVertexCount++] = v0;
                    waterVertices[waterVertexCount++] = v1;
                    waterVertices[waterVertexCount++] = v2;
                    waterVertices[waterVertexCount++] = v0;
                    waterVertices[waterVertexCount++] = v2;
                    waterVertices[waterVertexCount++] = v3;
                }
                chunk->lastWaterVertex = waterVertexCount-1;
            }
            
            uploadWorldMesh();
        }
    }
    
}

void triggerRenderChunkRebuild (Chunk *chunk) {
    chunk->triggerVertexRecreation = 1;
}