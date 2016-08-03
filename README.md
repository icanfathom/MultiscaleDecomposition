# Multiscale Decomposition via Levin, et. al interpolation

This is a c++ implementation of multiscale image decomposition as proposed by Subr, Soler, and Durand in *Edge-preserving Multiscale Image Decomposition based on Local Extrema*. The PDF and MatLab code can be found on [Kartic Subr's site](http://home.eps.hw.ac.uk/~ks400/research.html#MSDecompExtrema). I recommend you read the paper for a more in-depth explanation of what the interpolation function is doing. The technique they use is originally from [*Colorization Using Optimization*](http://www.cs.huji.ac.il/~yweiss/Colorization/) by Levin, Lischinski, and Weiss.

We looked for source code of the interpolation algorithm but could only find MatLab code. We decided to port it to c++. You will find our source code in Sightseer/Canvas.cpp. Below is a walkthrough; I hope you will find it helpful in understanding the process or porting it to additional languages.

This was a 2016 project by Joseph Merboth under the advisement of Dr. Nathan Gossett.

##Dependencies
If you want to run my c++ code you will need two extra frameworks: SDL and Eigen.

###SDL and SDL_Image
SDL is not required for any of the logic to work. It's merely a code package for image loading and window creation across different operating systems.

The relevant .h and .sdl files are included in my repository, inside the 'include' folder. If those don't work, use the links below to download them yourself. Here are the requirements:
- All of my header files need to include `SDL.h` and `SDL_image.h`.
- SDL requires you to link to `SDL2.lib`, `SDL2main.lib`, and `SDL2_image.lib`.

SDL is available at [libsdl.org](https://libsdl.org/). SDL_Image is available at [libsdl.org/projects/SDL_image/](https://libsdl.org/projects/SDL_image/).

###Eigen
Eigen is important to the project because it's used to solve the sparse matrix in the interpolation step, which is effectively a system of linear equations. Feel free to substitute another linear algebra package instead of Eigen.

Like SDL, Eigen is included in my repo under 'include/Eigen'. If that doesn't work for you, it should be a simple matter to get your own Eigen download because the whole framework is just a collection of header files.
 - The only dependency is in Canvas.h where I include `Eigen/Core` and `Eigen/Sparse`. (`Eigen/Core` contains the core Eigen libraries; `Eigen/Sparse` is everything relating to sparse matrices).

Eigen is available at [eigen.tuxfamily.org](http://eigen.tuxfamily.org/index.php?title=Main_Page).


##Code Walkthrough
Interpolation happens in 5 steps:

1. Find minima
2. Find maxima
3. Interpolate values between the minima
4. Interpolate values between the maxima
5. Take the average of interpolated values

###Finding minima and maxima
This is a straightforward process of looping through each pixel and examining the `k * k` pixels surrounding it. Look at their luminance values; if the center pixel is among the k smallest values, flag it as a minima. If it's among the k largest values, flag it as a maxima. Thus, you're looking for which pixels are *local* minima and maxima with respect to each `k * k` neighborhood. See methods [`findMinima`](https://github.com/icanfathom/sightseer/blob/MultiscaleDecomposition/Sightseer/Canvas.cpp#L123) and [`findMaxima`](https://github.com/icanfathom/sightseer/blob/MultiscaleDecomposition/Sightseer/Canvas.cpp#L165) in `Canvas.cpp` for my implementation.

Once you have a list of minima and maxima (which I called `minimaMap` and `maximaMap`), you run interpolation on both of them **separately**. After running on minima you will have a version of the image where every pixel that was a local minima retains its luminance value and every other pixel smoothly blends between them, and likewise for maxima.

###Interpolating
Interpolation happens in a function called `interpolateExtrema`.

```c++
VectorXf interpolateExtrema(Window* src, int k, std::vector<int>* extremaMap)
{
```

- `interpolateExtrema`: takes in a `Window`, which is simply a class that holds and displays an image. `src` gives us access to the width, height, and RGB values of the image.
- `k`: the neighborhood size over which we're interpolating (and should be the same as the `k` when finding minima and maxima).
- `extremaMap` is a vector with one int per pixel. A value of 1 means that pixel is a minima or maxima and 0 means it's not. Remember, we run interpolation on the image once for minima and once for maxima, so a 1 means the current pixel is either a minima or maxima, whichever we're working with right now.
- The return type `VectorXf` is a wrapper from Eigen that contains a vector of floats. This will be the interpolated pixel values.

```c++
float* luminance = new float[src->imgRes];
for (int i = 0; i < src->imgRes; i++)
{
	ColorRGB rgb = toRGB(src->img[i]);
	luminance[i] = ntscLuminance(rgb.r / 255.0f, rgb.g / 255.0f, rgb.b / 255.0f);
}
```

- `luminance`: an array of floats of size `imgRes` (width * height of the image).
- The `for` loop iterates through ever pixel, converts it to an RGB value, and then calculates luminance using [`ntscLuminance`](https://github.com/icanfathom/sightseer/blob/MultiscaleDecomposition/Sightseer/Eisel.cpp#L33). (This is a function copied from MatLab that divides an image into 3 channels, Y, I, and Q, with Y being the luminance channel).
- `luminance` now holds the luminance value of every pixel.

```c++
int sideLength = k / 2;
```

- `sideLength`: the radius of the neighborhood surrounding each pixel.

```c++
SparseMatrix<float> A(src->imgRes, src->imgRes);
A.reserve(VectorXd::Constant(src->imgRes, k * k));

std::vector<int> rows;
rows.reserve(k * k);
std::vector<int> cols;
cols.reserve(k * k);
std::vector<float> workingValues;
workingValues.reserve(k * k);
```

- `A`: 2-dimensional sparse matrix with dimensions `res * res` - one column and one row for each pixel. We reserve space in each column for `k * k` entries: one entry for each neighborhood surrounding that pixel.
- `rows`: index of the center pixel.
- `cols`: index of a neighbor to the center.
  - We'll be processing the image one pixel at a time. Each iteration we will have one 'center' pixel and a number of neighboring pixels. As we go along we're filling the matrix with weights to represent how the neighbor pixels affect the center (to get a smooth interpolation effect). As we insert into the matrix, the row is the index of the 'center' pixel and the column is the index of the neighbor pixel we're looking at right now. Together they give us a coordinate pair, which is a location in our 2D matrix. The value at that location is the weight that relates how those pixels transact.
- `workingValues`: the values of each neighorhood as we process it.

####Accumulating luminance values

```c++
for (int y = 0; y < src->imgHeight; y++)
{
	for (int x = 0; x < src->imgWidth; x++)
	{
```

- Loop through every pixel in the image.

```c++
if (!extremaMap->at(src->XYtoIndex(x, y)))
{
	for (int offsetX = -sideLength; offsetX <= sideLength; offsetX++)
	{
		for (int offsetY = -sideLength; offsetY <= sideLength; offsetY++)
		{
```

- Only interpolate if the center pixel is *not* an extrema.
- Go `sideLength` distance on either side of the center pixel to cover the neighborhood.

```c++
if (x + offsetX >= 0
	&& x + offsetX < src->imgWidth
	&& y + offsetY >= 0
	&& y + offsetY < src->imgHeight)
{
	if (offsetX != 0 || offsetY != 0)
	{
		rows.push_back(src->XYtoIndex(x, y));
		cols.push_back(src->XYtoIndex(x + offsetX, y + offsetY));
		workingValues.push_back(luminance[src->XYtoIndex(x + offsetX, y + offsetY)]);
	}
}
```

- If the current pixel we're looking at is within the image,</br>
**and** if it's not the center pixel:
  - Add index of the center pixel to `rows`.</br>
  - Add index of the current pixel to `cols`.</br>
- Add luminance of the current pixel to `workingValues`.

Now all neighbor's luminance values have been added to `workingValues`.

####Modifying values

```c++
float centerLuminance = luminance[src->XYtoIndex(x, y)];
workingValues.push_back(centerLuminance);
```

- Temporarily add the center pixel to `workingValues`. We need it to calculate avgLuminance.

```c++
float avgLuminance = avgFloats(workingValues);

std::vector<float> avgDeviation = std::vector<float>(workingValues.size());
for (int i = 0; i < workingValues.size(); i++)
{
	avgDeviation[i] = pow(workingValues[i] - avgLuminance, 2);
}
float csig = avgFloats(avgDeviation);
```

- Take the average of all luminance values
- Loop through all neighbors and record their deviation from the average
- Set `csig` as the average of all the deviations

```c++
workingValues.pop_back();
```

- Remove center pixel from `workingValues`. The proceeding calculations don't need it.

```c++
csig *= 0.6;
std::vector<float> deviationFromCenter = std::vector<float>(workingValues.size());
for (int i = 0; i < workingValues.size(); i++)
{
	deviationFromCenter[i] = pow(centerLuminance - workingValues[i], 2);
}
```

- Multiply `csig` by 0.6.
- Now take deviation from center. This is each pixel's deviation from the **center** pixel, not from the average of all pixels.

```c++
float smallestDeviation = minFloats(deviationFromCenter);
if (csig < -smallestDeviation / log(0.01f))
	csig = -smallestDeviation / log(0.01f);
if (csig < 0.000002)
	csig = 0.000002f;
```

- Find the smallest deviation from the center pixel.
- If `csig` is very small, set it to a suitable minimum to avoid dividing by zero later.

```c++
for (int i = 0; i < workingValues.size(); i++)
{
	workingValues[i] = exp(-pow(centerLuminance - workingValues[i], 2.0f) / csig);
}
```

- Loop through all neighbors:
  - Calculate this expression: ![Equation](https://github.com/icanfathom/sightseer/blob/MultiscaleDecomposition/Sightseer/Images/csig2.PNG "Equation")</br>
  where `l` is luminance of center pixel,</br>
  `xi` is luminance of current pixel,</br>
  `csig` is the value `csig`.
  - Since we're taking the square of the difference between luminance values, this ends up being a sort of least-squares problem we're solving. `csig` becomes a weighting component in the least-squares formulation. See the [Levin, et. al paper](http://www.cs.huji.ac.il/~yweiss/Colorization/) for a more full mathematical breakdown.

```c++
float sum = sumFloats(workingValues);
for (int i = 0; i < workingValues.size(); i++)
{
	workingValues[i] /= sum;
}
```

- Take the sum of all `workingValues`.
- Divide all `workingValues` by the sum.

```c++
for (int i = 0; i < workingValues.size(); i++)
{
	A.insert(rows[i], cols[i]) = -workingValues[i];
}
```

- Loop through all neighbors (this excludes the center pixel right now because it's not in `workingValues`)
  - Insert into the sparse matrix
    - The value from `rows` becomes the row index.
    - The value from `cols` becomes the column index.
    - The value from `workingValues` **is negated* and then becomes the value for that row and column of the matrix.

```c++
}

A.insert(src->XYtoIndex(x, y), src->XYtoIndex(x, y)) = 1.0f;
```

- Exit the `if` statement where we checked if the center pixel is not an extrema.
- Thus, this step runs regardless of whether or not the center pixel is an extrema:
  - Insert a `1` into the matrix, using the center pixel's coordinate as both the row and column index. This mean the center pixel gives itself a weight of 1 when we solve the system of equations.

```c++
	}
}
```

- Exit both `for` loops. We've gone through every pixel and added weights to the sparse matrix.

####Solving the matrix

- We've now filled the matrix `A` with weights representing how each pixel relates to its neighboring pixels.
- Basically, `A` sets up the constratints, and we'll set `b` to be the pixel values that must be enforced - we want maxima or minima to maintain their values while all other pixels change to accommodate them. Then, when we solve for `x`, we'll have a list of pixel values that obey the constraints of `A`.

```c++
VectorXf b(src->imgRes);
for (int i = 0; i < b.size(); i++)
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
```

- Create a vector of floats `b`. Initialize its capacity to the number of pixels in the image.
- Loop through every pixel in the image
  - If it **is** an extrema:
    - Set its `b` value to the luminance of the pixel.
  - If **not** an extrema:
    - Set its `b` value to 0.

```c++
BiCGSTAB<SparseMatrix<float>> solver;
solver.compute(A);
VectorXf x = solver.solve(b);

return x;
```

- Initialize a sparse matrix solver (we chose the BiCGSTAB solver from the Eigen library because it was fast and accurate).
- Give the solver our matrix `A` and our solution vector `b`.
- Solve for the vector `x` that satisfies the system of equations.
- Return `x`.
