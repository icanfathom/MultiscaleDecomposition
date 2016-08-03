#include "Canvas.h"

extern SDL_PixelFormat* Eisel::PIXEL_FORMAT;

Canvas::Canvas(char* windowTitle, std::string path)
{
	SDL_Surface* surface = loadImage(path);
	width = surface->w;
	height = surface->h;
	res = width * height;

	Window* mainWindow = createWindow("Main", width, height);
	
	//Set PIXEL_FORMAT in Eisel.cpp for use in color conversion, etc...
	PIXEL_FORMAT = SDL_GetWindowSurface(mainWindow->sdlWindow)->format;
	
	//Convert surface to screen format and paint on Main window
	auto optimizedSurface = SDL_ConvertSurface(surface, PIXEL_FORMAT, NULL);
	sourceImageU32 = convertSurfaceToPixelArray(optimizedSurface);

	mainWindow->fillWithImage(convertSurfaceToPixelArray(optimizedSurface));
	mainWindow->updateTexture();
	mainWindow->render();
}

Canvas::~Canvas()
{
	for (int i = windows.size() - 1; i >= 0; --i)
	{
		if (windows[i] != nullptr)
			delete windows[i];
	}
}

Window* Canvas::createWindow(const char* title, Window* base)
{
	Window* newWindow = new Window(title, base->imgWidth, base->imgHeight, nextWindowPos, nextWindowPos);
	newWindow->fillWithImage(base->img);
	windows.push_back(newWindow);
	nextWindowPos += 50;
	return newWindow;
}

Window* Canvas::createWindow(const char* title, int width, int height)
{
	Window* window = new Window(title, width, height, nextWindowPos, nextWindowPos);
	windows.push_back(window);
	nextWindowPos += 50;
	return window;
}

void Canvas::renderAll()
{
	for (auto window : windows)
	{
		window->render();
	}
}

void Canvas::handleEvent(SDL_Event& e)
{
	for (auto window : windows)
	{
		window->handleEvent(e);
	}

	if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
	{
		for (auto iter = windows.begin(); iter != windows.end(); ++iter)
		{
			if ((*iter)->windowID == e.window.windowID)
			{
				delete *iter;
				windows.erase(iter);
				break;
			}
		}
	}
}

Window* Canvas::runMultiDecomp(Window* base, int k)
{
	std::vector<int>* minima = findMinima(base, k);
	VectorXf interpLowerValues = interpolateExtrema(base, k, minima);
	delete minima;
	std::vector<int>* maxima = findMaxima(base, k);
	VectorXf interpUpperValues = interpolateExtrema(base, k, maxima);
	delete maxima;

	VectorXf* avg = new VectorXf((interpLowerValues + interpUpperValues) / 2.0);

	//Create new window with fine detail
	std::ostringstream stream;
	stream << "Detail level " << k;
	Window* fineDetail = createWindow(stream.str().c_str(), base->imgWidth, base->imgHeight);
	fineDetail->fillWithImage(sourceImageU32);
	fillWithMultiDecompDetail(fineDetail, avg);
	fineDetail->updateTexture();

	//Fill base window with coarse detail
	base->fillWithImage(sourceImageU32);
	fillWithMultiDecompResidual(base, avg);
	base->updateTexture();

	return fineDetail;
}

void Canvas::reconstructFromDecomps(Window* base, Window* overlay)
{
	int tempRes = base->imgWidth * base->imgHeight;

	for (int i = 0; i < tempRes; i++)
	{
		double l = toLAB(overlay->img[i]).l;
		l = l * 2.0 - 100.0;

		ColorLAB lab = toLAB(base->img[i]);
		lab.l += l;
		base->img[i] = toSDL(lab);
	}
}

std::vector<int>* Canvas::findMinima(Window* base, int k)
{
	int sideLength = k / 2;
	std::vector<int>* minima = new std::vector<int>(base->imgRes, 0); //Initialize all entries to 0
	ColorRGB rgb;

	for (int x = 0; x < base->imgWidth; x++)
	{
		for (int y = 0; y < base->imgHeight; y++)
		{
			rgb = toRGB(base->img[base->XYtoIndex(x, y)]);
			float centerLum = ntscLuminance(rgb.r, rgb.g, rgb.b);
			int numSmaller = 0;

			for (int offsetX = -sideLength; offsetX <= sideLength; offsetX++)
			{
				for (int offsetY = -sideLength; offsetY <= sideLength; offsetY++)
				{
					if (x + offsetX >= 0
						&& x + offsetX < base->imgWidth
						&& y + offsetY >= 0
						&& y + offsetY < base->imgHeight)
					{
						rgb = toRGB(base->img[base->XYtoIndex(x + offsetX, y + offsetY)]);
						float luminance = ntscLuminance(rgb.r, rgb.g, rgb.b);

						if (luminance < centerLum)
						{
							numSmaller++;
						}
					}
				}
			}

			if (numSmaller <= k)
				minima->at(base->XYtoIndex(x, y)) = 1; //If center pixel is one of the k smallest luminances, flag it as a minimum
		}
	}

	return minima;
}

