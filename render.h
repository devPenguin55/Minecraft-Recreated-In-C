#ifndef RENDER_H
#define RENDER_H
#include <GL/glu.h>
#include <GL/glut.h>
#include "chunks.h"

typedef struct UV 
{
    float u;
    float v;
    float u1;
    float v1;
} UV;

typedef struct SelectedBlockToRender {
    MeshQuad *meshQuad;
    float localX;
    float localY;
    float localZ;
    Chunk *chunk;
} SelectedBlockToRender;

extern GLfloat T;
extern GLfloat CameraX;
extern GLfloat CameraY;
extern GLfloat CameraZ;
extern GLfloat PlayerDirX;
extern GLfloat PlayerDirY;
extern GLfloat PlayerDirZ;
extern SelectedBlockToRender selectedBlockToRender;

void initGraphics();
void reshape(int width, int height);
void spinObject();
void face(
    GLfloat A[3],
    GLfloat B[3],
    GLfloat C[3],
    GLfloat D[3],
    GLfloat transformation[3],
    GLuint texture,
    GLfloat size[2]
);
void cubeFace(GLfloat Vertices[8][3], GLfloat transformation[3], GLfloat size[2], int faceType, int blockType);
void drawText(const char *text, float x, float y);
void drawGraphics();

#endif