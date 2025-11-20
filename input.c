#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include "input.h"
GLfloat CameraX = 0.5;
GLfloat CameraY = 3;
GLfloat CameraZ = 1;
GLfloat PlayerDirX = 0;
GLfloat PlayerDirY = -3;
GLfloat PlayerDirZ = -1;
float PLAYER_SPEED = 0.0005;

float yaw   = 132.0f; // horizontal rotation
float pitch = -56.0f; // vertical   rotation

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
        CameraY += PLAYER_SPEED;
    }   
    if (pressedKeys['f']) {
        CameraY -= PLAYER_SPEED;
    }
}