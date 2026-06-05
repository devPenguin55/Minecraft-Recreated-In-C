#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "input.h"
#include "render.h"
#include "chunkLoaderManager.h"
#include "player.h"

GLfloat PlayerDirX = -5;
GLfloat PlayerDirY = 0;
GLfloat PlayerDirZ = -1;
Player player = (Player){
    .position.x = 33.3,
    .position.y = 100,
    .position.z = 0,
    .velocity.x = 0,
    .velocity.y = 0,
    .velocity.z = 0,
    .isOnGround = 0,
    .isInWater = 0,
    .width = 0.75f,
    .height = 1.8f};
GLfloat EYE_HEIGHT_OFFSET = 1.62f;
GLfloat eyeX;
GLfloat eyeY;
GLfloat eyeZ;

int DEV_MODE = 0;

float DELTA_TIME = 0.001;

float yaw = 0.0f;   // horizontal rotation
float pitch = 0.0f; // vertical   rotation

int pressedKeys[256] = {0};

int isFullscreen = 0;
float userBlockBreakingTimeElapsed = -1.f;
int beginBlockBreakingIndex = -1;
int beginBlockBreakingBlockType = -1;

void playerInit()
{
    eyeX = player.position.x;
    eyeY = player.position.y + EYE_HEIGHT_OFFSET;
    eyeZ = player.position.z;
}

void toggleFullscreen()
{
    if (!isFullscreen)
    {
        glutFullScreen(); // go fullscreen
        isFullscreen = 1;
    }
    else
    {
        glutReshapeWindow(500, 500);   // restore windowed size
        glutPositionWindow(1000, 400); // optional
        isFullscreen = 0;
    }
}

void handleKeyDown(unsigned char key, int x, int y)
{
    pressedKeys[key] = 1;

    if (key == 'q')
    {
        int z = 5;
        z = z / (z - 5);
    }

    if (key == 'm')
    {
        toggleFullscreen();
    }
}

void handleKeyUp(unsigned char key, int x, int y)
{
    pressedKeys[key] = 0;
}

void handleMouse(int button, int state, int x_, int y_)
{
    // button: GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, etc.
    // state: GLUT_DOWN or GLUT_UP

    if ((button == GLUT_RIGHT_BUTTON || button == GLUT_LEFT_BUTTON) && state == GLUT_DOWN)
    {
        if (!selectedBlockToRender.active)
        {
            return;
        }

        int x, y, z;
        x = selectedBlockToRender.localX;
        y = selectedBlockToRender.localY;
        z = selectedBlockToRender.localZ;

        int chunkXUnit = ChunkWidthX * BlockWidthX;
        int chunkZUnit = ChunkLengthZ * BlockLengthZ;

        if (button == GLUT_LEFT_BUTTON)
        {
            // break blocks
            userBlockBreakingTimeElapsed = 0.0f;
            beginBlockBreakingIndex = (x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * y);
            beginBlockBreakingBlockType = selectedBlockToRender.chunk->blocks[beginBlockBreakingIndex].blockType;
        }
        else
        {
            // place blocks

            if (hotbarBlocks[hotbarActiveSlot] == -1)
            {
                return;
            }

            int blockIndex;
            if (selectedBlockToRender.hitFace == FACE_TOP)
            {
                blockIndex = x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * (y + 1);
                if ((y + 1) >= ChunkHeightY)
                {
                    return;
                }
                selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                if (playerCollides(&player))
                {
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                    return;
                }
                selectedBlockToRender.chunk->isDirty = 1;
            }
            else if (selectedBlockToRender.hitFace == FACE_BOTTOM)
            {
                blockIndex = x + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * (y - 1);
                if ((y - 1) < 0)
                {
                    return;
                }
                selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                if (playerCollides(&player))
                {
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                    return;
                }

                selectedBlockToRender.chunk->isDirty = 1;
            }
            else if (selectedBlockToRender.hitFace == FACE_LEFT)
            {
                if ((x - 1) >= 0)
                {
                    blockIndex = x - 1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                        selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                }
                else
                {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX - chunkXUnit) / (chunkXUnit)),
                        (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit)));
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = ChunkWidthX - 1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    result->chunkEntry->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        result->chunkEntry->blocks[blockIndex].isAir = 1;
                        result->chunkEntry->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }
            }
            else if (selectedBlockToRender.hitFace == FACE_RIGHT)
            {
                if ((x + 1) < ChunkWidthX)
                {
                    blockIndex = x + 1 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                        selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                }
                else
                {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX + chunkXUnit) / (chunkXUnit)),
                        (int)((selectedBlockToRender.chunk->chunkStartZ) / (chunkZUnit)));
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = 0 + (ChunkWidthX)*z + (ChunkWidthX * ChunkLengthZ) * y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    result->chunkEntry->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];
                    if (playerCollides(&player))
                    {
                        result->chunkEntry->blocks[blockIndex].isAir = 1;
                        result->chunkEntry->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }
            }
            else if (selectedBlockToRender.hitFace == FACE_FRONT)
            {
                if ((z + 1) < ChunkLengthZ)
                {
                    blockIndex = x + (ChunkWidthX) * (z + 1) + (ChunkWidthX * ChunkLengthZ) * y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                        selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                }
                else
                {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)),
                        (int)((selectedBlockToRender.chunk->chunkStartZ + chunkZUnit) / (chunkZUnit)));
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = x + (ChunkWidthX) * (0) + (ChunkWidthX * ChunkLengthZ) * y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    result->chunkEntry->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        result->chunkEntry->blocks[blockIndex].isAir = 1;
                        result->chunkEntry->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }
            }
            else if (selectedBlockToRender.hitFace == FACE_BACK)
            {
                if ((z - 1) >= 0)
                {
                    blockIndex = x + (ChunkWidthX) * (z - 1) + (ChunkWidthX * ChunkLengthZ) * y;
                    selectedBlockToRender.chunk->blocks[blockIndex].isAir = 0;
                    selectedBlockToRender.chunk->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        selectedBlockToRender.chunk->blocks[blockIndex].isAir = 1;
                        selectedBlockToRender.chunk->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                }
                else
                {
                    uint64_t chunkKey = packChunkKey(
                        (int)((selectedBlockToRender.chunk->chunkStartX) / (chunkXUnit)),
                        (int)((selectedBlockToRender.chunk->chunkStartZ - chunkZUnit) / (chunkZUnit)));
                    BucketEntry *result = getHashmapEntry(chunkKey);
                    blockIndex = x + (ChunkWidthX) * (ChunkLengthZ - 1) + (ChunkWidthX * ChunkLengthZ) * y;
                    result->chunkEntry->blocks[blockIndex].isAir = 0;
                    result->chunkEntry->blocks[blockIndex].blockType = hotbarBlocks[hotbarActiveSlot];

                    if (playerCollides(&player))
                    {
                        result->chunkEntry->blocks[blockIndex].isAir = 1;
                        result->chunkEntry->blocks[blockIndex].blockType = -1;
                        return;
                    }
                    selectedBlockToRender.chunk->isDirty = 1;
                    triggerRenderChunkRebuild(result->chunkEntry);
                }
            }
            triggerRenderChunkRebuild(selectedBlockToRender.chunk);
        }
    }
    else
    {
        userBlockBreakingTimeElapsed = -1.f;
    }
}

