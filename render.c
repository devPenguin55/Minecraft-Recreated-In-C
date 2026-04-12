#include <stdint.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "render.h"
#include "input.h"
#include "chunks.h"
#include "chunkLoaderManager.h"
#include "raycast.h"

GLfloat T = 0;

GLuint destroyStageTextureArray[9];

double lastFpsTime = 0.0;
double lastTime = 0.0;
double fps = 0.0;
int frameCount = 0;

SelectedBlockToRender selectedBlockToRender;

GLuint worldVBO = 0;
GLuint worldVAO = 0;
GLuint waterVBO = 0;
GLuint waterVAO = 0;

Vertex *worldVertices = NULL;
int worldVertexCount = 0;
int worldVertexCapacity = 0;
Vertex *waterVertices = NULL;
int waterVertexCount = 0;
int waterVertexCapacity = 0;

GLuint blockTextureArray;
int GRASS_SIDE_TEXTURE_ARRAY_INDEX;
int GRASS_TOP_TEXTURE_ARRAY_INDEX;
int DIRT_TEXTURE_ARRAY_INDEX;
int STONE_TEXTURE_ARRAY_INDEX;
int WATER_TEXTURE_ARRAY_INDEX;
int SAND_TEXTURE_ARRAY_INDEX;
int OAK_SIDE_TEXTURE_ARRAY_INDEX;
int OAK_TOP_TEXTURE_ARRAY_INDEX;
int LEAVES_TEXTURE_ARRAY_INDEX;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

static inline float clamp(float value, float minVal, float maxVal)
{
    if (value < minVal)
        return minVal;
    if (value > maxVal)
        return maxVal;
    return value;
}

GLuint worldShader;
char* loadFile(const char* path)
{
    FILE* file = fopen(path,"rb");
    fseek(file,0,SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* data = malloc(size+1);
    fread(data,1,size,file);
    data[size]=0;
    fclose(file);

    return data;
}

GLuint compileShader(const char* path, GLenum type)
{
    char* src = loadFile(path);

    GLuint shader = glCreateShader(type);
    glShaderSource(shader,1,(const char**)&src,NULL);
    glCompileShader(shader);

    free(src);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        printf("Shader compile error: %s\n", log);
    }

    return shader;
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
        if (channels == 1)
        {
            unsigned char *rgbData = malloc(width * height * 3);
            for (int i = 0; i < width * height; i++)
            {
                rgbData[i * 3 + 0] = data[i];
                rgbData[i * 3 + 1] = data[i];
                rgbData[i * 3 + 2] = data[i];
            }
            gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, rgbData);
            free(rgbData);
        }
        else
        {
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

GLuint loadTextureArray(const char* filenames[], int count) {
    int width, height, channels;
    unsigned char* data = stbi_load(filenames[0], &width, &height, &channels, 0);
    if (!data) {
        printf("Failed to load texture %s\n", filenames[0]);
        return 0;
    }

    GLenum format = GL_RGBA;

    GLuint texArray;
    glGenTextures(1, &texArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);

    // Allocate the array
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, width, height, count, 0, format, GL_UNSIGNED_BYTE, NULL);

    // Load each layer
    for (int i = 0; i < count; i++) {
        unsigned char* layerData = stbi_load(filenames[i], &width, &height, &channels, 4);
        if (!layerData) { printf("Failed to load %s\n", filenames[i]); continue; }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, layerData);
        stbi_image_free(layerData);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return texArray;
}

int isCameraInWater() {
    int voxelX = (int)round(CameraX / BlockWidthX);
    int voxelY = (int)round(CameraY / BlockHeightY);
    int voxelZ = (int)round(CameraZ / BlockLengthZ);

    int chunkX = (voxelX >= 0) ? voxelX / ChunkWidthX : (voxelX - (ChunkWidthX-1)) / ChunkWidthX;
    int chunkZ = (voxelZ >= 0) ? voxelZ / ChunkLengthZ : (voxelZ - (ChunkLengthZ-1)) / ChunkLengthZ;

    voxelX = voxelX - chunkX * ChunkWidthX;
    voxelY = voxelY;
    voxelZ = voxelZ - chunkZ * ChunkLengthZ;
     
    uint64_t chunkKey = packChunkKey(chunkX, chunkZ);
    BucketEntry* result = getHashmapEntry(chunkKey);
    if (!result) { return 0; };

    Chunk *curChunk = result->chunkEntry;
    int index = voxelX + ChunkWidthX * voxelZ + (ChunkWidthX*ChunkLengthZ)*voxelY;

    Block *block = &curChunk->blocks[index];
    return (block != NULL && block->blockType == BLOCK_TYPE_WATER);
}

