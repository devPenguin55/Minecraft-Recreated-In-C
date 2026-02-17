#ifndef INPUT_H
#define INPUT_H

#include <math.h>

extern int lastMouseX;
extern int lastMouseY;
extern int mouseInteractionStarted;
extern int pressedKeys[256];
extern GLfloat CameraX;
extern GLfloat CameraY;
extern GLfloat CameraZ;
extern GLfloat PlayerDirX;
extern GLfloat PlayerDirY;
extern GLfloat PlayerDirZ;
extern float PLAYER_SPEED;
extern int userBlockBreakingTimeElapsed;
extern int beginBlockBreakingBlockType;

void toggleFullscreen();
void handleKeyDown(unsigned char key, int x, int y);
void handleKeyUp(unsigned char key, int x, int y);
void handleMouse(int button, int state, int x, int y);
void handleMovingMouse(int x, int y);
void handleUserMovement();

#endif