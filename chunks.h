#ifndef CHUNKS_H
#define CHUNKS_H
#include <GL/glu.h>
#include <GL/glut.h>

typedef struct Block
{
    GLfloat     x;
    GLfloat     y;
    GLfloat     z;
    int     isAir;
    int blockType; // the block type for selecting the texture later
} Block;

typedef struct Chunk
{
    Block blocks[16 * 16 * 64];
    GLfloat        chunkStartX;
    GLfloat        chunkStartZ;
} Chunk;

typedef struct PlayerChunks
{
    Chunk chunks[16];
    int    amtChunks;
} PlayerChunks;

typedef struct MeshQuad 
{
    GLfloat        x;
    GLfloat        y;
    GLfloat        z;
    GLfloat    width;
    GLfloat   height;
    int     faceType; // these are listed below!
    int    blockType; // these are listed below!
} MeshQuad;

typedef struct ChunkMeshQuads {
    MeshQuad *quads;
    int    capacity;
    int    amtQuads;
} ChunkMeshQuads;

// these are the face types
#define FACE_TOP    1
#define FACE_BOTTOM 2
#define FACE_LEFT   3
#define FACE_RIGHT  4
#define FACE_FRONT  5
#define FACE_BACK   6

#define PLANE_X 1
#define PLANE_Y 2
#define PLANE_Z 3

#define BLOCK_TYPE_GRASS 1
#define BLOCK_TYPE_DIRT  2
#define BLOCK_TYPE_STONE 3

#define WORLD_HORIZONTAL_CHUNK_AMT 4
#define WORLD_VERTICAL_CHUNK_AMT   4

#define PLAYER_CHUNK_RADIUS 2

extern PlayerChunks world;
#define ChunkWidthX  16
#define ChunkLengthZ 16
#define ChunkHeightY 64

extern float BlockWidthX;
extern float BlockLengthZ;
extern float BlockHeightY;
extern ChunkMeshQuads chunkMeshQuads;
void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd);
void initWorld(PlayerChunks *world);
void initChunkMeshingSystem();
void handleProgramClose();
void generateChunkMesh(Chunk *chunk, int chunkIdx);
void loadChunksInPlayerRadius(GLfloat playerCoords[2]);

#endif