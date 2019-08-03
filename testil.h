#include <IL/il.h>
#include <GL/glut.h>

#ifndef TESTIL_H_   
#define TESTIL_H_

void send_tiles();
void read_pixels();
void display();
void reshape(GLsizei newwidth, GLsizei newheight);
void init_GL(int w, int h);
int load_image();
bool parseCommandLine(int ac, const char **av); 



#endif // TESTIL_H_