#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render.h"
#include "input.h"
#include "chunks.h"
#include "chunkLoaderManager.h"

GLfloat T = 0;
GLuint atlasTexture;

double lastFpsTime = 0.0;
double lastTime = 0.0;
double fps = 0.0;
int frameCount = 0;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

int ATLAS_WIDTH, ATLAS_HEIGHT;
GLuint loadTexture(const char *filename)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int channels;
    unsigned char *data = stbi_load(filename, &ATLAS_WIDTH, &ATLAS_HEIGHT, &channels, 0);
    if (data)
    {   
        printf("Loaded texture: %dx%d, channels: %d\n", ATLAS_WIDTH, ATLAS_HEIGHT, channels);
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, ATLAS_WIDTH, ATLAS_HEIGHT, 0, format, GL_UNSIGNED_BYTE, data);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, ATLAS_WIDTH, ATLAS_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, data);

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

    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);

    atlasTexture = loadTexture("atlas.png");
    glEnable(GL_TEXTURE_2D);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    

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

void uvCoordinatesFromTextureIndex(
    int textureIndex,
    UV *uv,
    int amtHorizTextures,
    int amtVertTextures,
    int atlasWidthPixels,
    int atlasHeightPixels
) {
    float tileWidth  = 1.0f / amtHorizTextures;
    float tileHeight = 1.0f / amtVertTextures;

    int tileX = textureIndex % amtHorizTextures;
    int tileY = textureIndex / amtHorizTextures;

    float epsilonU = 1.0f / atlasWidthPixels;
    float epsilonV = 1.0f / atlasHeightPixels;

    uv->u  = tileX * tileWidth + epsilonU;
    uv->v  = tileY * tileHeight + epsilonV;
    uv->u1 = (tileX + 1) * tileWidth - epsilonU;
    uv->v1 = (tileY + 1) * tileHeight - epsilonV;
}



