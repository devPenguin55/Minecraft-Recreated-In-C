#include <stdio.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

GLfloat T = 0;

void spinObject() {
    T =  T + 0.025;
    if (T > 360)
        T = 0;
    glutPostRedisplay();
}

void initGraphics() {
    glClearColor(0,0,0,1);
    glColor3f(1,0,0);
    glEnable(GL_DEPTH_TEST);
}

void face(GLfloat A[], GLfloat B[], GLfloat C[], GLfloat D[]) {
    // a face is a square
    glBegin(GL_POLYGON);
    glVertex3fv(A);
    glVertex3fv(B);
    glVertex3fv(C);
    glVertex3fv(D);
    glEnd();
}

void cube(GLfloat Vertices[8][3]) {
    // front
    glColor3f(1,0,0);
    face(Vertices[0], Vertices[1], Vertices[2], Vertices[3]);
    
    // back
    glColor3f(0,1,0);
    face(Vertices[4], Vertices[5], Vertices[6], Vertices[7]);
    
    // left
    glColor3f(0,0,1);
    face(Vertices[0], Vertices[3], Vertices[7], Vertices[4]);
    
    // right
    glColor3f(1,0,1);
    face(Vertices[1], Vertices[2], Vertices[6], Vertices[5]);

    // top
    glColor3f(1,1,0);
    face(Vertices[0], Vertices[1], Vertices[5], Vertices[4]);
    
    // bottom
    glColor3f(0,1,1);
    face(Vertices[3], Vertices[2], Vertices[6], Vertices[7]);
}

void drawGraphics() {
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
    glPointSize(5);
 
    glLoadIdentity();
    glRotatef(T,1,1,0);
    cube(Vertices);

    // switch the content of color and depth buffers 
    glutSwapBuffers();
}



int main(int argc, char *argv[]) {
    glutInit(&argc, argv);

    glutInitWindowSize(500, 500);
    glutInitWindowPosition(1000, 400);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); // rgb model, double buffer -> (color, depth)
    glutCreateWindow("Minecraft");

    initGraphics();

    glutDisplayFunc(drawGraphics);
    glutIdleFunc(spinObject);
    glutMainLoop();

    return 0;
}