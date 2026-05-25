#pragma once
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/UI/Widget.hpp"
#include <string>

class InputSystem;
class Clock;

struct WindowConfig
{
	InputSystem*	m_inputSystem = nullptr;
	std::string		m_windowTitle = "untitled App";

	float			m_aspectRatio = 2.f;
	bool			m_isFullscreen = false;

	float			maxClientFractionOfDesktop = 0.9f;

	IntVec2			m_size = IntVec2(-1, -1);
	IntVec2			m_pos = IntVec2(-1, -1);
};

// bool operator<(IntVec2 const& a, IntVec2 const& b);


class Window
{
friend class Camera;
friend class Widget;

public:
	Window(WindowConfig const& config);
	~Window();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void ShutDown();

	WindowConfig const& GetConfig() const;
	float				GetAspect() const;
	float				GetCurrentAspectRatio() const;
	IntVec2				GetWindowDimensions() const;				// this is logic dimensions
	IntVec2				GetWindowPhysicalPixelsDimensions() const;	// this is scaled because we are using 150% scale
	void*				GetDeviceContext() const;
	void*				GetHwnd() const;

	// all the classes could call this function to get a pointer to the window
	// if there is no window, it could return nullptr for checking if a window exists
	static Window* GetMainWindowPtr();
	Vec2 GetNormalizedCursorPos() const;

	static Window* s_theWindow;

	void ResetWindowsStatus(WindowConfig config); // use config to update the windows settings

	void SetFullScreen();

protected:
	void CreateOSWindow();
	void RunMessagePump();
	WindowConfig	m_config;

	// void* means a generic pointer to something
	// todo:??? why void* instead of HWND, means could static cast to every type of pointer?
	void* m_hwnd = nullptr; // HWND in windows (handle to the current window)

	// HDC in Windows ( handle to the display device context)
	// We don't use HDC here because otherwise we have to include window.h in this hpp file
	// Otherwise who includes this hpp will have to include the long windows.h
	// We could cast this pointer to other classes pointer later on
	void* m_displayContext = nullptr;
};