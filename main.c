#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

void initGraphics() {
    glClearColor(0,0,0,1);
    glColor3f(1,0,0);
}

void drawGraphics() {
    // clear color buffer to clear background, uses preset color setup in initGraphics
    glClear(GL_COLOR_BUFFER_BIT); 

    glPointSize(5);
    
    // makes a group
    glBegin(GL_POLYGON);
        glVertex2f(-0.5, 0.5);
        glVertex2f(0.5, 0.5);
        glVertex2f(0.5, -0.5);
        glVertex2f(-0.5, -0.5);
    glEnd();


    // immediately pushes the changes to buffer and screen
    glFlush();
}



int main(int argc, char *argv[]) {
    glutInit(&argc, argv);

    glutInitWindowSize(500, 500);
    glutInitWindowPosition(1000, 400);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutCreateWindow("Minecraft");

    initGraphics();

    glutDisplayFunc(drawGraphics);
    glutMainLoop();

    return 0;
}