float getWaterScreenY(int windowHeight) {
    float waterY = (SEA_LEVEL > CameraY) ? windowHeight * 0.3f : windowHeight * 0.7f;

    return waterY;
}

void initGraphics()
{
    glClearColor(0, 0, 0, 1);
    glColor3f(1, 1, 1);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

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




    GLuint vs = compileShader("shader.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("shader.frag", GL_FRAGMENT_SHADER);

    worldShader = glCreateProgram();

    glAttachShader(worldShader, vs);
    glAttachShader(worldShader, fs);

    glBindAttribLocation(worldShader, 0, "position");
    glBindAttribLocation(worldShader, 1, "texCoord");
    glBindAttribLocation(worldShader, 2, "layer");
    glLinkProgram(worldShader);


    const char* blockTextures[] = {
        "assets\\grassSide.png",
        "assets\\grassTop.png",
        "assets\\dirt.png",
        "assets\\stone.png",
        "assets\\water.png",
        "assets\\sand.png",
        "assets\\oak_log.png",
        "assets\\oak_log_top.png",
        "assets\\leaves.png"
    };
    GRASS_SIDE_TEXTURE_ARRAY_INDEX = 0;
    GRASS_TOP_TEXTURE_ARRAY_INDEX = 1;
    DIRT_TEXTURE_ARRAY_INDEX = 2;
    STONE_TEXTURE_ARRAY_INDEX = 3;
    WATER_TEXTURE_ARRAY_INDEX = 4;
    SAND_TEXTURE_ARRAY_INDEX = 5;
    OAK_SIDE_TEXTURE_ARRAY_INDEX = 6;
    OAK_TOP_TEXTURE_ARRAY_INDEX = 7;
    LEAVES_TEXTURE_ARRAY_INDEX = 8;

    blockTextureArray = loadTextureArray(blockTextures, 9);
}

void reshape(int width, int height)
{
    if (height == 0)
    {
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
    GLfloat size[2])
{
    GLfloat vA[3] = {A[0], A[1], A[2]};
    GLfloat vB[3] = {B[0], B[1], B[2]};
    GLfloat vC[3] = {C[0], C[1], C[2]};
    GLfloat vD[3] = {D[0], D[1], D[2]};

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
    if (amtDy == 0.0f)
    {
        // X–Z face
        for (int i = 0; i < 4; i++)
        {
            GLfloat *v = (i == 0 ? vA : i == 1 ? vB
                                    : i == 2   ? vC
                                               : vD);
            v[0] = (v[0] + 0.5f) * size[0] - 0.5f;
            v[2] = (v[2] + 0.5f) * size[1] - 0.5f;
        }
    }
    else if (amtDx == 0.0f)
    {
        // Y–Z face
        for (int i = 0; i < 4; i++)
        {
            GLfloat *v = (i == 0 ? vA : i == 1 ? vB
                                    : i == 2   ? vC
                                               : vD);
            v[1] = (v[1] + 0.5f) * size[0] - 0.5f;
            v[2] = (v[2] + 0.5f) * size[1] - 0.5f;
        }
    }
    else
    {
        // X–Y face
        for (int i = 0; i < 4; i++)
        {
            GLfloat *v = (i == 0 ? vA : i == 1 ? vB
                                    : i == 2   ? vC
                                               : vD);
            v[0] = (v[0] + 0.5f) * size[0] - 0.5f;
            v[1] = (v[1] + 0.5f) * size[1] - 0.5f;
        }
    }

    glPushMatrix();
    glTranslatef(transformation[0], transformation[1], transformation[2]);
    glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);

    if ((int)texture == BLOCK_TYPE_SELECT)
    {
        
        // simple block outline, not a block breaking animation

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-2.0f, -2.0f); // slight offset to draw on top

        glColor4f(0.3f, 0.3f, 0.3f, 0.1f); // gray with 40% transparency

        glBegin(GL_QUADS);
            glVertex3fv(vA);
            glVertex3fv(vB);
            glVertex3fv(vC);
            glVertex3fv(vD);
        glEnd();

        glDisable(GL_POLYGON_OFFSET_FILL);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // reset color
        glEnable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        glPopMatrix();
        
        ///////////////////////////

        glPushMatrix();
        glTranslatef(transformation[0], transformation[1], transformation[2]);
        glScalef(BlockWidthX+0.01, BlockHeightY+0.01, BlockLengthZ+0.01);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.5f);

        glDepthMask(GL_FALSE);


        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Slightly expand the quad outward
        float expand = 1.f;  // 1% larger

        GLfloat center[3] = {
            (vA[0] + vC[0]) * 0.5f,
            (vA[1] + vC[1]) * 0.5f,
            (vA[2] + vC[2]) * 0.5f
        };

        GLfloat eA[3], eB[3], eC[3], eD[3];

        for (int i = 0; i < 3; i++) {
            eA[i] = center[i] + (vA[i] - center[i]) * expand;
            eB[i] = center[i] + (vB[i] - center[i]) * expand;
            eC[i] = center[i] + (vC[i] - center[i]) * expand;
            eD[i] = center[i] + (vD[i] - center[i]) * expand;
        }

        glLineWidth(2.0f);   // keep it 1px
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

        glDepthMask(GL_FALSE);   // draw on top but respect depth

        glBegin(GL_LINES);
            glVertex3fv(eA); glVertex3fv(eB);
            glVertex3fv(eB); glVertex3fv(eC);
            glVertex3fv(eC); glVertex3fv(eD);
            glVertex3fv(eD); glVertex3fv(eA);
        glEnd();

        glDepthMask(GL_TRUE);

        glDisable(GL_LINE_SMOOTH);
        glEnable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);

        glDepthMask(GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glPopMatrix();

        if (userBlockBreakingTimeElapsed == 0) {glDepthMask(GL_TRUE); return;}
        glPushMatrix();
        glTranslatef(transformation[0], transformation[1], transformation[2]);
        glScalef(BlockWidthX, BlockHeightY, BlockLengthZ);


        int stage = 9 * ((float)userBlockBreakingTimeElapsed / blockBreakingTimeByBlockType[beginBlockBreakingBlockType]);
        if (stage > 8)
            stage = 8;

        glEnable(GL_BLEND);
        glDepthMask(GL_FALSE); // don’t write depth
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, destroyStageTextureArray[stage]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // float crackAlpha = (stage / 9.0f);
        // crackAlpha = (crackAlpha > 1.0f) ? 1.0f : crackAlpha;
        float crackAlpha = 0.6;
        glColor4f(1.0f, 1.0f, 1.0f, crackAlpha);

        // Compute face normal from vertices
        GLfloat u[3], v[3], normal[3];
        for (int i = 0; i < 3; i++)
        {
            u[i] = vB[i] - vA[i];
            v[i] = vC[i] - vA[i];
        }
        normal[0] = u[1] * v[2] - u[2] * v[1];
        normal[1] = u[2] * v[0] - u[0] * v[2];
        normal[2] = u[0] * v[1] - u[1] * v[0];
        float len = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (len != 0.0f)
        {
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
        if (amtDy == 0.0f)
        { // X–Z face
            uvs[0][0] = 0;
            uvs[0][1] = 0;
            uvs[1][0] = 1;
            uvs[1][1] = 0;
            uvs[2][0] = 1;
            uvs[2][1] = 1;
            uvs[3][0] = 0;
            uvs[3][1] = 1;
        }
        else if (amtDx == 0.0f)
        { // Y–Z face
            uvs[0][0] = 0;
            uvs[0][1] = 0;
            uvs[1][0] = 1;
            uvs[1][1] = 0;
            uvs[2][0] = 1;
            uvs[2][1] = 1;
            uvs[3][0] = 0;
            uvs[3][1] = 1;
        }
        else
        { // X–Y face
            uvs[0][0] = 0;
            uvs[0][1] = 0;
            uvs[1][0] = 1;
            uvs[1][1] = 0;
            uvs[2][0] = 1;
            uvs[2][1] = 1;
            uvs[3][0] = 0;
            uvs[3][1] = 1;
        }

        glBegin(GL_QUADS);
        glTexCoord2f(uvs[0][0], uvs[0][1]);
        glVertex3f(vA[0], vA[1], vA[2]);
        glTexCoord2f(uvs[1][0], uvs[1][1]);
        glVertex3f(vB[0], vB[1], vB[2]);
        glTexCoord2f(uvs[2][0], uvs[2][1]);
        glVertex3f(vC[0], vC[1], vC[2]);
        glTexCoord2f(uvs[3][0], uvs[3][1]);
        glVertex3f(vD[0], vD[1], vD[2]);
        glEnd();

        glDepthMask(GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_CULL_FACE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glPopMatrix();
        return;
    }

    glEnd();
    glPopMatrix();
}

void cubeFace(GLfloat Vertices[8][3], GLfloat transformation[3], GLfloat size[2], int faceType, int blockType)
{
    int sideTextureIndex;
    int topTextureIndex;
    int bottomTextureIndex;

    switch (blockType)
    {
    case BLOCK_TYPE_SELECT:
        sideTextureIndex = BLOCK_TYPE_SELECT;
        topTextureIndex = BLOCK_TYPE_SELECT;
        bottomTextureIndex = BLOCK_TYPE_SELECT;
        break;
    default:
        printf("No correct block type entered!\n");
        break;
    }

    switch (faceType)
    {
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

void drawText(const char *text, float x, float y)
{
    glRasterPos2f(x, y);
    while (*text)
    {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *text);
        text++;
    }
}

void checkForWorldChunkVerticesDeletion() {
    int changedVBO = 0;
    for (int loadedChunkIdx = 0; loadedChunkIdx < chunkLoaderManager.loadedChunks.amtLoadedChunks; loadedChunkIdx++) {
        Chunk *loadedChunk = (chunkLoaderManager.loadedChunks.loadedChunks[loadedChunkIdx]);

        if (loadedChunk->triggerVertexDeletion) {
            loadedChunk->triggerVertexDeletion = 0;
            

            int amtVerticesInChunk = loadedChunk->lastVertex - loadedChunk->firstVertex + 1;
            if (loadedChunk->lastVertex != -1) {
                int start = loadedChunk->firstVertex;
                int end = loadedChunk->lastVertex + 1;

                int moveCount = worldVertexCount - end;

                if (moveCount > 0)
                {
                    memmove(&worldVertices[start],
                            &worldVertices[end],
                            moveCount * sizeof(Vertex));
                }
                
                worldVertexCount -= amtVerticesInChunk;
                changedVBO = 1;
            }

            int amtWaterVerticesInChunk = loadedChunk->lastWaterVertex - loadedChunk->firstWaterVertex + 1;
            if (loadedChunk->lastWaterVertex != -1) {
                int start = loadedChunk->firstWaterVertex;
                int end = loadedChunk->lastWaterVertex + 1;

                int moveCount = waterVertexCount - end;

                if (moveCount > 0)
                {
                    memmove(&waterVertices[start],
                            &waterVertices[end],
                            moveCount * sizeof(Vertex));
                }

                waterVertexCount -= amtWaterVerticesInChunk;
                changedVBO = 1;
            }



            

            for (int otherChunkIdx = 0; otherChunkIdx < chunkLoaderManager.loadedChunks.amtLoadedChunks; otherChunkIdx++) {
                Chunk *otherChunk = (chunkLoaderManager.loadedChunks.loadedChunks[otherChunkIdx]);
                if (otherChunk->hasVertices && otherChunk->lastVertex != -1 && loadedChunk->lastVertex != -1) {
                    if (otherChunk->firstVertex > loadedChunk->firstVertex) {
                        otherChunk->firstVertex -= amtVerticesInChunk;
                        otherChunk->lastVertex  -= amtVerticesInChunk;
                    }
                }
                if (otherChunk->hasWaterVertices && otherChunk->lastWaterVertex != -1 && loadedChunk->lastWaterVertex != -1) {
                    if (otherChunk->firstWaterVertex > loadedChunk->firstWaterVertex) {
                        otherChunk->firstWaterVertex -= amtWaterVerticesInChunk;
                        otherChunk->lastWaterVertex  -= amtWaterVerticesInChunk;
                    }
                }
            }

            loadedChunk->firstVertex = -1;
            loadedChunk->lastVertex  = -1;
            loadedChunk->firstWaterVertex = -1;
            loadedChunk->lastWaterVertex  = -1;
            loadedChunk->hasVertices = 0;
            loadedChunk->hasWaterVertices = 0;
        }
    }
    
    
    if (changedVBO) {
        uploadWorldMesh();
    }
}

void buildWorldMesh()
{
    checkForWorldChunkVerticesDeletion();

    int changedVBO = 0;
    for (int renderChunkIdx = 0; renderChunkIdx < chunkLoaderManager.renderChunks.amtRenderChunks; renderChunkIdx++) {
         Chunk *renderChunk = (chunkLoaderManager.renderChunks.renderChunks[renderChunkIdx]);

         if (!renderChunk->hasVertices && renderChunk->hasMesh) {
            renderChunk->hasVertices = 1;
            renderChunk->hasWaterVertices = 0;
            renderChunk->triggerVertexDeletion = 0;

            int firstQuadIndex = renderChunk->firstQuadIndex;
            int lastQuadIndex = renderChunk->lastQuadIndex;

            renderChunk->firstVertex = worldVertexCount;
            for (int i = firstQuadIndex; i < (lastQuadIndex+1); i++)
            {
                MeshQuad *q = &chunkMeshQuads.quads[i];
                if (q->blockType == BLOCK_TYPE_WATER || q->blockType == BLOCK_TYPE_LEAVES) { 
                    continue;
                }

                if (worldVertices == NULL) {
                    worldVertexCapacity = 1024;
                    worldVertices = malloc(sizeof(Vertex) * worldVertexCapacity);
                } else {
                    if ((worldVertexCount + 6) > worldVertexCapacity)
                    {
                        worldVertexCapacity = worldVertexCapacity * 2 + 1024;
                        worldVertices = realloc(worldVertices, sizeof(Vertex) * worldVertexCapacity);
                    }
                }

                float x = q->x;
                float y = q->y;
                float z = q->z;

                float w = q->width;
                float h = q->height;

                Vertex v0, v1, v2, v3;
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
                switch(q->faceType)
                {
                    case FACE_FRONT:
                        v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                        v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                        v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                        v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                        break;

                    case FACE_BACK:
                        v0 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], 0, 0};
                        v1 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], w, 0};
                        v2 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], w, h};
                        v3 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], 0, h};
                        break;

                    case FACE_LEFT:
                        v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                        v1 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], w, 0};
                        v2 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], w, h};
                        v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                        break;

                    case FACE_RIGHT:
                        v0 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], 0, 0};
                        v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                        v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                        v3 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], 0, h};
                        break;
                    case FACE_TOP:
                        v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                        v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                        v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                        v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                        break;

                    case FACE_BOTTOM:
                        v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                        v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                        v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                        v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                        break;
                }

                float minX = MIN4(v0.x,v1.x, v2.x, v3.x);
                float maxX = MAX4(v0.x,v1.x, v2.x, v3.x);
                float minY = MIN4(v0.y,v1.y, v2.y, v3.y);
                float maxY = MAX4(v0.y,v1.y, v2.y, v3.y);
                float minZ = MIN4(v0.z,v1.z, v2.z, v3.z);
                float maxZ = MAX4(v0.z,v1.z, v2.z, v3.z);

                float amtDx = maxX - minX;
                float amtDy = maxY - minY;
                float amtDz = maxZ - minZ;

                float size[2] = {w, h};
                // scale along face axes only

                int sideTextureIndex;
                int topTextureIndex;
                int bottomTextureIndex;

                switch (q->blockType)
                {
                case BLOCK_TYPE_GRASS:
                    sideTextureIndex = GRASS_SIDE_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = GRASS_TOP_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                    break;
                case BLOCK_TYPE_DIRT:
                    sideTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = DIRT_TEXTURE_ARRAY_INDEX;
                    break;
                case BLOCK_TYPE_STONE:
                    sideTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = STONE_TEXTURE_ARRAY_INDEX;
                    break;
                case BLOCK_TYPE_SAND:
                    sideTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = SAND_TEXTURE_ARRAY_INDEX;
                    break;
                case BLOCK_TYPE_OAK:
                    sideTextureIndex = OAK_SIDE_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = OAK_TOP_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = OAK_TOP_TEXTURE_ARRAY_INDEX;
                    break;
                case BLOCK_TYPE_LEAVES:
                    sideTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                    break;
                default:
                    printf("No correct block type entered!\n");
                    break;
                }


                if (amtDy == 0.0f)
                {
                    // X–Z face
                    for (int i = 0; i < 4; i++)
                    {
                        Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                                                : i == 2   ? &v2
                                                           : &v3);
                        v->x = (v->x + 0.5f) * size[0] - 0.5f;
                        v->z = (v->z + 0.5f) * size[1] - 0.5f;


                        v->u = v->x + 0.5f;
                        v->v = v->z + 0.5f;

                        v->layer = (q->faceType == FACE_TOP) ? topTextureIndex : bottomTextureIndex;          
                    }

                }
                else if (amtDx == 0.0f)
                {
                    // Y–Z face
                    for (int i = 0; i < 4; i++)
                    {
                        Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                        : i == 2   ? &v2
                                   : &v3);
                        v->y = (v->y + 0.5f) * size[0] - 0.5f;
                        v->z = (v->z + 0.5f) * size[1] - 0.5f;

                        v->u = v->z + 0.5f;
                        v->v = 1.0 - (v->y + 0.5f);

                        v->layer = sideTextureIndex;  
                    }
                }
                else
                {
                    // X–Y face
                    for (int i = 0; i < 4; i++)
                    {
                        Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                        : i == 2   ? &v2
                                   : &v3);
                        v->x = (v->x + 0.5f) * size[0] - 0.5f;
                        v->y = (v->y + 0.5f) * size[1] - 0.5f;

                        v->u = v->x + 0.5f;
                        v->v = 1.0 - (v->y + 0.5f);

                        v->layer = sideTextureIndex; 
                    } 
                }


                for (int i = 0; i < 4; i++)
                {
                    Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                    : i == 2   ? &v2
                               : &v3);

                    v->x += x;
                    v->y += y;
                    v->z += z;
                }

                worldVertices[worldVertexCount++] = v0;
                worldVertices[worldVertexCount++] = v1;
                worldVertices[worldVertexCount++] = v2;
                worldVertices[worldVertexCount++] = v0;
                worldVertices[worldVertexCount++] = v2;
                worldVertices[worldVertexCount++] = v3;
            }
            renderChunk->lastVertex = worldVertexCount-1;

            changedVBO = 1;
         }

         if (!renderChunk->hasWaterVertices && renderChunk->hasMesh) {
             renderChunk->hasWaterVertices = 1;
             renderChunk->triggerVertexDeletion = 0;

             int firstQuadIndex = renderChunk->firstQuadIndex;
             int lastQuadIndex = renderChunk->lastQuadIndex;


             

             renderChunk->firstWaterVertex = waterVertexCount;
             for (int i = firstQuadIndex; i < (lastQuadIndex+1); i++)
             {
                 MeshQuad *q = &chunkMeshQuads.quads[i];
                 if (q->blockType != BLOCK_TYPE_WATER || q->blockType != BLOCK_TYPE_LEAVES) { 
                     continue;
                 }

                 if ((q->y < (float)SEA_LEVEL-1)) {
                     continue;
                  }

                 if (waterVertices == NULL) {
                     waterVertexCapacity = 1024;
                     waterVertices = malloc(sizeof(Vertex) * waterVertexCapacity);
                 } else {
                     if (waterVertexCount + 6 > waterVertexCapacity)
                     {
                         waterVertexCapacity = waterVertexCapacity * 2 + 1024;
                         waterVertices = realloc(waterVertices, sizeof(Vertex) * waterVertexCapacity);
                     }
                 }
                
                 float x = q->x;
                 float y = q->y;
                 float z = q->z;

                 float w = q->width;
                 float h = q->height;

                 Vertex v0, v1, v2, v3;
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

                 switch(q->faceType)
                 {
                     case FACE_FRONT:
                         v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                         v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                         v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                         v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                         break;

                     case FACE_BACK:
                         v0 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], 0, 0};
                         v1 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], w, 0};
                         v2 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], w, h};
                         v3 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], 0, h};
                         break;

                     case FACE_LEFT:
                         v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                         v1 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], w, 0};
                         v2 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], w, h};
                         v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                         break;

                     case FACE_RIGHT:
                         v0 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], 0, 0};
                         v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                         v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                         v3 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], 0, h};
                         break;
                     case FACE_TOP:
                         v0 = (Vertex){Vertices[0][0], Vertices[0][1], Vertices[0][2], 0, 0};
                         v1 = (Vertex){Vertices[1][0], Vertices[1][1], Vertices[1][2], w, 0};
                         v2 = (Vertex){Vertices[5][0], Vertices[5][1], Vertices[5][2], w, h};
                         v3 = (Vertex){Vertices[4][0], Vertices[4][1], Vertices[4][2], 0, h};
                         break;

                     case FACE_BOTTOM:
                         v0 = (Vertex){Vertices[7][0], Vertices[7][1], Vertices[7][2], 0, 0};
                         v1 = (Vertex){Vertices[6][0], Vertices[6][1], Vertices[6][2], w, 0};
                         v2 = (Vertex){Vertices[2][0], Vertices[2][1], Vertices[2][2], w, h};
                         v3 = (Vertex){Vertices[3][0], Vertices[3][1], Vertices[3][2], 0, h};
                         break;
                 }

                 float minX = MIN4(v0.x,v1.x, v2.x, v3.x);
                 float maxX = MAX4(v0.x,v1.x, v2.x, v3.x);
                 float minY = MIN4(v0.y,v1.y, v2.y, v3.y);
                 float maxY = MAX4(v0.y,v1.y, v2.y, v3.y);
                 float minZ = MIN4(v0.z,v1.z, v2.z, v3.z);
                 float maxZ = MAX4(v0.z,v1.z, v2.z, v3.z);

                 

                 float amtDx = maxX - minX;
                 float amtDy = maxY - minY;
                 float amtDz = maxZ - minZ;

                 float size[2] = {w, h};
                 // scale along face axes only

                 int sideTextureIndex;   
                 int topTextureIndex;    
                 int bottomTextureIndex;
                 if (q->blockType == BLOCK_TYPE_WATER) {
                    sideTextureIndex = WATER_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = WATER_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = WATER_TEXTURE_ARRAY_INDEX;
                 } else {
                    sideTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                    topTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                    bottomTextureIndex = LEAVES_TEXTURE_ARRAY_INDEX;
                 }


                 if (amtDy == 0.0f)
                 {
                     // X–Z face
                     for (int i = 0; i < 4; i++)
                     {
                         Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                                                 : i == 2   ? &v2
                                                            : &v3);
                         v->x = (v->x + 0.5f) * size[0] - 0.5f;
                         v->z = (v->z + 0.5f) * size[1] - 0.5f;


                         v->u = v->x + 0.5f;
                         v->v = v->z + 0.5f;

                         v->layer = (q->faceType == FACE_TOP) ? topTextureIndex : bottomTextureIndex;          
                     }

                 }
                 else if (amtDx == 0.0f)
                 {
                     // Y–Z face
                     for (int i = 0; i < 4; i++)
                     {
                         Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                         : i == 2   ? &v2
                                    : &v3);
                         v->y = (v->y + 0.5f) * size[0] - 0.5f;
                         v->z = (v->z + 0.5f) * size[1] - 0.5f;

                         v->u = v->z + 0.5f;
                         v->v = 1.0 - (v->y + 0.5f);

                         v->layer = sideTextureIndex;  
                     }
                 }
                 else
                 {
                     // X–Y face
                     for (int i = 0; i < 4; i++)
                     {
                         Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                         : i == 2   ? &v2
                                    : &v3);
                         v->x = (v->x + 0.5f) * size[0] - 0.5f;
                         v->y = (v->y + 0.5f) * size[1] - 0.5f;

                         v->u = v->x + 0.5f;
                         v->v = 1.0 - (v->y + 0.5f);

                         v->layer = sideTextureIndex; 
                     } 
                 }


                 for (int i = 0; i < 4; i++)
                 {
                     Vertex *v = (i == 0 ? &v0 : i == 1 ? &v1
                     : i == 2   ? &v2
                                : &v3);

                     v->x += x;
                     v->y += y;
                     v->z += z;


                     if (v->y > 0.0f) // top vertices of cube
                     {
                         v->y -= 0.1;
                     }
                 }

                 waterVertices[waterVertexCount++] = v0;
                 waterVertices[waterVertexCount++] = v1;
                 waterVertices[waterVertexCount++] = v2;
                 waterVertices[waterVertexCount++] = v0;
                 waterVertices[waterVertexCount++] = v2;
                 waterVertices[waterVertexCount++] = v3;
             }
             renderChunk->lastWaterVertex = waterVertexCount-1;

             changedVBO = 1;
          }
    }

    

    if (changedVBO) {
        uploadWorldMesh();
    }

}



