#ifndef RENDER_H
#define RENDER_H
#include <GL/glu.h>
#include <GL/glut.h>

extern GLfloat T;
extern GLfloat CameraX;
extern GLfloat CameraY;
extern GLfloat CameraZ;
extern GLfloat PlayerDirX;
extern GLfloat PlayerDirY;
extern GLfloat PlayerDirZ;
extern GLuint atlasTexture;

typedef struct UV 
{
    float u;
    float v;
    float u1;
    float v1;
} UV;

void initGraphics();
void reshape(int width, int height);
void spinObject();
void uvCoordinatesFromTextureIndex(int textureIndex, UV *uv, int amtHorizTextures, int amtVertTextures);
void face(GLfloat A[], GLfloat B[], GLfloat C[], GLfloat D[], GLfloat transformation[3], int textureIndex, GLfloat size[3]);
void cubeFace(GLfloat Vertices[8][3], GLfloat transformation[3], GLfloat size[2], int faceType);
void drawGraphics();

#endif