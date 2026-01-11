#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render.h"
#include "input.h"
#include "chunks.h"
#include "chunkLoaderManager.h"
#include "raycast.h"

GLfloat T = 0;
GLuint grassSideTexture;
GLuint grassTopTexture;
GLuint dirtTexture;
GLuint stoneTexture;

double lastFpsTime = 0.0;
double lastTime = 0.0;
double fps = 0.0;
int frameCount = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

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

        GLenum format;
        if (channels == 1) {
            unsigned char* rgbData = malloc(width * height * 3);
            for (int i = 0; i < width * height; i++) {
                rgbData[i*3 + 0] = data[i];
                rgbData[i*3 + 1] = data[i];
                rgbData[i*3 + 2] = data[i];
            }
            gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, rgbData);
            free(rgbData);
        } else {
            format = (channels == 4) ? GL_RGBA : GL_RGB;
            gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);

    grassSideTexture = loadTexture("assets\\grassSide.png");
    grassTopTexture  = loadTexture("assets\\grassTop.png");
    dirtTexture      = loadTexture("assets\\dirt.png");
    stoneTexture     = loadTexture("assets\\stone.png");
    glEnable(GL_TEXTURE_2D);

    

}

void reshape(int width, int height)
{
    if (height == 0) { 
        height = 1; 
    }

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

void face(
    GLfloat A[3],
    GLfloat B[3],
    GLfloat C[3],
    GLfloat D[3],
    GLfloat transformation[3],
    GLuint texture,
    GLfloat size[2]
) {
    GLfloat vA[3] = { A[0], A[1], A[2] };
    GLfloat vB[3] = { B[0], B[1], B[2] };
    GLfloat vC[3] = { C[0], C[1], C[2] };
    GLfloat vD[3] = { D[0], D[1], D[2] };

    float minX = MIN4(A[0], B[0], C[0], D[0]);
    float maxX = MAX4(A[0], B[0], C[0], D[0]);
    float minY = MIN4(A[1], B[1], C[1], D[1]);
    float maxY = MAX4(A[1], B[1], C[1], D[1]);
    float minZ = MIN4(A[2], B[2], C[2], D[2]);
    float maxZ = MAX4(A[2], B[2], C[2], D[2]);

    float amtDx = maxX - minX; 
    float amtDy = maxY - minY;
    float amtDz = maxZ - minZ;

    // scale along face axes only
    if (amtDy == 0.0f) {
        // X–Z face
        for (int i = 0; i < 4; i++) {
            GLfloat* v = (i == 0 ? vA : i == 1 ? vB : i == 2 ? vC : vD);
            v[0] = (v[0] + 0.5f) * size[0] - 0.5f;
            v[2] = (v[2] + 0.5f) * size[1] - 0.5f;
        }
    } else if (amtDx == 0.0f) {
        // Y–Z face
        for (int i = 0; i < 4; i++) {
            GLfloat* v = (i == 0 ? vA : i == 1 ? vB : i == 2 ? vC : vD);
            v[1] = (v[1] + 0.5f) * size[0] - 0.5f;
            v[2] = (v[2] + 0.5f) * size[1] - 0.5f;
        }
    } else {
        // X–Y face
        for (int i = 0; i < 4; i++) {
            GLfloat* v = (i == 0 ? vA : i == 1 ? vB : i == 2 ? vC : vD);
            v[0] = (v[0] + 0.5f) * size[0] - 0.5f;
            v[1] = (v[1] + 0.5f) * size[1] - 0.5f;
        }
    }

    glPushMatrix();
    glTranslatef(transformation[0], transformation[1], transformation[2]);
    glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    
    if (pressedKeys['z']) {
        glBegin(GL_LINE_LOOP);
    } else {
        glBegin(GL_QUADS);
    }

    GLfloat U0, V0, U1, V1;
    U0 = 0.0f;
    V0 = 0.0f;
    U1 = size[0];
    V1 = size[1];

    if (amtDy == 0.0f) {       // X–Z face
        glTexCoord2f(vA[0] + 0.5f, vA[2] + 0.5f); glVertex3fv(vA);
        glTexCoord2f(vB[0] + 0.5f, vB[2] + 0.5f); glVertex3fv(vB);
        glTexCoord2f(vC[0] + 0.5f, vC[2] + 0.5f); glVertex3fv(vC);
        glTexCoord2f(vD[0] + 0.5f, vD[2] + 0.5f); glVertex3fv(vD);
    } else if (amtDx == 0.0f) { // Y–Z face
        glTexCoord2f(vA[2] + 0.5f, 1.0f - (vA[1] + 0.5f)); glVertex3fv(vA);
        glTexCoord2f(vB[2] + 0.5f, 1.0f - (vB[1] + 0.5f)); glVertex3fv(vB);
        glTexCoord2f(vC[2] + 0.5f, 1.0f - (vC[1] + 0.5f)); glVertex3fv(vC);
        glTexCoord2f(vD[2] + 0.5f, 1.0f - (vD[1] + 0.5f)); glVertex3fv(vD);
    } else {                    // X–Y face
        glTexCoord2f(vA[0] + 0.5f, 1.0f - (vA[1] + 0.5f)); glVertex3fv(vA);
        glTexCoord2f(vB[0] + 0.5f, 1.0f - (vB[1] + 0.5f)); glVertex3fv(vB);
        glTexCoord2f(vC[0] + 0.5f, 1.0f - (vC[1] + 0.5f)); glVertex3fv(vC);
        glTexCoord2f(vD[0] + 0.5f, 1.0f - (vD[1] + 0.5f)); glVertex3fv(vD);
    }

    glEnd();
    glPopMatrix();
}


void cubeFace(GLfloat Vertices[8][3], GLfloat transformation[3], GLfloat size[2], int faceType, int blockType)
{   
    int sideTextureIndex;
    int topTextureIndex;
    int bottomTextureIndex;

    switch (blockType) {
        case BLOCK_TYPE_GRASS:
            sideTextureIndex   = grassSideTexture;
            topTextureIndex    = grassTopTexture;
            bottomTextureIndex = dirtTexture;
            break;
        case BLOCK_TYPE_DIRT:
            sideTextureIndex   = dirtTexture;
            topTextureIndex    = dirtTexture;
            bottomTextureIndex = dirtTexture;
            break;
        case BLOCK_TYPE_STONE:
            sideTextureIndex   = stoneTexture;
            topTextureIndex    = stoneTexture;
            bottomTextureIndex = stoneTexture;
            break;
        default:
            printf("No correct block type entered!\n");
            break;
    }

    switch (faceType) {
        case FACE_FRONT:
            face(Vertices[0], Vertices[1], Vertices[2], Vertices[3], transformation, sideTextureIndex, size);
            break;

        case FACE_BACK:
            face(Vertices[5], Vertices[4], Vertices[7], Vertices[6], transformation, sideTextureIndex, size);
            break;

        case FACE_LEFT:
            face(Vertices[7], Vertices[3], Vertices[0], Vertices[4], transformation, sideTextureIndex, size);
            break;

        case FACE_RIGHT:
            face(Vertices[2], Vertices[6], Vertices[5], Vertices[1], transformation, sideTextureIndex, size);
            break;

        case FACE_TOP:
            face(Vertices[0], Vertices[1], Vertices[5], Vertices[4], transformation, topTextureIndex, size);
            break;

        case FACE_BOTTOM:
            face(Vertices[7], Vertices[6], Vertices[2], Vertices[3], transformation, bottomTextureIndex, size);
            break;
    }

}

void drawText(const char *text, float x, float y) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        text++;
    }
}