void face(GLfloat A[], GLfloat B[], GLfloat C[], GLfloat D[], GLfloat transformation[3], int textureIndex, GLfloat size[2])
{
    float minX = MIN4(A[0], B[0], C[0], D[0]);
    float maxX = MAX4(A[0], B[0], C[0], D[0]);

    float minY = MIN4(A[1], B[1], C[1], D[1]);
    float maxY = MAX4(A[1], B[1], C[1], D[1]);

    float minZ = MIN4(A[2], B[2], C[2], D[2]);
    float maxZ = MAX4(A[2], B[2], C[2], D[2]);

    // these differences help determine which axes are used to then create the sub faces
    float amtDx = maxX - minX;
    float amtDy = maxY - minY;
    float amtDz = maxZ - minZ;


    UV uv;
    uvCoordinatesFromTextureIndex(textureIndex, &uv, 6, 3, ATLAS_WIDTH, ATLAS_HEIGHT);
    float du = uv.u1 - uv.u;
    float dv = uv.v1 - uv.v;
    float U0 = uv.u;
    float V0 = uv.v;
    float U1 = uv.u + du;
    float V1 = uv.v + dv;

    // iterate for 2 of the 3 axes, but we do not know which one is the right one until the function is called
    if (amtDy == 0) {
        for (float dx = 0; dx < size[0]; dx++) {
            for (float dz = 0; dz < size[1]; dz++) {
                glPushMatrix();
                glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);
                glTranslatef(
                    transformation[0], 
                    transformation[1], 
                    transformation[2]
                );
                glBindTexture(GL_TEXTURE_2D, atlasTexture);
                if (pressedKeys['z']) {
                    glBegin(GL_LINE_LOOP);
                } else {
                    glBegin(GL_QUADS);
                }
    
                // dx
                A[0] += dx;
                B[0] += dx;
                C[0] += dx;
                D[0] += dx;
                // dz
                A[2] += dz;
                B[2] += dz;
                C[2] += dz;
                D[2] += dz;
                glTexCoord2f(U0, V0); glVertex3fv(A);
                glTexCoord2f(U1, V0); glVertex3fv(B);
                glTexCoord2f(U1, V1); glVertex3fv(C);
                glTexCoord2f(U0, V1); glVertex3fv(D);
                glEnd();
                glPopMatrix();
                // dx
                A[0] -= dx;
                B[0] -= dx;
                C[0] -= dx;
                D[0] -= dx;
                // dz
                A[2] -= dz;
                B[2] -= dz;
                C[2] -= dz;
                D[2] -= dz;
            }
        }
    } else if (amtDx == 0) {
        for (float dy = 0; dy < size[0]; dy++) {
            for (float dz = 0; dz < size[1]; dz++) {
                glPushMatrix();
                glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);
                glTranslatef(
                    transformation[0], 
                    transformation[1], 
                    transformation[2]
                );
                glBindTexture(GL_TEXTURE_2D, atlasTexture);
                if (pressedKeys['z']) {
                    glBegin(GL_LINE_LOOP);
                } else {
                    glBegin(GL_QUADS);
                }
        
                // dy
                A[1] += dy;
                B[1] += dy;
                C[1] += dy;
                D[1] += dy;
                // dz
                A[2] += dz;
                B[2] += dz;
                C[2] += dz;
                D[2] += dz;
                glTexCoord2f(U0, V1); glVertex3fv(A);
                glTexCoord2f(U1, V1); glVertex3fv(B);
                glTexCoord2f(U1, V0); glVertex3fv(C);
                glTexCoord2f(U0, V0); glVertex3fv(D);
                glEnd();
                glPopMatrix();
                // dy
                A[1] -= dy;
                B[1] -= dy;
                C[1] -= dy;
                D[1] -= dy;
                // dz
                A[2] -= dz;
                B[2] -= dz;
                C[2] -= dz;
                D[2] -= dz;
            }
        }
    } else {
        // if dz == 0
        for (float dx = 0; dx < size[0]; dx++) {
            for (float dy = 0; dy < size[1]; dy++) {
                glPushMatrix();
                glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);
                glTranslatef(
                    transformation[0], 
                    transformation[1], 
                    transformation[2]
                );
                glBindTexture(GL_TEXTURE_2D, atlasTexture);
                if (pressedKeys['z']) {
                    glBegin(GL_LINE_LOOP);
                } else {
                    glBegin(GL_QUADS);
                }

                // dx
                A[0] += dx;
                B[0] += dx;
                C[0] += dx;
                D[0] += dx;
                // dy
                A[1] += dy;
                B[1] += dy;
                C[1] += dy;
                D[1] += dy;
                glTexCoord2f(U0, V0); glVertex3fv(A);
                glTexCoord2f(U1, V0); glVertex3fv(B);
                glTexCoord2f(U1, V1); glVertex3fv(C);
                glTexCoord2f(U0, V1); glVertex3fv(D);
                glEnd();
                glPopMatrix();
                // dx
                A[0] -= dx;
                B[0] -= dx;
                C[0] -= dx;
                D[0] -= dx;
                // dy
                A[1] -= dy;
                B[1] -= dy;
                C[1] -= dy;
                D[1] -= dy;
            }
        }
    }
}

void cubeFace(GLfloat Vertices[8][3], GLfloat transformation[3], GLfloat size[2], int faceType, int blockType)
{   
    int sideTextureIndex;
    int topTextureIndex;
    int bottomTextureIndex;

    switch (blockType) {
        case BLOCK_TYPE_GRASS:
            sideTextureIndex   = 0;
            topTextureIndex    = 6;
            bottomTextureIndex = 1;
            break;
        case BLOCK_TYPE_DIRT:
            sideTextureIndex   = 1;
            topTextureIndex    = 1;
            bottomTextureIndex = 1;
            break;
        case BLOCK_TYPE_STONE:
            sideTextureIndex   = 2;
            topTextureIndex    = 2;
            bottomTextureIndex = 2;
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
    drawText(text, 50, 0);
    
    
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