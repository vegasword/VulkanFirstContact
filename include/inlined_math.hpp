#pragma once

#include <stdint.h>

static inline uint32_t Clamp(uint32_t input, uint32_t low, uint32_t high)
{
	if (input > high) return high;
	if (input < low)  return low;
	return input;
}