INCS = -Iglad/include
CFLAGS = -std=gnu99 -Wall -pedantic `pkg-config --cflags glfw3` $(INCS)
LDFLAGS = -lm `pkg-config --libs glfw3`

SRC = main.c
OBJ = $(SRC:.c=.o)

all : gltty

gltty : glad.o $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

glad.o : glad/src/glad.c
	$(CC) -c $< $(INCS)

%.o : %.c
	$(CC) -c $< $(CFLAGS)

clean :
	rm -f gltty $(OBJ) glad.o

.PHONY = all clean
