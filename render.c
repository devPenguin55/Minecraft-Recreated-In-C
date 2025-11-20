#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render.h"
#include "input.h"
#include "chunks.h"

GLfloat T = 0;
GLuint atlasTexture;

GLuint loadTexture(const char *filename)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);
    if (data)
    {   
        printf("Loaded texture: %dx%d, channels: %d\n", width, height, channels);
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        printf("Failed to load texture: %s\n", filename);
    }

    stbi_image_free(data);
    return textureID;
}




void initGraphics()
{
    glClearColor(0, 0, 0, 1);
    glColor3f(1, 1, 1);
    glEnable(GL_DEPTH_TEST);

    atlasTexture = loadTexture("atlas.png");
    glEnable(GL_TEXTURE_2D);
}

void reshape(int width, int height)
{
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)width / (float)height;

    gluPerspective(70.0f, aspect, 0.1f, 500.0f);

    glMatrixMode(GL_MODELVIEW);
}


void spinObject()
{
    T = T + 0.00025;
    if (T > 360)
        T = 0;
    glutPostRedisplay();
}

void uvCoordinatesFromTextureIndex(int textureIndex, UV *uv, int amtHorizTextures, int amtVertTextures) {
    float horizLen = 1.0f/amtHorizTextures;
    float vertLen = 1.0f/amtVertTextures;
    
    float x = (int)(textureIndex % amtHorizTextures) * horizLen;
    float y = (int)(textureIndex / amtHorizTextures) * vertLen;

    // bottom left
    uv->u = x;
    uv->v = y;
    // top right
    uv->u1 = x + horizLen;
    uv->v1 = y + vertLen;
}

void face(GLfloat A[], GLfloat B[], GLfloat C[], GLfloat D[], GLfloat transformation[3], int textureIndex)
{
    glPushMatrix();
    glTranslatef(transformation[0], transformation[1], transformation[2]); // move it up
    glScalef(BlockWidthX, BlockLengthZ, BlockHeightY);

    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    if (pressedKeys['z']) {
        glBegin(GL_LINE_LOOP);
    } else {
        glBegin(GL_QUADS);
    }
    
    
    UV uv;
    uvCoordinatesFromTextureIndex(textureIndex, &uv, 6, 3);

    glTexCoord2f(uv.u,  uv.v);  glVertex3fv(A);
    glTexCoord2f(uv.u1, uv.v);  glVertex3fv(B);
    glTexCoord2f(uv.u1, uv.v1); glVertex3fv(C);
    glTexCoord2f(uv.u,  uv.v1); glVertex3fv(D);
    glEnd();

    glPopMatrix();
}

void cube(GLfloat Vertices[8][3], GLfloat transformation[3])
{
    // FRONT (0, 1, 2, 3)
    face(Vertices[0], Vertices[1], Vertices[2], Vertices[3], transformation, 0);

    // BACK (4, 5, 6, 7)
    face(Vertices[5], Vertices[4], Vertices[7], Vertices[6], transformation, 0);

    // LEFT (0, 3, 7, 4)
    face(Vertices[4], Vertices[0], Vertices[3], Vertices[7], transformation, 0);

    // RIGHT (1, 2, 6, 5)
    face(Vertices[1], Vertices[5], Vertices[6], Vertices[2], transformation, 0);

    // TOP (0, 1, 5, 4)
    face(Vertices[4], Vertices[5], Vertices[1], Vertices[0], transformation, 6);

    // BOTTOM (3, 2, 6, 7)
    face(Vertices[3], Vertices[2], Vertices[6], Vertices[7], transformation, 1);
}


void drawGraphics()
{
    handleUserMovement();

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

    // clear color buffer to clear background, uses preset color setup in initGraphics
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // camera
    glLoadIdentity();
    // eye -> where the camera is located in the world space
    // center -> where the camera is looking at
    // up -> what direction is up for the camera
    // CameraX = cos(T)*15;
    // CameraZ = sin(T)*15;
    gluLookAt(
        CameraX, CameraY, CameraZ,
        CameraX + PlayerDirX, CameraY + PlayerDirY, CameraZ + PlayerDirZ,
        0, 1, 0);

    glPointSize(5);

    for (int chunkIndex = 0; chunkIndex < 1; chunkIndex++)
    {
        Chunk *curChunk = &(world.chunks[chunkIndex]);
        float minX = 727379969;
        float maxX = -727379969;
        float minZ = 727379969;
        float maxZ = -727379969;
        for (int blockIndex = 0; blockIndex < ChunkWidthX * ChunkLengthZ * ChunkHeightY; blockIndex++)
        {
            Block *curBlock = &(curChunk->blocks[blockIndex]);

            minX = (curBlock->x < minX) ? curBlock->x : minX;
            maxX = (curBlock->x > maxX) ? curBlock->x : maxX;
            minZ = (curBlock->z < minZ) ? curBlock->z : minZ;
            maxZ = (curBlock->z > maxZ) ? curBlock->z : maxZ;

            if (curBlock->isAir) { continue; }
            
            GLfloat translation[3] = {curBlock->x, curBlock->y, curBlock->z};
            cube(Vertices, translation);
        }

        glPushMatrix();
        glLineWidth(3.0f);

        float halfX = (BlockWidthX) / 2.0f;
        float halfZ = (BlockLengthZ) / 2.0f;
        float borderY = 0.3f; // or top layer
        // printf("x from %f to %f, y from %f to %f\n", minX, maxX, minZ, maxZ);
        glBegin(GL_LINE_LOOP);
        glVertex3f(minX - halfX, borderY, minZ - halfZ);
        glVertex3f(maxX + halfX, borderY, minZ - halfZ);
        glVertex3f(maxX + halfX, borderY, maxZ + halfZ);
        glVertex3f(minX - halfX, borderY, maxZ + halfZ);
        glEnd();

        glLineWidth(1.0f);
        glPopMatrix();

    }

    // switch the content of color and depth buffers
    glutSwapBuffers();
}