#include "coal_helper.h"
#include <stdio.h>

void cm_spiral_loop(unsigned int width, unsigned int height,
					void (*func)(unsigned int x, unsigned int y))
{
	int centerX = (int)((width - 1) / 2);
	int centerY = (int)((height - 1) / 2);

	// Start from the center
	int x = centerX;
	int y = centerY;

	const static int dirX[4] = { 1, 0, -1, 0 };
	const static int dirY[4] = { 0, 1, 0, -1 };

	int dir = 0; // Start direction is right
	int steps = 1; // Number of steps in the current layer
	int stepCount = 0; // Step counter

	func(x, y); // Call function for the center

	while (steps < width || steps < height)
	{
		// Move in the current direction
		for (int i = 0; i < steps; i++)
		{
			x += dirX[dir];
			y += dirY[dir];
			
			// Ensure the coordinates are within bounds
			if (x >= 0 && x < width && y >= 0 && y < height) func(x, y);
		}

		// Change direction
		dir = (dir + 1) % 4;
		stepCount++;

		// Increase steps after two direction changes
		if (stepCount % 2 == 0) steps++;
	}
}

uint32_t cm_trailing_zeros(uint64_t n)
{
	return n ? __builtin_ctzll(n) : 64u;
}

uint32_t cm_trailing_ones(uint64_t n)
{
	return n ? __builtin_ctzll(~n) : 64u;
}

uint32_t cm_pow2(uint32_t n)
{
	uint32_t num = 1;
	for (uint32_t i = 0; i < n; ++i) num *= 2;
	return num;
}