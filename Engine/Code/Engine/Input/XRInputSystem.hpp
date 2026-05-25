#pragma once
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include <array>

namespace Side
{
	const int LEFT = 0;
	const int RIGHT = 1;
	const int COUNT = 2;
}  // namespace Side

struct InputState
{
	XrActionSet actionSet{ XR_NULL_HANDLE };
	XrAction grabAction{ XR_NULL_HANDLE };
	XrAction poseAction{ XR_NULL_HANDLE };
	XrAction vibrateAction{ XR_NULL_HANDLE };
	XrAction quitAction{ XR_NULL_HANDLE };
	XrAction recenterAction{ XR_NULL_HANDLE };

	XrAction right_joystick_x_Action{ XR_NULL_HANDLE };
	XrAction right_joystick_y_Action{ XR_NULL_HANDLE };

	std::array<XrPath, Side::COUNT> handSubactionPath;
	std::array<XrSpace, Side::COUNT> handSpace;
	// std::array<float, Side::COUNT> handScale = { {1.0f, 1.0f} };
	std::array<XrBool32, Side::COUNT> handActive;
};