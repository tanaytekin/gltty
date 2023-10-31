INCS = -Iglad/include
CFLAGS = -std=gnu99 -Wall -pedantic `pkg-config --cflags glfw3 freetype2` $(INCS)
LDFLAGS = -lm `pkg-config --libs glfw3 freetype2`

SRC = main.c ft.c gl.c
OBJ = $(SRC:.c=.o)

all : gltty

gltty : glad.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

glad.o : glad/src/glad.c
	$(CC) -c $< $(INCS)

%.o : %.c gltty.h
	$(CC) -c $< $(CFLAGS)

clean :
	rm -f gltty $(OBJ) glad.o

.PHONY = all clean
