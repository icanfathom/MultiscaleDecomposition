#include <SDL.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include "Canvas.h"

using namespace std;

const int NEIGHBORHOOD_SIZE = 5;

Canvas* canvas;
Window* activeWindow;
bool quit = false;
bool ctrlKeyDown = false;
bool altKeyDown = false;

void init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL had an initialization error. SDL_Error: %s\n", SDL_GetError());
	}
}

void cleanup()
{
	delete canvas;
	SDL_Quit();
}

Window* getNextSelectedWindow()
{
	SDL_Event e;

	while (true)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			canvas->handleEvent(e);

			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					return nullptr;
				}
			}
			else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				for (auto window : canvas->windows)
				{
					if (e.window.windowID == window->windowID)
					{
						return window;
					}
				}
			}
		}

		SDL_Delay(100);
	}
}

void setMode(Canvas* canvas, const char mode)
{
	cout << endl << "Entering mode " << (ctrlKeyDown ? "ctrl + " : "") << (altKeyDown ? "alt + " : "") << mode << endl;
	
	activeWindow = nullptr;
	for (auto window : canvas->windows)
	{
		if (window->keyboardFocus)
		{
			activeWindow = window;
			break;
		}
	}

	switch (mode)
	{
	case 'c': //Clear (source image)
		activeWindow->fillWithImage(canvas->sourceImageU32);
		activeWindow->updateTexture();
		break;
	case 'd': //Multiscale decomposition
		canvas->runMultiDecomp(activeWindow, NEIGHBORHOOD_SIZE);
		break;
	case 'm': //Maxima only
		canvas->fillWithMaximaOnly(activeWindow, NEIGHBORHOOD_SIZE);
		activeWindow->updateTexture();
		break;
	case 'n': //Minima only
		canvas->fillWithMinimaOnly(activeWindow, NEIGHBORHOOD_SIZE);
		activeWindow->updateTexture();
		break;
	case 'r': //Reconstruct
	{
		printf("Select the window to overlay\n");
		Window* overlayWindow = getNextSelectedWindow();
		if (overlayWindow != nullptr)
		{
			canvas->reconstructFromDecomps(activeWindow, overlayWindow);
			activeWindow->updateTexture();
		}
	}
		break;
	}

	canvas->renderAll();
	printf("Done\n");
}

int main(int argc, char* args[])
{
	try
	{
		init();
	}
	catch(std::exception& e)
	{
		printf(e.what());
		cleanup();
		return 1;
	}

	//Create main window and fill with an image
	canvas = new Canvas("Main", "Images/Sunset1.png");
	setMode(canvas, 'c');
	printf("\nPress a letter and hit enter:\n");
	printf("c: reset to source image\n");
	printf("d: run decomposition\n");
	printf("m: show maxima only\n");
	printf("n: show minima only\n");
	printf("r: recompose detail layer onto current layer\n");

	SDL_Event e;
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}

			canvas->handleEvent(e); //Let the window handle resizing and repositioning

			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_LCTRL)
				{
					ctrlKeyDown = true;
				}
				else if (e.key.keysym.sym == SDLK_LALT)
				{
					altKeyDown = true;
				}
				else
				{
					setMode(canvas, e.key.keysym.sym);
				}
			}
			else if (e.type == SDL_KEYUP)
			{
				if (e.key.keysym.sym == SDLK_LCTRL)
				{
					ctrlKeyDown = false;
				}
				else if (e.key.keysym.sym == SDLK_LALT)
				{
					altKeyDown = false;
				}
			}
		}

		SDL_Delay(100);
	}

	cleanup();
	return 0;
}
