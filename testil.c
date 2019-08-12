#include <IL/il.h>
#include <GL/glut.h>
#include <stdbool.h>
#include <mpi.h>
#include <math.h>



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
int my_id, root_process, ierr, num_procs, expected_tiles, tile_elements_index, vertical_frontier, horizontal_frontier;
MPI_Status status;

typedef struct Tile{
  int x,y;
  unsigned char* pix_buf;
}Tile;

struct Tile* tiles_array;

void receive_tiles(){
  for(int x=0; x<expected_tiles; x++){
    Tile tile_obj;
    unsigned char buff[tile_size*tile_size*4];
    ierr = MPI_Recv( &tile_obj, sizeof(tile_obj), MPI_BYTE, root_process, 0, MPI_COMM_WORLD, &status);
    ierr = MPI_Recv( buff, (tile_size*tile_size*4), MPI_BYTE, root_process, 1, MPI_COMM_WORLD, &status);
    tile_obj.pix_buf = &buff;
    tiles_array[x] = tile_obj;
  }
}

/* send_tiles is a function that creates all the tiles to be passed to the other processes, this function is used only by the root process.
It uses the get_process function to know to which nodes send the tile. */

void send_tiles(){
  int pixels_offset = 0;
  int *tags = malloc(sizeof(int) * col*rows);
  for (int i=0; i<col*rows; i++)
  {
    tags[i] = 0;
  }

  for(int y=0; y<(height/tile_size); y++){
    for(int x=0; x<(width/tile_size); x++){
      Tile tile_obj;
      tile_obj.x = (x*tile_size);
      tile_obj.y = (y*tile_size);
      tile_obj.pix_buf = (unsigned char*)malloc(tile_size*tile_size*4);
      int pix_line = 0;

      for(int j=(y*tile_size); j<((y+1)*tile_size); j++){
        memcpy(&tile_obj.pix_buf[pix_line*tile_size*4], &pixels[pixels_offset], sizeof(unsigned char)*4*tile_size);
        pix_line++;
        pixels_offset = pixels_offset + (4*tile_size);  
      }

      int process_to_send; 
      int min = -1;
      int it = 0;
      while(it <= num_procs-1){
        switch (it)
        {
        case 0:
          process_to_send = get_process(tile_obj.x+1, tile_obj.y+1);
          break;
        case 1:
          process_to_send = get_process(tile_obj.x+tile_size, tile_obj.y+1);
          break;
        case 2:
          process_to_send = get_process(tile_obj.x+1, tile_obj.y+tile_size);
          break;
        case 3:
          process_to_send = get_process(tile_obj.x+tile_size, tile_obj.y+tile_size);
          break;
        }
        if(min<process_to_send){
          min = process_to_send;
          if(process_to_send == root_process){
            tiles_array[tile_elements_index] = tile_obj;
            tile_elements_index++;
          }else{
            ierr = MPI_Send( &tile_obj, sizeof(tile_obj), MPI_BYTE, process_to_send, 0, MPI_COMM_WORLD);
            ierr = MPI_Send( tile_obj.pix_buf, (tile_size*tile_size*4), MPI_BYTE, process_to_send, 1, MPI_COMM_WORLD);

          }
          tags[process_to_send]++;
        }
        it++;
      }
      //free(tile_obj.pix_buf);
      //free(pixels);
    }  
  }

  for (int i=0; i<col*rows; i++)
  {
    printf("Total of Tiles sent to process %d : %d \n\n", i , tags[i]);
  }
}

/* get_process is an auxiliary function that helps send_tiles to locate in which process the tile that contains
x and y should be assigned to */

int get_process(int x, int y){
  int process = 0;
  int row_height = vertical_frontier;
  int col_width = horizontal_frontier;
  for(int i = 0; i<rows; i++){
    if(y>row_height){
      row_height += vertical_frontier;
      process += col;
    }else{
      i = rows;
    }
  }
  for(int i = 0; i<col; i++){ 
    if(x>col_width){
      col_width += horizontal_frontier;
      process ++;
    }else{
      i = col;
    }
  }
  return process;
}

