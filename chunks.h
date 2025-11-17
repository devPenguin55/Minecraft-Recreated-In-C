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

extern PlayerChunks world;
extern int ChunkWidthX;
extern int ChunkLengthZ;
extern int ChunkHeightY;
extern float BlockWidthX;
extern float BlockLengthZ;
extern float BlockHeightY;

void createChunk(Chunk *chunk, GLfloat xAdd, GLfloat zAdd);
void initWorld(PlayerChunks *world);
void generateChunkMesh(Chunk *chunk);

#endif