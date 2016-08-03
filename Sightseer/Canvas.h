#pragma once

#include <iostream>
#include <vector>
#include <SDL.h>
#include <Eigen/Core>
#include <Eigen/Sparse>
#include "Window.h"

using namespace Eigen;
using namespace Eisel;

class Canvas
{
public:
	Canvas(char* windowTitle, std::string path);
	~Canvas();
	Window* createWindow(const char* title, Window* window);
	Window* createWindow(const char* title, int width, int height);
	void renderAll();
	void handleEvent(SDL_Event& e);

	Window* runMultiDecomp(Window* base, int k);
	void reconstructFromDecomps(Window* base, Window* overlay);
	std::vector<int>* findMaxima(Window* base, int k);
	std::vector<int>* findMinima(Window* base, int k);
	VectorXf interpolateExtrema(Window* base, int k, std::vector<int>* extremaMap);

	void fillWithMultiDecompResidual(Window*, VectorXf* multiDecompValues);
	void fillWithMultiDecompDetail(Window*, VectorXf* multiDecompValues);
	void fillWithMaximaOnly(Window* window, int k);
	void fillWithMinimaOnly(Window* window, int k);
	
	int width;
	int height;
	int res; //Resolution = total number of pixels

	std::vector<Window*> windows;
	int nextWindowPos = 50;

	Uint32* sourceImageU32;	//This is the format SDL uses internally.
							//Eisel.cpp has helper functions toRGB and toLAB that can convert to RGB and LAB color.
};