/* read_pixels works for getting all the rgba values from the framebuffer that is bind to the window.
 It stores the values in a global GLubyte array called pixels and then prints the values in console*/
void read_pixels(){
  pixels = (unsigned char*)malloc(height*width*4);
  glReadBuffer(GL_FRONT);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
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

  int L = sqrt(num_procs);
  int BW = width/L; 
  int BH = height/L;
  pixels = malloc(width * height *4);
  int X0 = BW * (my_id%L);
  int Y0 = BH * (my_id/L);
  for(int i=0; i<expected_tiles; i++){
    int xt = tiles_array[i].x - X0;
    int yt = tiles_array[i].y - Y0;
    for(int j=0; j<tile_size; j++){
      int offset = ((yt + j) * BW + xt)*4;
      memcpy(&pixels[offset],&tiles_array[i].pix_buf[j*tile_size*4], sizeof(unsigned char)*tile_size*4);
    }      
  }


  glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels );
    
  glutSwapBuffers();  
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

// void add_tile(Tile tile){
//   tiles_array[tile_elements_index++] = tile;
// }



int main(int argc, char **argv) 
{
    ierr = MPI_Init(&argc, &argv);

    if ( !parseCommandLine(argc, argv))
    {
        /* no image file to  display */
        printf("No image file to display, use the arg -f to specify the filepath. \n\n");
        return -1;
    }
     
    /* find out MY process ID, and how many processes were started. */
    
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    vertical_frontier = ceil(height/rows);
    horizontal_frontier = ceil(width/col);
    double expected_width_tiles, expected_height_tiles;
    if (vertical_frontier%tile_size!=0){
      expected_height_tiles = ceil(vertical_frontier/tile_size);
    }else{
      expected_height_tiles = vertical_frontier/tile_size;
    }
    
    if(horizontal_frontier%tile_size!=0){
      expected_width_tiles = ceil(horizontal_frontier/tile_size);
    }else{
      expected_width_tiles = horizontal_frontier/tile_size;
    }
    
    expected_tiles = expected_height_tiles*expected_width_tiles;
    //printf("Expecting %d tiles\n", expected_tiles);

    tiles_array = malloc(expected_tiles*sizeof(struct Tile));
    tile_elements_index = 0;

    /* GLUT init */
      glutInit(&argc, argv);            // Initialize GLUT
      glutInitDisplayMode(GLUT_RGBA); // Enable double buffered mode
      glutInitWindowSize(width, height);   // Set the window's initial width & height
      glutInitWindowPosition(0,0);
      glutCreateWindow(argv[0]);      // Create window with the name of the executable
      glutDisplayFunc(display);       // Register callback handler for window re-paint event
      glutReshapeFunc(reshape);       // Register callback handler for window re-size event
      
      /* OpenGL 2D generic init */
      init_GL(width, height);

    if( my_id == 0 ) {
      GLuint texid;
      int    image;
      unsigned int fbo;

      

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
      printf("File: %s. RGBA loaded. Width: %d ,Height: %d \n\n", filename, width, height);  
      read_pixels();
      

      /* Main loop */
      //
      
      /* Delete used resources and quit */
      ilDeleteImages(1, &image); /* Because we have already copied image data into texture data we can release memory used by image. */
      glDeleteTextures(1, &texid);
      glDeleteFramebuffers(1, &fbo); 
    }
    else{
      receive_tiles();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    // if(my_id ==2){
    //   for(int x=0; x<expected_tiles; x++){
    //     printf("Printing Tile #%d \n", x);
    //     int pix_line = 0;
    //     for(int l = 0; l <tile_size*4; l++){
    //       printf("PixBuff[%d]: %d \n\n", l ,tiles_array[x].pix_buf[l+(pix_line*tile_size*4)]);
    //     }
    //     printf("====================================================== \n");
    //   }
    // }  
    
    glutMainLoop();

    free(tiles_array);
    free(pixels);
    ierr = MPI_Finalize();

    return 0;
} 