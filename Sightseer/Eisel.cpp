#include "Eisel.h"

namespace Eisel
{
	extern SDL_PixelFormat* PIXEL_FORMAT = nullptr;

	float increaseContrast100(float luminance)
	{
		/*
		-100 = -100,
		0 = 0,
		100 = 100,
		values in between are exaggerated
		so positives increase and negatives decrease,
		always staying within the sleeve of [-100.0, 100.0]
		*/
		return luminance * (2.0f - abs(luminance / 100.0f));
	}

	float increaseContrast1(float luminance)
	{
		/*
		-1 = -1,
		0 = 0,
		1 = 1,
		values in between are exaggerated
		so positives increase and negatives decrease,
		always staying within the sleeve of [-1.0, 1.0]
		*/
		return luminance * (2.0f - abs(luminance));
	}

	float ntscLuminance(ColorRGB rgb)
	{
		/*
		MatLab has an rgb2ntsc functions that converts
		rgb to Y, I, Q channels.
		We took their luminance calculation,
		which is from the Y channel.
		*/
		return (float)(0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
	}

	float ntscLuminance(float r, float g, float b)
	{
		/*
		MatLab has an rgb2ntsc functions that converts
		rgb to Y, I, Q channels.
		We took their luminance calculation,
		which is from the Y channel.

		This function expects values in the range [0, 1].
		*/
		return 0.299f * r + 0.587f * g + 0.114f * b;
	}

	float clamp(float value, float minimum, float maximum)
	{
		return std::min(maximum, std::max(minimum, value));
	}

	float avgFloats(std::vector<float>& floats)
	{
		float sum = 0.0;
		for (auto num : floats)
		{
			sum += num;
		}
		return sum / floats.size();
	}

	float sumFloats(std::vector<float>& floats)
	{
		float sum = 0.0;
		for (auto num : floats)
		{
			sum += num;
		}
		return sum;
	}

	float minFloats(std::vector<float>& floats)
	{
		float smallest = floats[0];
		for (auto num : floats)
		{
			if (num < smallest)
				smallest = num;
		}
		return smallest;
	}

	double clamp(double value, double minimum, double maximum)
	{
		return std::min(maximum, std::max(minimum, value));
	}

	ColorRGB toRGB(ColorLAB lab)
	{
		return toRGB(toXYZ(lab));
	}

	ColorRGB toRGB(ColorXYZ xyz)
	{
		double var_X = xyz.x / 100.0; //X from 0 to  95.047
		double var_Y = xyz.y / 100.0; //Y from 0 to 100.000
		double var_Z = xyz.z / 100.0; //Z from 0 to 108.883

		double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
		double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
		double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;

		if (var_R > 0.0031308)	var_R = 1.055 * pow(var_R, 1.0 / 2.4) - 0.055;
		else					var_R = 12.92 * var_R;
		if (var_G > 0.0031308)	var_G = 1.055 * pow(var_G, 1.0 / 2.4) - 0.055;
		else					var_G = 12.92 * var_G;
		if (var_B > 0.0031308)	var_B = 1.055 * pow(var_B, 1.0 / 2.4) - 0.055;
		else					var_B = 12.92 * var_B;

		double R = var_R * 255.0;
		double G = var_G * 255.0;
		double B = var_B * 255.0;
		
		/*
		Be ye warned: R, G, and B (but especially B) have a chance of returning negative values.
		This is not a valid state for an RGB color.
		Make sure you're handling this potentiality,
		ESPECIALLY if you're storing your RGB values as unsigned ints.
		I was getting crazy blue splotches in the middle of my yellows because B was going negative
		and ColorRGB wasn't correctly clamping to 0.
		I don't even know why this would cause that to happen.

		Notice, now, that my ColorRGB constructor clamps the RGB values as ints before storing them as uints.
		Very important.
		*/
		return ColorRGB(R, G, B);
	}
	
	ColorRGB toRGB(Uint32 color)
	{
		ColorRGB rgb;
		SDL_GetRGB(color, PIXEL_FORMAT, &rgb.r, &rgb.g, &rgb.b);
		return rgb;
	}
	
	ColorLAB toLAB(ColorRGB rgb)
	{
		return toLAB(toXYZ(rgb));
	}

	ColorLAB toLAB(ColorXYZ xyz)
	{
		double var_X = xyz.x / refWhiteX;
		double var_Y = xyz.y / refWhiteY;
		double var_Z = xyz.z / refWhiteZ;

		if (var_X > 0.008856)	var_X = pow(var_X, 1.0 / 3.0);
		else					var_X = (7.787 * var_X) + (16.0 / 116.0);
		if (var_Y > 0.008856)	var_Y = pow(var_Y, 1.0 / 3.0);
		else					var_Y = (7.787 * var_Y) + (16.0 / 116.0);
		if (var_Z > 0.008856)	var_Z = pow(var_Z, 1.0 / 3.0);
		else					var_Z = (7.787 * var_Z) + (16.0 / 116.0);

		double l = (116.0 * var_Y) - 16.0;
		double a = 500.0 * (var_X - var_Y);
		double b = 200.0 * (var_Y - var_Z);

		return ColorLAB(l, a, b);
	}

	ColorLAB toLAB(Uint32 color)
	{
		return toLAB(toRGB(color));
	}

	ColorXYZ toXYZ(ColorRGB rgb)
	{
		double var_R = rgb.r / 255.0;
		double var_G = rgb.g / 255.0;
		double var_B = rgb.b / 255.0;

		if (var_R > 0.04045) var_R = pow((var_R + 0.055) / 1.055, 2.4);
		else                 var_R = var_R / 12.92;
		if (var_G > 0.04045) var_G = pow((var_G + 0.055) / 1.055, 2.4);
		else                 var_G = var_G / 12.92;
		if (var_B > 0.04045) var_B = pow((var_B + 0.055) / 1.055, 2.4);
		else                 var_B = var_B / 12.92;

		var_R *= 100.0;
		var_G *= 100.0;
		var_B *= 100.0;

		//Observer. = 2°, Illuminant = D65
		double X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
		double Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
		double Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

		return ColorXYZ(X, Y, Z);
	}

	ColorXYZ toXYZ(ColorLAB lab)
	{
		double var_Y = (lab.l + 16.0) / 116.0;
		double var_X = lab.a / 500.0 + var_Y;
		double var_Z = var_Y - lab.b / 200.0;

		if (pow(var_X, 3.0) > 0.008856)		var_X = pow(var_X, 3.0);
		else								var_X = (var_X - 16.0 / 116.0) / 7.787;
		if (lab.l > KAPPA * EPSILON)		var_Y = pow(var_Y, 3.0);
		else								var_Y = lab.l / KAPPA;
		if (pow(var_Z, 3.0) > 0.008856)		var_Z = pow(var_Z, 3.0);
		else								var_Z = (var_Z - 16.0 / 116.0) / 7.787;

		double X = var_X * refWhiteX;
		double Y = var_Y * refWhiteY;
		double Z = var_Z * refWhiteZ;

		return ColorXYZ(X, Y, Z);
	}

	Uint32 toSDL(ColorRGB colorRgb)
	{
		return SDL_MapRGB(PIXEL_FORMAT, colorRgb.r, colorRgb.g, colorRgb.b);
	}

	Uint32 toSDL(ColorLAB colorLab)
	{
		return toSDL(toRGB(colorLab));
	}

	SDL_Surface* loadImage(std::string path)
	{
		//The final optimized image
		SDL_Surface* optimizedSurface = NULL;

		//Load image at specified path
		SDL_Surface* loadedSurface = IMG_Load(path.c_str());
		if (loadedSurface == NULL)
		{
			printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
		}

		return loadedSurface;
	}

	Uint32* convertSurfaceToPixelArray(SDL_Surface* surface)
	{
		int res = surface->w * surface->h; //Width * height
		Uint32* pixels = new Uint32[res];

		SDL_LockSurface(surface);
		for (int i = 0; i < res; i++)
		{
			pixels[i] = *((Uint32*)surface->pixels + i);
		}
		SDL_UnlockSurface(surface);

		return pixels;
	}
}