std::vector<int>* Canvas::findMaxima(Window* base, int k)
{
	int sideLength = k / 2;
	std::vector<int>* maxima = new std::vector<int>(base->imgRes, 0); //Initialize all entries to 0
	ColorRGB rgb;

	for (int x = 0; x < base->imgWidth; x++)
	{
		for (int y = 0; y < base->imgHeight; y++)
		{
			rgb = toRGB(base->img[base->XYtoIndex(x, y)]);
			float centerLum = ntscLuminance(rgb.r, rgb.g, rgb.b);
			int numLarger = 0;

			for (int offsetX = -sideLength; offsetX <= sideLength; offsetX++)
			{
				for (int offsetY = -sideLength; offsetY <= sideLength; offsetY++)
				{
					if (x + offsetX >= 0
						&& x + offsetX < base->imgWidth
						&& y + offsetY >= 0
						&& y + offsetY < base->imgHeight)
					{
						rgb = toRGB(base->img[base->XYtoIndex(x + offsetX, y + offsetY)]);
						float luminance = ntscLuminance(rgb.r, rgb.g, rgb.b);

						if (luminance > centerLum)
						{
							numLarger++;
						}
					}
				}
			}

			if (numLarger <= k)
				maxima->at(base->XYtoIndex(x, y)) = 1; //If center pixel is one of the k largest luminances, flag it as a maximum
		}
	}

	return maxima;
}

/*
Interpolates luminance values across the image,
holding local extrema constant and modifying neighboring values to smoothly transition between them.
@params
r			red channel
g			green channel
b			blue channel
width		width of the image in pixels
height		height of the image in pixels
k			the length of each edge of the neighborhood. The neighborhood ends up being k * k pixels centered on one central pixel.
extremaMap	a vector containing flags for each pixel. 1 means the corresponding pixel is an extrema, 0 means it's not.
*/
VectorXf Canvas::interpolateExtrema(Window* src, int k, std::vector<int>* extremaMap)
{
	//res is the total number of pixels in the imag
	float* luminance = new float[src->imgRes];
	for (int i = 0; i < src->imgRes; i++)
	{
		/*
		MatLab has an rgb2ntsc functions that converts
		rgb to Y, I, Q channels.
		We took their luminance calculation,
		which is from the Y channel.

		This function expects values in the range [0, 1].
		*/
		ColorRGB rgb = toRGB(src->img[i]);
		luminance[i] = ntscLuminance(rgb.r / 255.0f, rgb.g / 255.0f, rgb.b / 255.0f);
	}

	int sideLength = k / 2; //We'll loop from -sideLength to sideLength to handle all pixels surrounding the current center pixel

	SparseMatrix<float> A(src->imgRes, src->imgRes); //'A' has one coordinate pair per pixel.
	/*
	Now we reserve room for k*k non-zero entries per column.
	This step is crucial to making the matrix insertions reasonably fast.
	*/
	A.reserve(VectorXd::Constant(src->imgRes, k * k));

	std::vector<int> rows;
	rows.reserve(k * k);
	std::vector<int> cols;
	cols.reserve(k * k);
	std::vector<float> workingValues; //Will hold the luminance of neighboring pixels
	workingValues.reserve(k * k);

	/*
	Outer loop goes through y indices,
	Inner loop goes through x.
	This is because the sparse matrix needs to be set up in THIS ORDER EXACTLY
	Don't mess with me.
	:P
	*/
	for (int y = 0; y < src->imgHeight; y++)
	{
		for (int x = 0; x < src->imgWidth; x++)
		{
			if (!extremaMap->at(src->XYtoIndex(x, y)))	//Only interpolate value if it's not an extrema.
			{										//This means extrema will always keep their luminance values. In theory.
				for (int offsetX = -sideLength; offsetX <= sideLength; offsetX++)
				{
					for (int offsetY = -sideLength; offsetY <= sideLength; offsetY++)
					{
						if (x + offsetX >= 0
							&& x + offsetX < src->imgWidth
							&& y + offsetY >= 0
							&& y + offsetY < src->imgHeight)
						{
							if (offsetX != 0 || offsetY != 0)
							{
								rows.push_back(src->XYtoIndex(x, y)); //Index of center pixel
								cols.push_back(src->XYtoIndex(x + offsetX, y + offsetY)); //Index of current neighbor
								workingValues.push_back(luminance[src->XYtoIndex(x + offsetX, y + offsetY)]); //Luminance of current neighbor
							}
						}
					}
				}

				float centerLuminance = luminance[src->XYtoIndex(x, y)];

				/*
				'neighbors' excludes the center pixel itself.
				For most calculations that's what we want,
				but for avgDeviation we need the avg to be calculated including the center pixel.
				*/
				workingValues.push_back(centerLuminance);
				float avgLuminance = avgFloats(workingValues);

				std::vector<float> avgDeviation = std::vector<float>(workingValues.size());
				for (int i = 0; i < workingValues.size(); i++)
				{
					avgDeviation[i] = pow(workingValues[i] - avgLuminance, 2);
				}
				float csig = avgFloats(avgDeviation);

				workingValues.pop_back(); //Remove center pixel after calculating avg

				csig *= 0.6;
				std::vector<float> deviationFromCenter = std::vector<float>(workingValues.size());
				for (int i = 0; i < workingValues.size(); i++)
				{
					deviationFromCenter[i] = pow(centerLuminance - workingValues[i], 2);
				}
				float smallestDeviation = minFloats(deviationFromCenter);
				if (csig < -smallestDeviation / log(0.01f))
					csig = -smallestDeviation / log(0.01f);
				if (csig < 0.000002)
					csig = 0.000002f;

				for (int i = 0; i < workingValues.size(); i++)
				{
					workingValues[i] = exp(-pow(centerLuminance - workingValues[i], 2.0f) / csig);
				}
				float sum = sumFloats(workingValues);
				for (int i = 0; i < workingValues.size(); i++)
				{
					workingValues[i] /= sum;
				}

				//Now add all the neighbors to matrix A with the weights we calculated
				for (int i = 0; i < workingValues.size(); i++)
				{
					A.insert(rows[i], cols[i]) = -workingValues[i]; //Negate the values before storing them
				}
			}

			//Now add center pixel with weight=1
			A.insert(src->XYtoIndex(x, y), src->XYtoIndex(x, y)) = 1.0f;

			rows.clear();
			cols.clear();
			workingValues.clear();
		}
	}

	VectorXf b(src->imgRes);
	for (int i = 0; i < b.size(); i++) //We want the solver to keep extrema values the same. Only non-extrema are interpolated.
	{
		if (extremaMap->at(i) != 0)
		{
			b[i] = luminance[i];
		}
		else
		{
			b[i] = 0.0f;
		}
	}

	delete[] luminance; //The sparse matrix is a memory hog. To help not crash the program, I delete everything I can as soon as I can.

	/*
	The Eigen framework has a variety of sparse matrix solvers available.
	Only 2 of the several I tried gave the results we wanted:
	BiCGSTAB and SparseLU.
	I chose BiCGSTAB because it was 3 times faster.
	*/
	BiCGSTAB<SparseMatrix<float>> solver;
	solver.compute(A);
	VectorXf x = solver.solve(b);

	return x;
}

