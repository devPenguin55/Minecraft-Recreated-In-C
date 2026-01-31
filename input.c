#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "input.h"
#include "render.h"
#include "chunkLoaderManager.h"


GLfloat CameraX = 33.3;
GLfloat CameraY = 63;//63;
GLfloat CameraZ = 0;
GLfloat PlayerDirX = -5;
GLfloat PlayerDirY = 0;
GLfloat PlayerDirZ = -1;
float PLAYER_SPEED = 0.005;

float rightMouseHeldDuration = 0.0f;


float yaw   = 0.0f; // horizontal rotation
float pitch = 0.0f; // vertical   rotation

int lastMouseX = 0;
int lastMouseY = 0;
int mouseInteractionStarted = 0;

int pressedKeys[256] = {0};


void handleKeyDown(unsigned char key, int x, int y) {
    pressedKeys[key] = 1;
}

void handleKeyUp(unsigned char key, int x, int y) {
    pressedKeys[key] = 0;
}



void handleMouse(int button, int state, int x, int y) {
    // button: GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, etc.
    // state: GLUT_DOWN or GLUT_UP
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        lastMouseX = x;
        lastMouseY = y;

        mouseInteractionStarted = 1;
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        mouseInteractionStarted = 0;
    }

    if ((button == GLUT_RIGHT_BUTTON || button == GLUT_MIDDLE_BUTTON) && state == GLUT_DOWN) {
        int x,y,z;
        x = selectedBlockToRender.localX;
        y = selectedBlockToRender.localY;
        z = selectedBlockToRender.localZ;
        
        int chunkXUnit =   ChunkWidthX * BlockWidthX;
        int chunkZUnit = ChunkLengthZ * BlockLengthZ;
        
        if (button == GLUT_RIGHT_BUTTON) {
            selectedBlockToRender.chunk->blocks[x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y].isAir = 1;
        } else {
            int blockIndex;

            if (selectedBlockToRender.hitFace == FACE_TOP) {
                blockIndex = x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*(y+1);
                if ((y+1) >= ChunkHeightY) {
                    return;
                }
                selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                

            } else if (selectedBlockToRender.hitFace == FACE_BOTTOM) {
                blockIndex = x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*(y-1);
                if ((y-1) < 0) {
                    return;
                }
                selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;

            } else if (selectedBlockToRender.hitFace == FACE_LEFT) {
                if ((x-1) >= 0) {
                    blockIndex = x-1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                } else {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX - chunkXUnit) / (chunkXUnit)), 
                        (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit))
                    );
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = ChunkWidthX-1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }

            } else if (selectedBlockToRender.hitFace == FACE_RIGHT) {
                if ((x+1) < ChunkWidthX) {
                    blockIndex = x+1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                } else {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX + chunkXUnit) / (chunkXUnit)), 
                        (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit))
                    );
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = 0 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }

            } else if (selectedBlockToRender.hitFace == FACE_FRONT) {
                if ((z + 1) < ChunkLengthZ) {
                    blockIndex = x + (ChunkWidthX)*(z+1) + (ChunkWidthX * ChunkLengthZ)*y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                } else {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)), 
                        (int)((selectedBlockToRender.chunk->chunkStartZ + chunkZUnit) / (chunkZUnit))
                    );
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = x + (ChunkWidthX)*(0) + (ChunkWidthX * ChunkLengthZ)*y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }

            } else if (selectedBlockToRender.hitFace == FACE_BACK) {
                if ((z-1) >= 0) {
                    blockIndex = x + (ChunkWidthX)*(z-1) + (ChunkWidthX * ChunkLengthZ)*y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                } else {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)), 
                        (int)((selectedBlockToRender.chunk->chunkStartZ - chunkZUnit) / (chunkZUnit))
                    );
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = x + (ChunkWidthX)*(ChunkLengthZ - 1) + (ChunkWidthX * ChunkLengthZ)*y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }
            }
        }
        
        
        if (x == 0 ) {
            uint64_t chunkKey = packChunkKey(
                (int)((selectedBlockToRender.chunk->chunkStartX - chunkXUnit) / (chunkXUnit)), 
                (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit))
            );
            BucketEntry *result = getHashmapEntry(chunkKey);
            triggerRenderChunkRebuild(result->chunkEntry);
        } else if (x == ChunkWidthX-1) {
            uint64_t chunkKey = packChunkKey(
                (int)((selectedBlockToRender.chunk->chunkStartX + chunkXUnit) / (chunkXUnit)), 
                (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit))
            );
            BucketEntry *result = getHashmapEntry(chunkKey);
            triggerRenderChunkRebuild(result->chunkEntry);
        }

        if (z == 0 ) {
            uint64_t chunkKey = packChunkKey(
                (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)), 
                (int)((selectedBlockToRender.chunk->chunkStartZ - chunkZUnit) / (chunkZUnit))
            );
            BucketEntry *result = getHashmapEntry(chunkKey);
            triggerRenderChunkRebuild(result->chunkEntry);
        } else if (z == ChunkLengthZ-1) {
            uint64_t chunkKey = packChunkKey(
                (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)), 
                (int)((selectedBlockToRender.chunk->chunkStartZ + chunkZUnit) / (chunkZUnit))
            );
            BucketEntry *result = getHashmapEntry(chunkKey);
            triggerRenderChunkRebuild(result->chunkEntry);
        }

        triggerRenderChunkRebuild(selectedBlockToRender.chunk);
    } 
}

void handleMovingMouse(int x, int y) {
    if (!mouseInteractionStarted) { return; }

    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    yaw += dx * 0.2f;
    pitch += -dy * 0.2f;
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    // printf("%f %f\n", yaw, pitch);
    PlayerDirX = cosf(pitch * M_PI/180) * sinf(yaw * M_PI/180);
    PlayerDirY = sinf(pitch * M_PI/180);
    PlayerDirZ = -cosf(pitch * M_PI/180) * cosf(yaw * M_PI/180);

    lastMouseX = x;
    lastMouseY = y;

    glutPostRedisplay();
}

void handleUserMovement() {
    GLfloat RightX =  PlayerDirZ;   // perpendicular on XZ plane
    GLfloat RightZ = -PlayerDirX;

    if (pressedKeys['w']) {
        CameraX += PlayerDirX * PLAYER_SPEED;
        CameraZ += PlayerDirZ * PLAYER_SPEED;
    }
    if (pressedKeys['s']) {
        CameraX -= PlayerDirX * PLAYER_SPEED;
        CameraZ -= PlayerDirZ * PLAYER_SPEED;
    }     
    if (pressedKeys['d']) {
        CameraX -= RightX * PLAYER_SPEED;
        CameraZ -= RightZ * PLAYER_SPEED;
    }
    if (pressedKeys['a']) {
        CameraX += RightX * PLAYER_SPEED;
        CameraZ += RightZ * PLAYER_SPEED;
    }
    if (pressedKeys['r']) {
        CameraY += 0.8 * PLAYER_SPEED;
    }   
    if (pressedKeys['f']) {
        CameraY -= 0.8 * PLAYER_SPEED;
    }
}