void uploadWorldMesh() {
    if (worldVAO == 0) {
        glGenVertexArrays(1, &worldVAO);
    }
    
    glBindVertexArray(worldVAO);
    
    if (worldVBO == 0) {
        glGenBuffers(1, &worldVBO);
    }

    glBindBuffer(GL_ARRAY_BUFFER, worldVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * worldVertexCount, worldVertices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // layer attribute location
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, layer)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // unbind


    // * ////////////////////////////////////////////////////////

    if (waterVAO == 0) {
        glGenVertexArrays(1, &waterVAO);
    }
    
    glBindVertexArray(waterVAO);
    
    if (waterVBO == 0) {
        glGenBuffers(1, &waterVBO);
    }

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * waterVertexCount, waterVertices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // layer attribute location
    glVertexAttribIPointer(2, 1, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, layer)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // unbind
}


void drawGraphics()
{
    frameCount++;
    double currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0; // seconds

    // update FPS every 0.25 seconds
    if (currentTime - lastFpsTime >= 0.25)
    {
        fps = frameCount / (currentTime - lastFpsTime);
        frameCount = 0;
        lastFpsTime = currentTime;
    }
    PLAYER_SPEED = 10 * (currentTime - lastTime);
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


    

    // clip = modelview * projection
    // clip = proj * modelview
    GLfloat clip[16];
    glGetFloatv(GL_PROJECTION_MATRIX, clip);

    GLfloat modelview[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

    glPushMatrix();
    glLoadMatrixf(clip);
    glMultMatrixf(modelview);
    glGetFloatv(GL_MODELVIEW_MATRIX, clip);
    glPopMatrix(); 

    
    buildWorldMesh();   // fills worldVertices and worldVertexCount

    glUseProgram(worldShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, blockTextureArray);
    glUniform1i(glGetUniformLocation(worldShader, "blockTextures"), 0);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    glBindVertexArray(worldVAO);
    glDrawArrays(GL_TRIANGLES, 0, worldVertexCount);
    glBindVertexArray(0);
    
    // glDisable(GL_BLEND);

    // * ////////////

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

      
    glDisable(GL_CULL_FACE); 
    glBindVertexArray(waterVAO);
    glDrawArrays(GL_TRIANGLES, 0, waterVertexCount);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE); 

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_2D_ARRAY);
    glDisable(GL_TEXTURE_3D);
    
    for (int i = 0; i < 16; i++)
        glDisableVertexAttribArray(i);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (selectedBlockToRender.active)
    {
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

        for (int i = 1; i < 7; i++)
        {
            cubeFace(Vertices, translation, size, i, BLOCK_TYPE_SELECT);
        }

        glColor3f(1.0f, 1.0f, 1.0f);
    } 
    glPopAttrib();
    
    
    glUseProgram(0);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
    int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

    gluOrtho2D(0, windowWidth, windowHeight, 0);
    

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    // draw 2d things
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(windowWidth / 2, windowHeight / 2 - 15);
    glVertex2f(windowWidth / 2, windowHeight / 2 + 15);
    glVertex2f(windowWidth / 2 - 15, windowHeight / 2);
    glVertex2f(windowWidth / 2 + 15, windowHeight / 2);
    glEnd();

    char text[64];
    snprintf(text, sizeof(text), "FPS: %.1f, Vertices: %d", fps, worldVertexCount+waterVertexCount);

    glColor3f(1, 1, 1);
    drawText(text, 5, 15);

    // glColor3f(1.0f, 1.0f, 1.0f);



    int inWater = isCameraInWater(); 
    if (inWater) { 
        glEnable(GL_BLEND); 
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
        float depthAlpha = MAX(MIN((SEA_LEVEL+2 - CameraY)/25, 0.55), 0.25);
        glColor4f(0.0f, 0.3f, 0.8f, depthAlpha); 
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(0, windowHeight);
        glVertex2f(windowWidth, windowHeight);
        glVertex2f(windowWidth, 0);
        glEnd();
        glDisable(GL_BLEND); 
    }





    glColor3f(1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);



    

    // switch the content of color and depth buffers
    glutSwapBuffers();
}