void Canvas::fillWithMultiDecompResidual(Window* window, VectorXf* multiDecompValues)
{
	for (int i = 0; i < window->imgRes; i++)
	{
		float avg = (*multiDecompValues)[i];
		avg *= 100.0f; //From [0.0, 1.0] to [0.0, 100.0]
		avg = std::min(100.0f, std::max(0.0f, avg)); //Clamp to [0.0, 100.0]

		ColorLAB lab = toLAB(window->img[i]);
		ColorLAB lab2 = ColorLAB(avg, lab.a, lab.b);
		ColorRGB rgb = toRGB(lab2);
		window->img[i] = toSDL(rgb);
	}
}

void Canvas::fillWithMultiDecompDetail(Window* window, VectorXf* multiDecompValues)
{
	for (int i = 0; i < window->imgRes; i++)
	{
		float avg = (*multiDecompValues)[i];
		avg *= 100.0f; //From [0.0, 1.0] to [0.0, 100.0]
		avg = std::min(100.0f, std::max(0.0f, avg)); //Clamp to [0.0, 100.0]

		ColorLAB lab = toLAB(window->img[i]);
		float l = lab.l;
		l = l - avg; //Now in [-100.0, 100.0]
		l = (l + 100.0) / 2.0; //Now in [0.0, 100.0]
		ColorRGB rgb = toRGB(ColorLAB(l, lab.a, lab.b));
		window->img[i] = toSDL(rgb);
	}
}

void Canvas::fillWithMaximaOnly(Window* window, int k)
{
	auto maxima = findMaxima(window, k);
	for (int i = 0; i < maxima->size(); i++)
	{
		if (!maxima->at(i))
		{
			window->img[i] = toSDL(ColorRGB(0, 0, 0));
		}
	}

	delete maxima;
}

void Canvas::fillWithMinimaOnly(Window* window, int k)
{
	auto minima = findMinima(window, k);
	for (int i = 0; i < minima->size(); i++)
	{
		if (!minima->at(i))
		{
			window->img[i] = toSDL(ColorRGB(0, 0, 0));
		}
	}

	delete minima;
}