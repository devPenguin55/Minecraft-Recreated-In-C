#ifndef CHUNKS_H
#define CHUNKS_H
#include <GL/glu.h>
#include <GL/glut.h>

typedef struct Block
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    int isAir;
} Block;

typedef struct Chunk
{
    Block blocks[16 * 16 * 64];
} Chunk;

typedef struct PlayerChunks
{
    Chunk chunks[16];
} PlayerChunks;

typedef struct MeshQuad 
{
    GLfloat      x;
    GLfloat      y;
    GLfloat      z;
    GLfloat  width;
    GLfloat height;
    int   faceType; // these are listed below!
} MeshQuad;

typedef struct ChunkMeshQuads {
    MeshQuad *quads;
    int capacity;
    int amtQuads;
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


extern PlayerChunks world;
#define ChunkWidthX  16
#define ChunkLengthZ 16
#define ChunkHeightY 5
#define MASK_INDEX(x,y,z)  ((y)*ChunkLengthZ*ChunkWidthX + (z)*ChunkWidthX + (x))
extern float BlockWidthX;
extern float BlockLengthZ;
extern float BlockHeightY;

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd);
void initWorld(PlayerChunks *world);
void generateFaceMaskQuads(ChunkMeshQuads *chunkMeshQuads, int *faceMask, int firstPlane, int secondPlane, int constantPlane);
void generateChunkMesh(Chunk *chunk);

#endif