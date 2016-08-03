#include "Window.h"

/*
Portions of this SDL code taken from LazyFoo's excellent tutorials:
http://lazyfoo.net/tutorials/SDL/index.php.
I owe you, LazyFoo.
*/

Window::Window(const char* windowTitle, int width, int height, int x, int y)
{
	windowWidth = std::max(width, 400);
	windowHeight = std::max(height, 400);

	//Create window
	sdlWindow = SDL_CreateWindow(windowTitle, x, y, windowWidth, windowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (sdlWindow == NULL)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return;
	}

	//Create renderer for window
	renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(sdlWindow);
		sdlWindow = NULL;
		return;
	}
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);

	windowID = SDL_GetWindowID(sdlWindow);
	shown = true;
	this->windowTitle = const_cast<char*>(windowTitle);
	mouseFocus = true;
	keyboardFocus = true;
	fullscreen = false;

	img = new Uint32[width * height];
	
	imgWidth = width;
	imgHeight = height;
	imgRes = width * height;
	if (width >= 400 && height >= 400)
	{
		renderTarget.x = 0;
		renderTarget.y = 0;
		renderTarget.w = width;
		renderTarget.h = height;
	}
	else
	{
		renderTarget.x = (400 - width) / 2;
		renderTarget.y = (400 - height) / 2;
		renderTarget.w = width;
		renderTarget.h = height;
	}
}

Window::~Window()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(sdlWindow);
}

void Window::fillWithImage(SDL_Surface* surface)
{
	SDL_LockSurface(surface);
	for (int i = 0; i < imgRes; i++)
	{
		img[i] = *((Uint32*)surface->pixels + i);
	}
	SDL_UnlockSurface(surface);
}

void Window::fillWithImage(Uint32* image)
{
	for (int i = 0; i < imgRes; i++)
	{
		img[i] = image[i];
	}
}

void Window::render()
{
	if (texture != nullptr)
	{
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, &renderTarget);
		SDL_RenderPresent(renderer);
	}
}

void Window::updateTexture()
{
	SDL_UpdateTexture(texture, NULL, img, imgWidth * sizeof(Uint32));
}

void Window::resizeWindow(int width, int height)
{
	width = std::max(width, 400);
	height = std::max(height, 400);
	windowWidth = width;
	windowHeight = height;
	SDL_SetWindowSize(sdlWindow, width, height);
	SDL_RenderPresent(renderer);
}

void Window::resizeCanvas(int width, int height)
{
	//Update canvas dimensions
	imgWidth = width;
	imgHeight = height;
	imgRes = width * height;
	renderTarget.w = width;
	renderTarget.h = height;

	if (width < 400)
	{
		renderTarget.x = (400 - width) / 2;
	}
	else
	{
		renderTarget.x = 0;
	}

	if (height < 400)
	{
		renderTarget.y = (400 - height) / 2;
	}
	else
	{
		renderTarget.y = 0;
	}

	//Create new canvas of this size
	Uint32* newCanvas = new Uint32[imgRes];
	delete[] img;
	img = newCanvas;

	//Create new texture
	SDL_DestroyTexture(texture);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
}

void Window::resizeWindowAndImg(int width, int height)
{
	resizeWindow(width, height);
	resizeCanvas(width, height);
}

void Window::handleEvent(SDL_Event& e)
{
	//If an event was detected for this window
	if (e.type == SDL_WINDOWEVENT && e.window.windowID == windowID)
	{
		//Caption update flag
		bool updateCaption = false;
		switch (e.window.event)
		{
			//Window appeared
		case SDL_WINDOWEVENT_SHOWN:
			shown = true;
			break;

			//Window disappeared
		case SDL_WINDOWEVENT_HIDDEN:
			shown = false;
			break;

			//Get new dimensions and repaint
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			resizeWindow(e.window.data1, e.window.data2);
			break;

			//Repaint on expose
		case SDL_WINDOWEVENT_EXPOSED:
			SDL_RenderPresent(renderer);
			break;

			//Mouse enter
		case SDL_WINDOWEVENT_ENTER:
			mouseFocus = true;
			updateCaption = true;
			break;

			//Mouse exit
		case SDL_WINDOWEVENT_LEAVE:
			mouseFocus = false;
			updateCaption = true;
			break;

			//Keyboard focus gained
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			keyboardFocus = true;
			updateCaption = true;
			break;

			//Keyboard focus lost
		case SDL_WINDOWEVENT_FOCUS_LOST:
			keyboardFocus = false;
			updateCaption = true;
			break;

			//Window minimized
		case SDL_WINDOWEVENT_MINIMIZED:
			minimized = true;
			break;

			//Window maxized
		case SDL_WINDOWEVENT_MAXIMIZED:
			minimized = false;
			break;

			//Window restored
		case SDL_WINDOWEVENT_RESTORED:
			minimized = false;
			break;
		}
	}
	else if (e.type == SDL_MOUSEBUTTONDOWN && mouseFocus)
	{
		int x, y;
		SDL_GetMouseState(&x, &y);
		x -= renderTarget.x;
		y -= renderTarget.y;
		if (x < imgWidth && y < imgHeight)
		{
			ColorRGB rgb;
			SDL_GetRGB(img[XYtoIndex(x, y)], SDL_GetWindowSurface(sdlWindow)->format, &rgb.r, &rgb.g, &rgb.b);
			ColorLAB lab = toLAB(rgb);
			printf("r: %d g: %d b: %d \t l: %lf a: %lf b: %lf\n", rgb.r, rgb.g, rgb.b, lab.l, lab.a, lab.b);
		}
	}
}

void Window::focus()
{
	//Restore window if needed
	if (!shown)
	{
		SDL_ShowWindow(sdlWindow);
	}

	//Move window forward
	SDL_RaiseWindow(sdlWindow);
}

int Window::XYtoIndex(int x, int y)
{
	//Take an (x, y) coordinate pair and return the corresponding index in a 1-dimensional array
	return y * imgWidth + x;
}
