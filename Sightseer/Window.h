#pragma once

#include <sstream>
#include <algorithm>
#include <SDL.h>
#include "Eisel.h"

using namespace Eisel;

class Window
{
public:
	Window(const char* windowTitle, int width, int height, int x = 0, int y = 0);
	~Window();

	void fillWithImage(SDL_Surface* image);
	void fillWithImage(Uint32* image);
	void updateTexture();
	void render();

	void resizeWindow(int width, int height);
	void resizeCanvas(int width, int height);
	void resizeWindowAndImg(int width, int height);

	virtual void handleEvent(SDL_Event& e);
	void focus();
	
	int XYtoIndex(int x, int y);
	
	SDL_Window* sdlWindow;
	SDL_Renderer* renderer;
	SDL_Rect renderTarget;
	SDL_Texture* texture;

	int windowID;
	char* windowTitle;
	int windowWidth;
	int windowHeight;

	Uint32* img; //The actual pixels being displayed, stored as 4 8-bit unsigned ints
	int imgWidth;
	int imgHeight;
	int imgRes; //Resolution = total number of pixels

	bool mouseFocus;
	bool keyboardFocus;
	bool fullscreen;
	bool minimized;
	bool shown;
};
