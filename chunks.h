#ifndef CHUNKS_H
#define CHUNKS_H
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdint.h>

typedef struct Block
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    int isAir;
    int blockType; // the block type for selecting the texture later
    int isSlope;
} Block;

typedef struct Chunk
{
    Block blocks[16 * 16 * 128];
    uint8_t *lightData; // for each, Upper 4 bits -> skylight, Lower 4 bits -> blockLight
    GLfloat chunkStartX;
    GLfloat chunkStartZ;
    int firstQuadIndex;
    int lastQuadIndex;
    int flag;
    int hasMesh;
    uint64_t key;
    int hasVertices;
    int hasWaterVertices;
    int firstVertex;
    int lastVertex;
    int firstWaterVertex;
    int lastWaterVertex;
    int triggerVertexDeletion;
    int triggerVertexRecreation;
    int isDirty;
    int lightDirty;
    int gpuLightIndex;
    int isInitialLightCreated;
} Chunk;

typedef struct MeshQuad
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat width;
    GLfloat height;
    int faceType;  // these are listed below!
    int blockType; // these are listed below!
    int slopeDirection; // these are only for the slope blocks!
} MeshQuad;

typedef struct ChunkMeshQuads
{
    MeshQuad *quads;
    int capacity;
    int amtQuads;
} ChunkMeshQuads;

typedef struct BlockType
{
    int id; // essentially BLOCK_TYPE_GRASS or smt
    int topTexture;
    int bottomTexture;
    int sideTexture;
    int isRenderSolid;
    float blockBreakingTime;
    int isRenderCross;
    int isPhysicsSolid;
    int lightEmissivePower; // 0 to 15
} BlockType;

typedef struct QueueEntry
{
    int x;
    int y;
    int z;
} QueueEntry;

typedef struct Queue
{
    QueueEntry items[100000];
    int front;
    int rear;
    int capacity;
    int size;
} Queue;

// these are the face types
#define FACE_TOP 1
#define FACE_BOTTOM 2
#define FACE_LEFT 3
#define FACE_RIGHT 4
#define FACE_FRONT 5
#define FACE_BACK 6
#define FACE_CROSS 7 // use for things like flowers
#define FACE_SLOPE 8

#define PLANE_X 1
#define PLANE_Y 2
#define PLANE_Z 3

#define BLOCK_TYPE_SELECT -55
#define BLOCK_TYPE_AIR 0
#define BLOCK_TYPE_GRASS 1
#define BLOCK_TYPE_DIRT 2
#define BLOCK_TYPE_STONE 3
#define BLOCK_TYPE_WATER 4
#define BLOCK_TYPE_RED_CLAY 5
#define BLOCK_TYPE_OAK 6
#define BLOCK_TYPE_LEAVES 7
#define BLOCK_TYPE_ORCHID 8
#define BLOCK_TYPE_SHORT_GRASS 9
#define BLOCK_TYPE_TORCH 10

#define ChunkWidthX 16
#define ChunkLengthZ 16
#define ChunkHeightY 128

#define CHUNK_FLAG_LOADED 1
#define CHUNK_FLAG_RENDERED_AND_LOADED 2

#define SEA_LEVEL 35

#define BASELINE_SKYLIGHT_VALUE 15

// upper is more left and bigger, lower is more right and smaller
#define GET_SKYLIGHT(b) (((b) >> 4) & 0xF)                           // upper 4 bits
#define GET_BLOCK_LIGHT(b) ((b) & 0xF)                               // lower 4 bits
#define SET_SKYLIGHT(b, s) ((b) = ((b) & 0x0F) | (((s) & 0xF) << 4)) // clear upper 4 bits and insert shifted skylight
#define SET_BLOCK_LIGHT(b, s) ((b) = ((b) & 0xF0) | ((s) & 0xF))     // clear lower 4 bits and insert block light

#define IS_CELL_SOLID(cell) ((cell).blockType != BLOCK_TYPE_AIR)


extern float BlockWidthX;
extern float BlockLengthZ;
extern float BlockHeightY;
extern ChunkMeshQuads chunkMeshQuads;
extern Queue lightingQueue;
extern BlockType blockRegistry[100];

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd, int isFirstCreation, int flag, uint64_t key);
void initChunkMeshingSystem();
void handleProgramClose();
void generateChunkMesh(Chunk *chunk);
void deleteChunkMesh(Chunk *chunk);
void resetLightingQueue(Queue *queue);
void enqueue(Queue *queue, int worldX, int worldY, int worldZ);
QueueEntry *dequeue(Queue *queue);
void propagateLightBFS(Queue *targetQueue, int isBlockLight);
void seedNeighborBorderBlockLighting(Queue *targetQueue, Chunk *chunk);
void seedNeighborBorderSkyLighting(Queue *targetQueue, Chunk *chunk);
void computeInitialLightingForChunk(Chunk *chunk);
#endif