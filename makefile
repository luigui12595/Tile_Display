OBJS	= testil.o
SOURCE	= testil.c
HEADER	= 
OUT	= tileDisplay
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = -L/usr/X11R6/lib -lGL -lGLU -lglut -lIL

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

testil.o: testil.c
	$(CC) $(FLAGS) testil.c 


clean:
	rm -f $(OBJS) $(OUT)
