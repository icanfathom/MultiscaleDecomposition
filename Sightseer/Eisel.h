#pragma once

#include <stdio.h>
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <vector>

/*
I'm going to apologize right now for the name Eisel.
It's a group of helper functions, so 'Helper' would have been the obvious name to use.
However, that just seemed so boring.

I always try to come up with more creative names...which leads to obtuse code
that requires a block comment (like this one) to even explain.
I am a bad man.

I knew I needed a Helper class and I knew it was going to be utilized by Canvas.cpp.

So I chose the name Eisel.
Because an Eisel always supports a Canvas.
*/

namespace Eisel
{
	extern SDL_PixelFormat* PIXEL_FORMAT;

	const double refWhiteX = 95.047;	//reference white
	const double refWhiteY = 100.0;		//reference white
	const double refWhiteZ = 108.883;	//reference white
	const double EPSILON = 0.008856;	//actual CIE standard
	const double KAPPA = 903.3;			//actual CIE standard
	const double GAMMA = 2.2;			//approximates sRGB

	struct ColorRGB
	{
		Uint8 r;
		Uint8 g;
		Uint8 b;

		ColorRGB() { r = 0; g = 0; b = 0; }
		ColorRGB(int r, int g, int b) { this->r = clamp(r); this->g = clamp(g); this->b = clamp(b); }
		ColorRGB operator+(ColorRGB &other) const { return ColorRGB(r + other.r, g + other.g, b + other.b); }
		ColorRGB operator+=(ColorRGB &other) { r += other.r; g += other.g; b += other.b; return *this; }
		ColorRGB operator*(double m) const { return ColorRGB(r * m, g * m, b * m); }
		ColorRGB operator/(double q) const { return ColorRGB(r / q, g / q, b / q); }
		bool operator==(const ColorRGB &other) const { return r == other.r && g == other.g && b == other.b; }
		Uint8 clamp(int val) const { return std::max(0, std::min(255, val)); }
		void clampAll() { r = clamp((int)r); g = clamp((int)g); b = clamp((int)b); }
	};

	struct ColorXYZ
	{
		double x;
		double y;
		double z;

		ColorXYZ() { x = 0.0; y = 0.0; z = 0.0; }
		ColorXYZ(double x, double y, double z) { this->x = x; this->y = y; this->z = z; clampX(); clampY(); clampZ(); }
		ColorXYZ operator+(ColorXYZ &other) const { return ColorXYZ(x + other.x, y + other.y, z + other.z); }
		ColorXYZ operator+=(ColorXYZ &other) { x += other.x; y += other.y; z += other.z; return *this; }
		void clampX() { x = std::max(0.0, std::min(refWhiteX, x)); }
		void clampY() { y = std::max(0.0, std::min(refWhiteY, y)); }
		void clampZ() { z = std::max(0.0, std::min(refWhiteZ, z)); }
	};

	struct ColorLAB
	{
		double l;
		double a;
		double b;

		ColorLAB() { l = 0; a = 0; b = 0; }
		ColorLAB(double l, double a, double b) { this->l = l; this->a = a; this->b = b; clampAll(); }
		ColorLAB operator+(ColorLAB &other) const { return ColorLAB(l + other.l, a + other.a, b + other.b); }
		ColorLAB operator+=(ColorLAB &other) { l += other.l; a += other.a; b += other.b; return *this; }
		ColorLAB operator*(double m) const { return ColorLAB(l * m, a * m, b * m); }
		ColorLAB operator*=(double m) { l *= m; a *= m; b *= m; return *this; }
		double distance(ColorLAB &other) const { return sqrt(pow(l - other.l, 2.0) + pow(a - other.a, 2.0) + pow(b - other.b, 2.0)); }
		void clampL() { l = std::max(0.0, std::min(100.0, l)); }
		void clampAB() { a = std::max(-128.0, std::min(128.0, a)); b = std::max(-128.0, std::min(128.0, b)); }
		void clampAll() { clampL(); clampAB(); }
	};

	float increaseContrast100(float luminance);
	float increaseContrast1(float luminance);
	float ntscLuminance(ColorRGB colorRgb);
	float ntscLuminance(float r, float g, float b);
	double clamp(double val, double min, double max);
	float clamp(float val, float min, float max);
	float avgFloats(std::vector<float>&);
	float sumFloats(std::vector<float>&);
	float minFloats(std::vector<float>&);

	ColorRGB toRGB(ColorLAB lab);
	ColorRGB toRGB(ColorXYZ xyz);
	ColorRGB toRGB(Uint32 color);
	ColorLAB toLAB(ColorRGB rgb);
	ColorLAB toLAB(ColorXYZ xyz);
	ColorLAB toLAB(Uint32 color);
	ColorXYZ toXYZ(ColorRGB rgb);
	ColorXYZ toXYZ(ColorLAB lab);
	Uint32 toSDL(ColorRGB rgb); //Not an actual color space. This is SDL's internal representation.
	Uint32 toSDL(ColorLAB lab); //Not an actual color space. This is SDL's internal representation. 
	
	SDL_Surface* loadImage(std::string path);
	Uint32* convertSurfaceToPixelArray(SDL_Surface* image);
}