void drawGraphics()
{
    frameCount++;
    double currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0;  // seconds

    // update FPS every 0.25 seconds
    if (currentTime - lastFpsTime >= 0.25) {
        fps = frameCount / (currentTime - lastFpsTime);
        frameCount = 0;
        lastFpsTime = currentTime;
    }
    PLAYER_SPEED = 10*(currentTime - lastTime);
    lastTime = currentTime;
    handleUserMovement();

    raycastFromCamera();

    GLfloat playerCoords[2] = {CameraX, CameraZ};
    loadChunks(playerCoords);

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

    for (int quadIndex = 0; quadIndex < chunkMeshQuads.amtQuads; quadIndex++)
    {
        MeshQuad *curQuad = &(chunkMeshQuads.quads[quadIndex]);
        

            
        GLfloat translation[3];
        translation[0] = curQuad->x;
        translation[1] = curQuad->y;
        translation[2] = curQuad->z;
        
        /*

        * FACE GUIDE

        switch (curQuad->faceType) {
            case FACE_TOP:
                xWidth = curQuad->width;
                zLength = curQuad->height;
                yHeight = 1;
                break;
            case FACE_BOTTOM: 
                xWidth = curQuad->width;
                zLength = curQuad->height;
                yHeight = 1;
                break;
            case FACE_LEFT:
                xWidth = 1;
                zLength = curQuad->height;
                yHeight = curQuad->width;
                break;
            case FACE_RIGHT:
                xWidth = 1;
                zLength = curQuad->height;
                yHeight = curQuad->width;
                break;
            case FACE_FRONT:
                xWidth = curQuad->width;
                zLength = 1;
                yHeight = curQuad->height;
                break;
            case FACE_BACK:
                xWidth = curQuad->width;
                zLength = 1;
                yHeight = curQuad->height;
                break;
        } */

        GLfloat size[2] = {curQuad->width, curQuad->height};
        cubeFace(Vertices, translation, size, curQuad->faceType, curQuad->blockType);


        // glPushMatrix();
        // glLineWidth(3.0f);

        // float halfX = (BlockWidthX) / 2.0f;
        // float halfZ = (BlockLengthZ) / 2.0f;
        // float borderY = 0.3f; // or top layer
        // // printf("x from %f to %f, y from %f to %f\n", minX, maxX, minZ, maxZ);
        // glBegin(GL_LINE_LOOP);
        // glVertex3f(minX - halfX, borderY, minZ - halfZ);
        // glVertex3f(maxX + halfX, borderY, minZ - halfZ);
        // glVertex3f(maxX + halfX, borderY, maxZ + halfZ);
        // glVertex3f(minX - halfX, borderY, maxZ + halfZ);
        // glEnd();

        // glLineWidth(1.0f);
        // glPopMatrix();

    }



    char text[64];
    snprintf(text, sizeof(text), "FPS: %.1f, Quads: %d", fps, chunkMeshQuads.amtQuads);

    glColor3f(1, 1, 1);
    drawText(text, 50, 100);
    
    
    glBegin(GL_POINTS);
        glColor3f(1, 0, 0);
        glVertex3f(-1, 0, -1);
        glColor3f(0, 1, 0);
        glVertex3f(2, 0, -1);
        glColor3f(1, 0, 0);
        glVertex3f(-1, 3, -1);
        glColor3f(1, 1, 1);
    glEnd();


    // switch the content of color and depth buffers
    glutSwapBuffers();
}