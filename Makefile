INC = -Iglad/include
CFLAGS = -std=c99 -Wall -Wextra -pedantic `pkg-config --cflags glfw3 freetype2` $(INC)
LDFLAGS = `pkg-config --libs glfw3 freetype2`

all: gltty

gltty: main.o glad.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) -c -o $@ $< $(CFLAGS)

glad.o: glad/src/glad.c
	$(CC) -c -o $@ $< $(INC)

clean:
	rm -rf main.o glad.o gltty

.PHONY: all clean
