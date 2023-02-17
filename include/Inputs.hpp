#pragma once

typedef struct Inputs
{
	bool forward, backward;
	bool left, right;
	bool up, down;
	bool rotatingCamera;
	float sensitivity = 0.2f;
	double mouseX, mouseY;
} Inputs;