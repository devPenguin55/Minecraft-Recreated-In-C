#ifndef INPUT_H
#define INPUT_H

#include <math.h>
#include "player.h"

extern int lastMouseX;
extern int lastMouseY;
extern int mouseInteractionStarted;
extern int pressedKeys[256];
extern GLfloat PlayerDirX;
extern GLfloat PlayerDirY;
extern GLfloat PlayerDirZ;
extern Player player;
extern float DELTA_TIME;
extern float userBlockBreakingTimeElapsed;
extern int beginBlockBreakingBlockType;
extern GLfloat EYE_HEIGHT_OFFSET;
extern int beginBlockBreakingIndex;
extern GLfloat eyeX;
extern GLfloat eyeY;
extern GLfloat eyeZ;

void toggleFullscreen();
void handleKeyDown(unsigned char key, int x, int y);
void handleKeyUp(unsigned char key, int x, int y);
void blockPlacingOrBreakingLightingRecalculation();
void handleMouse(int button, int state, int x, int y);
void handleMovingMouse(int x, int y);
void handleUserMovement();

#endif