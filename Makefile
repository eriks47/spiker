CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = `pkg-config --libs --cflags sdl2` -lSDL2_ttf -lSDL2_image

main: main.c
	$(CC) $(CFLAGS) $(LIBS) $< -o _SDL_TEST

run: main
	./_SDL_TEST

clean:
	rm _SDL_TEST
