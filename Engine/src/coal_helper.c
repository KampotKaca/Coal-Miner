#include "coal_helper.h"

void cm_spiral_loop(unsigned int width, unsigned int height,
					void (*func)(unsigned int x, unsigned int y))
{
	int centerX = (int)width / 2;
	int centerY = (int)height / 2;

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