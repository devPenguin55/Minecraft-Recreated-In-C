#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "input.h"
#include "render.h"
#include "chunkLoaderManager.h"


GLfloat CameraX = 33.3;
GLfloat CameraY = 50;//63;
GLfloat CameraZ = 0;
GLfloat PlayerDirX = -5;
GLfloat PlayerDirY = 0;
GLfloat PlayerDirZ = -1;
float PLAYER_SPEED = 0.005;

float yaw   = 0.0f; // horizontal rotation
float pitch = 0.0f; // vertical   rotation


int pressedKeys[256] = {0};

int isFullscreen = 0;
int userBlockBreakingTimeElapsed = 0;
int beginBlockBreakingIndex = -1;

void toggleFullscreen() {
    if (!isFullscreen) {
        glutFullScreen();   // go fullscreen
        isFullscreen = 1;
    } else {
        glutReshapeWindow(500, 500);  // restore windowed size
        glutPositionWindow(1000, 400); // optional
        isFullscreen = 0;
    }
}

void handleKeyDown(unsigned char key, int x, int y) {
    pressedKeys[key] = 1;

    if (key == 'q') {
        int z = 5;
        z = z/(z-5);
    }

    if (key == 'e') {
        toggleFullscreen();
    }
}

void handleKeyUp(unsigned char key, int x, int y) {
    pressedKeys[key] = 0;
}


void handleMouse(int button, int state, int x_, int y_) {
    // button: GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, etc.
    // state: GLUT_DOWN or GLUT_UP

    if ((button == GLUT_RIGHT_BUTTON || button == GLUT_LEFT_BUTTON) && state == GLUT_DOWN) {
        if (!selectedBlockToRender.active) {
            return;
        }
        int x,y,z;
        x = selectedBlockToRender.localX;
        y = selectedBlockToRender.localY;
        z = selectedBlockToRender.localZ;
        
        int chunkXUnit =   ChunkWidthX * BlockWidthX;
        int chunkZUnit = ChunkLengthZ * BlockLengthZ;
        
        if (button == GLUT_LEFT_BUTTON) {
            // break blocks
            userBlockBreakingTimeElapsed = 1;
            beginBlockBreakingIndex = (x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y);
        } else {
            // place blocks
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
    int windowCenterX = glutGet(GLUT_WINDOW_WIDTH)  / 2;
    int windowCenterY = glutGet(GLUT_WINDOW_HEIGHT) / 2; 

    int dx = x - windowCenterX;
    int dy = y - windowCenterY;

    yaw += dx * 0.2f;
    pitch += -dy * 0.2f;
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    // printf("%f %f\n", yaw, pitch);
    PlayerDirX = cosf(pitch * M_PI/180) * sinf(yaw * M_PI/180);
    PlayerDirY = sinf(pitch * M_PI/180);
    PlayerDirZ = -cosf(pitch * M_PI/180) * cosf(yaw * M_PI/180);

    glutPostRedisplay();
    glutWarpPointer(windowCenterX, windowCenterY);

    if (userBlockBreakingTimeElapsed >= 1) {
        userBlockBreakingTimeElapsed++;
        
        if (!selectedBlockToRender.active) {
            return;
        }
        int x,y,z;
        x = selectedBlockToRender.localX;
        y = selectedBlockToRender.localY;
        z = selectedBlockToRender.localZ;
        
        int chunkXUnit =   ChunkWidthX * BlockWidthX;
        int chunkZUnit = ChunkLengthZ * BlockLengthZ;

        // break blocks
        int blockType = selectedBlockToRender.chunk->blocks[x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y].blockType;

        if (userBlockBreakingTimeElapsed >= 150 && beginBlockBreakingIndex == (x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y)) {
            selectedBlockToRender.chunk->blocks[x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y].isAir = 1;
            userBlockBreakingTimeElapsed = 0;
            triggerRenderChunkRebuild(selectedBlockToRender.chunk);
        } else {
            if (beginBlockBreakingIndex == -1) {
                beginBlockBreakingIndex = (x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y);
            } else if (beginBlockBreakingIndex != (x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ)*y)) {
                userBlockBreakingTimeElapsed = 0;
            }
            return;
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
    }
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