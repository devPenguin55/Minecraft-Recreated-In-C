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

GLuint destroyStageTextureArray[9];

double lastFpsTime = 0.0;
double lastTime = 0.0;
double fps = 0.0;
int frameCount = 0;

SelectedBlockToRender selectedBlockToRender;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

static inline float clamp(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}


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

    destroyStageTextureArray[0] = loadTexture("assets\\destroy_stage_0.png");
    destroyStageTextureArray[1] = loadTexture("assets\\destroy_stage_1.png");
    destroyStageTextureArray[2] = loadTexture("assets\\destroy_stage_2.png");
    destroyStageTextureArray[3] = loadTexture("assets\\destroy_stage_3.png");
    destroyStageTextureArray[4] = loadTexture("assets\\destroy_stage_4.png");
    destroyStageTextureArray[5] = loadTexture("assets\\destroy_stage_5.png");
    destroyStageTextureArray[6] = loadTexture("assets\\destroy_stage_6.png");
    destroyStageTextureArray[7] = loadTexture("assets\\destroy_stage_7.png");
    destroyStageTextureArray[8] = loadTexture("assets\\destroy_stage_8.png");
    glEnable(GL_TEXTURE_2D);

    selectedBlockToRender.active = 0;
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

    if ((int)texture == BLOCK_TYPE_OUTLINE) {
        if (beginBlockBreakingBlockType == -1) {
            glPopMatrix();
            return;
        }

        int stage = 9 * ((float)userBlockBreakingTimeElapsed / blockBreakingTimeByBlockType[beginBlockBreakingBlockType]);
        if (stage > 8) stage = 8;

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE);       // don’t write depth
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, destroyStageTextureArray[stage]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        float crackAlpha = (stage / 20.0f);
        crackAlpha = (crackAlpha > 0.5f) ? 0.5f : (crackAlpha*1.5f);
        glColor4f(1.0f, 1.0f, 1.0f, crackAlpha);

        // Compute face normal from vertices
        GLfloat u[3], v[3], normal[3];
        for (int i = 0; i < 3; i++) {
            u[i] = vB[i] - vA[i];
            v[i] = vC[i] - vA[i];
        }
        normal[0] = u[1]*v[2] - u[2]*v[1];
        normal[1] = u[2]*v[0] - u[0]*v[2];
        normal[2] = u[0]*v[1] - u[1]*v[0];
        float len = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
        if (len != 0.0f) {
            normal[0] /= len;
            normal[1] /= len;
            normal[2] /= len;
        }

        
        glDisable(GL_CULL_FACE);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-4.0f, -4.0f);
        
        glDepthMask(GL_FALSE);  
        glEnable(GL_DEPTH_TEST);
        // map texture coords based on face orientation
        GLfloat uvs[4][2];
        if (amtDy == 0.0f) {       // X–Z face
            uvs[0][0] = 0; uvs[0][1] = 0;
            uvs[1][0] = 1; uvs[1][1] = 0;
            uvs[2][0] = 1; uvs[2][1] = 1;
            uvs[3][0] = 0; uvs[3][1] = 1;
        } else if (amtDx == 0.0f) { // Y–Z face
            uvs[0][0] = 0; uvs[0][1] = 0;
            uvs[1][0] = 1; uvs[1][1] = 0;
            uvs[2][0] = 1; uvs[2][1] = 1;
            uvs[3][0] = 0; uvs[3][1] = 1;
        } else {                    // X–Y face
            uvs[0][0] = 0; uvs[0][1] = 0;
            uvs[1][0] = 1; uvs[1][1] = 0;
            uvs[2][0] = 1; uvs[2][1] = 1;
            uvs[3][0] = 0; uvs[3][1] = 1;
        }

        glBegin(GL_QUADS);
            glTexCoord2f(uvs[0][0], uvs[0][1]); glVertex3f(vA[0], vA[1], vA[2]);
            glTexCoord2f(uvs[1][0], uvs[1][1]); glVertex3f(vB[0], vB[1], vB[2]);
            glTexCoord2f(uvs[2][0], uvs[2][1]); glVertex3f(vC[0], vC[1], vC[2]);
            glTexCoord2f(uvs[3][0], uvs[3][1]); glVertex3f(vD[0], vD[1], vD[2]);
        glEnd();

        glDepthMask(GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_CULL_FACE);
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glPopMatrix();
        return;
    }




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

    glDepthMask(GL_TRUE);

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
        case BLOCK_TYPE_OUTLINE:
            sideTextureIndex   = BLOCK_TYPE_OUTLINE;
            topTextureIndex    = BLOCK_TYPE_OUTLINE;
            bottomTextureIndex = BLOCK_TYPE_OUTLINE;
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
    }

    if (selectedBlockToRender.active) {
        GLfloat translation[3];
        translation[0] = selectedBlockToRender.worldX;
        translation[1] = selectedBlockToRender.worldY;
        translation[2] = selectedBlockToRender.worldZ;
        GLfloat size[2] = {BlockWidthX, BlockLengthZ};


        float outlineScale = 1.f;
        float offset = (outlineScale - 1.0f) * 0.5f;

        translation[0] -= offset;
        translation[1] -= offset;
        translation[2] -= offset;

        size[0] *= outlineScale;
        size[1] *= outlineScale;

        for (int i = 1; i < 7; i++) {
            cubeFace(Vertices, translation, size, i, BLOCK_TYPE_OUTLINE);
        }

        glColor3f(1.0f, 1.0f, 1.0f);

    }
    

    


    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int windowWidth  = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    gluOrtho2D(0, windowWidth, windowHeight, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // draw 2d things
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
        glVertex2f(windowWidth/2, windowHeight/2 - 15);
        glVertex2f(windowWidth/2, windowHeight/2 + 15);
        glVertex2f(windowWidth/2 - 15, windowHeight/2);
        glVertex2f(windowWidth/2 + 15, windowHeight/2);
    glEnd();

    char text[64];
    snprintf(text, sizeof(text), "FPS: %.1f, Quads: %d", fps, chunkMeshQuads.amtQuads);

    glColor3f(1, 1, 1);
    drawText(text, 5, 15);

    glColor3f(1.0f, 1.0f, 1.0f); 
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    // switch the content of color and depth buffers
    glutSwapBuffers();
}