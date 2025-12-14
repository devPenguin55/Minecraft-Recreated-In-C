#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/freeglut.h>
#include <math.h>
#include "input.h"
#include "render.h"
#include "chunks.h"
#include "chunkLoaderManager.h"

int main(int argc, char *argv[]) {
    initChunkLoaderManager();
    initWorld(&world);
    initChunkMeshingSystem();
    for (int i = 0; i<world.amtChunks; i++) {
        generateChunkMesh(&(world.chunks[i]), i);
    }

    glutInit(&argc, argv);
    
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(1000, 400);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); // rgb model, double buffer -> (color, depth)
    glutCreateWindow("Minecraft");
    glutSetCursor(GLUT_CURSOR_NONE);

    initGraphics();

    glutReshapeFunc(reshape);
    glutDisplayFunc(drawGraphics);
    glutKeyboardFunc(handleKeyDown);
    glutKeyboardUpFunc(handleKeyUp);
    glutIdleFunc(spinObject);
    glutMouseFunc(handleMouse);
    glutMotionFunc(handleMovingMouse);
    glutCloseFunc(handleProgramClose);
    glutMainLoop();

    return 0;
}