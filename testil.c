#include <IL/il.h>
#include <GL/glut.h>
#include <stdbool.h>
//#include <mpi.h>



#define DEFAULT_WIDTH  1024
#define DEFAULT_HEIGHT 768

int tile_size = 64;
int width  = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;
int rows = 2;
int col = 2; 
char* filename;
bool print = false;
unsigned char* pixels;

typedef struct{
    int x,y;
    unsigned char* pix_buf;
}Tile;

void send_tiles(){
  int pixels_offset = 0;
  for(int y=0; y<(height/tile_size); y++){
    for(int x=0; x<(width/tile_size); x++){
      Tile tile_obj;
      tile_obj.x = (x*tile_size);
      tile_obj.y = (y*tile_size);
      printf("Init position of tile: X: %d , Y: %d \n", tile_obj.x, tile_obj.y);
      tile_obj.pix_buf = (unsigned char*)malloc(tile_size*tile_size*4);
      int pix_line = 0;

      for(int j=(y*tile_size); j<((y+1)*tile_size); j++){
        printf("Y position: %d, X position: %d \n" , j, x*tile_size);
        memcpy(&tile_obj.pix_buf[pix_line*tile_size*4], &pixels[pixels_offset], sizeof(unsigned char)*4*tile_size);
        // for(int l = 0; l <tile_size*4; l++){
        //   printf("PixBuff[%d]: %d \n", l ,tile_obj.pix_buf[l+(pix_line*tile_size*4)]);
        //   printf("Pixels[%d]: %d \n", l ,pixels[l+pixels_offset]);
        // }
        pix_line++;
        pixels_offset = pixels_offset + (4*tile_size);  
      }
      
      printf("Tile in position: X: %d , Y: %d created. \n\n", tile_obj.x, tile_obj.y);
      
    }  
  }
}

/* read_pixels works for getting all the rgba values from the framebuffer that is bind to the window.
 It stores the values in a global GLubyte array called pixels and then prints the values in console*/
void read_pixels(){
  pixels = (unsigned char*)malloc(height*width*4);
  glReadBuffer(GL_FRONT);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  if(print){
    for(int i = 0; i < width; i++)
    {
      for(int j = 0; j < height; j++)
      {
        printf("X: %d , Y: %d \n", i, j);
        printf( "R: %d ", (int)pixels[offset(i,j,0)] );

        printf( "G: %d ", (int)pixels[offset(i,j,1)] );

        printf( "B: %d ", (int)pixels[offset(i,j,2)] ); 

        printf( "A: %d \n", (int)pixels[offset(i,j,3)] );

        printf( "============================================= \n");
      }
    }
  }
  send_tiles();
}

int offset(int x, int y, int z) { 
    return (z * width * height) + (y * width) + x; 
}

/* Handler for window-repaint event. Called back when the window first appears and
   whenever the window needs to be re-painted. */
void display() 
{
    // Clear color and depth buffers
       glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
       glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

    /* Draw a quad */
       glBegin(GL_QUADS);
           glTexCoord2i(0, 0); glVertex2i(0,   0);
           glTexCoord2i(0, 1); glVertex2i(0,   height);
           glTexCoord2i(1, 1); glVertex2i(width, height);
           glTexCoord2i(1, 0); glVertex2i(width, 0);
       glEnd();
    
    glutSwapBuffers();
    read_pixels();
} 

/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei newwidth, GLsizei newheight) 
{  
    // Set the viewport to cover the new window
       glViewport(0, 0, width=newwidth, height=newheight);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     glOrtho(0.0, width, height, 0.0, 0.0, 100.0);
     glMatrixMode(GL_MODELVIEW);

    glutPostRedisplay();
}


  
/* Initialize OpenGL Graphics */
void init_GL(int w, int h) 
{
     glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT
     glEnable(GL_TEXTURE_2D);     // Enable 2D texturing
 
    glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
     glLoadIdentity();
     glOrtho(0.0, w, h, 0.0, 0.0, 100.0);

     glMatrixMode(GL_MODELVIEW);    // Set the matrix mode to object modeling

     glClearColor(0.0f, 0.0f, 0.0f, 0.0f); 
     glClearDepth(0.0f);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
}

/* Load an image using DevIL and return the devIL handle (-1 if failure) */
int load_image()
{
    ILboolean success; 
    ILuint image; 

    ilGenImages(1, &image); /* Generation of one image name */
    ilBindImage(image); /* Binding of image name */
    success = ilLoadImage(filename); /* Loading of the image filename by DevIL */
     
    if (success) /* If no error occured: */
    {
        /* Convert every colour component into unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
        success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE); 
        if (!success)
           {
              return -1;
           }
        
    }
    else
        return -1;

    return image;
}

bool parseCommandLine(int ac, const char **av)
  {
    bool file = false;
    for (int i = 0; i < ac; ++i) {
      char* arg = av[i];
      if (strcmp(arg, "-w") == 0 || strcmp(arg, "-width") == 0) {
        width = atoi(av[++i]);
      } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "-height") == 0) {
        height = atoi(av[++i]);
      } else if (strcmp(arg, "-f") == 0 || strcmp(arg, "-file") == 0){
        file = true; 
        filename = av[++i];
      } else if (strcmp(arg, "-c") == 0 || strcmp(arg, "-cols") == 0) {
        col = atoi(av[++i]);
      } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "-rows") == 0) {
        rows = atoi(av[++i]);
      } else if (strcmp(arg, "-p") == 0) {
        print = true;
      } else if (strcmp(arg, "-ts") == 0) {
        tile_size = atoi(av[++i]);
      }
    }
    return file;
  }

int main(int argc, char **argv) 
{

    // int ierr;

    // ierr = MPI_Init(&argc, &argv);

    if ( !parseCommandLine(argc, argv))
    {
        /* no image file to  display */
        printf("No image file to display, use the arg -f to specify the filepath. \n\n");
        return -1;
    }

    GLuint texid;
    int    image;
    unsigned int fbo;

    /* GLUT init */
    glutInit(&argc, argv);            // Initialize GLUT
    glutInitDisplayMode(GLUT_DOUBLE); // Enable double buffered mode
    glutInitWindowSize(width, height);   // Set the window's initial width & height
    glutInitWindowPosition(0,0);
    glutCreateWindow(argv[0]);      // Create window with the name of the executable
    glutDisplayFunc(display);       // Register callback handler for window re-paint event
    glutReshapeFunc(reshape);       // Register callback handler for window re-size event
    
    /* OpenGL 2D generic init */
    init_GL(width, height);

    /* Initialization of DevIL */
     if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
     {
           printf("wrong DevIL version ");
           return -1;
     }
     ilInit(); 


    /* load the file picture with DevIL */
    image = load_image();
    if ( image == -1 )
    {
        printf("Can't load picture file %s by DevIL ", filename);
        return -1;
    }

    /* OpenGL texture binding of the image loaded by DevIL  */
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glGenTextures(1, &texid); /* Texture name generation */
    glBindTexture(GL_TEXTURE_2D, texid); /* Binding of texture name */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear interpolation for magnification filter */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear interpolation for minifying filter */
    glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData()); /* Texture specification */
       
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);  
    printf("File: %s. RGBA loaded. Width: %d ,Height: %d \n", filename, width, height);  
    //read_pixels();
    

    /* Main loop */
    glutMainLoop();
    
    /* Delete used resources and quit */
    ilDeleteImages(1, &image); /* Because we have already copied image data into texture data we can release memory used by image. */
    glDeleteTextures(1, &texid);
    glDeleteFramebuffers(1, &fbo); 

    //ierr = MPI_Finalize();

    return 0;
} 