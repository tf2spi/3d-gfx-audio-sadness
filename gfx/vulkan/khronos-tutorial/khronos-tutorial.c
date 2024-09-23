#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#define WIDTH 640
#define HEIGHT 480

int main(int argc, char **argv)
{
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL not initialized! %s\n", SDL_GetError());
		return 1;
	}

	SDL_Window *window = SDL_CreateWindow(
		"Khronos Tutorial",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH,
		HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
	if (window == NULL)
	{
		printf("Window could not be created! %s", SDL_GetError());
		return 1;
	}

	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			}
		}
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