void handleMovingMouse(int x, int y)
{
    int windowCenterX = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int windowCenterY = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    int dx = x - windowCenterX;
    int dy = y - windowCenterY;

    yaw += dx * 0.2f;
    pitch += -dy * 0.2f;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    // printf("%f %f\n", yaw, pitch);
    PlayerDirX = cosf(pitch * M_PI / 180) * sinf(yaw * M_PI / 180);
    PlayerDirY = sinf(pitch * M_PI / 180);
    PlayerDirZ = -cosf(pitch * M_PI / 180) * cosf(yaw * M_PI / 180);

    glutPostRedisplay();
    glutWarpPointer(windowCenterX, windowCenterY);

    
}

void handleUserMovement()
{
    GLfloat RightX = PlayerDirZ; // perpendicular on XZ plane
    GLfloat RightZ = -PlayerDirX;

    player.velocity.x = 0;
    player.velocity.z = 0;
    float moveX = 0.0f;
    float moveZ = 0.0f;
    if (pressedKeys['w'])
    {
        moveX += PlayerDirX;
        moveZ += PlayerDirZ;
    }
    if (pressedKeys['s'])
    {
        moveX -= PlayerDirX;
        moveZ -= PlayerDirZ;
    }
    if (pressedKeys['d'])
    {
        moveX -= RightX;
        moveZ -= RightZ;
    }
    if (pressedKeys['a'])
    {
        moveX += RightX;
        moveZ += RightZ;
    }

    // if (DEV_MODE) {
    if (pressedKeys['r'])
    {
        player.velocity.y = 5.0f;
    }
    if (pressedKeys['f'])
    {
        player.velocity.y = 0.0f;
    }
    // } else {
    if (pressedKeys[' '] && (player.isOnGround || player.isInWater))
    {
        player.velocity.y = 7.5f;
        player.isOnGround = 0;
    }
    // }

    float length = sqrtf(moveX * moveX + moveZ * moveZ);

    if (length > 0.0f)
    {
        moveX /= length;
        moveZ /= length;
    }
    player.velocity.x = moveX * 5;
    player.velocity.z = moveZ * 5;

    // hotbar selection (keys 1–9)
    if (pressedKeys['1'])
        hotbarActiveSlot = 0;
    if (pressedKeys['2'])
        hotbarActiveSlot = 1;
    if (pressedKeys['3'])
        hotbarActiveSlot = 2;
    if (pressedKeys['4'])
        hotbarActiveSlot = 3;
    if (pressedKeys['5'])
        hotbarActiveSlot = 4;
    if (pressedKeys['6'])
        hotbarActiveSlot = 5;
    if (pressedKeys['7'])
        hotbarActiveSlot = 6;
    if (pressedKeys['8'])
        hotbarActiveSlot = 7;
    if (pressedKeys['9'])
        hotbarActiveSlot = 8;
}