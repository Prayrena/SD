#include "Engine/Renderer/Window.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/Camera.hpp"

// #define WIN32_LEAN_AND_MEAN  // we set it up in the engine property setting	
#include <windows.h>

#include <shellscalingapi.h> // For SetProcessDpiAwareness
#pragma comment(lib, "Shcore.lib") // Link with Shcore.lib

Window* Window :: s_theWindow = nullptr;// Actual definition of the global advertised in .hppFile

extern InputSystem* g_theInput;


//bool operator<(IntVec2 const& a, IntVec2 const& b)
//{
//	if (a.x < b.x && a.y < b.y)
//	{
//		return true;
//	}
//	else return false;
//}

//-----------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).
//
// #SD1ToDo: We will move this function to a more appropriate place (Engine/Renderer/Window.cpp) later on...
//windows will notify you message through this function(say a key is pressed down) 
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	// Get the window (assume we only have one for now)
	Window* window = Window::GetMainWindowPtr();
	GUARANTEE_OR_DIE(window != nullptr, "window was null");

	// Ask the window for a pointer to the InputSystem it was created with (in its Input SystemConfig)
	InputSystem* input = window->GetConfig().m_inputSystem;
	GUARANTEE_OR_DIE(input != nullptr, "Window's InputSystem pointer was null");

	switch (wmMessageCode)
	{
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
		// it triggers when player closes this program (task manager will kill the program rootly)
		case WM_CLOSE:
		{
			FireEvent("quit");
			return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		}

		// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
		// user could get multiple key down with only one keyup
		// case WM_KEYDOWN:
		// {
		// 	// use this line to check which key 
		// 	// char unicode characters, use''
		// 	// string use " "
		// 	unsigned char asKey = (unsigned char)wParam;
		// 	//std :: string testing = "q";
		// 	if ( input )
		// 	{
		// 		input->HandleKeyPressed(asKey);
		// 		return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		// 	}
		// 	break;
		// }

		case WM_CHAR: // will only contains the character information but not the arrow input
		{
			EventArgs args;
			args.SetValue("TextInput", Stringf("%d", (unsigned char)wParam));
			FireEvent("CharInput", args);
			return 0;
		}

		case WM_KEYDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyPressed", args);
			return 0;
		}

		case WM_KEYUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", Stringf("%d", (unsigned char)wParam));
			FireEvent("KeyReleased", args);
			return 0;
		}


		// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
		//case WM_KEYUP:
		//{
		//	unsigned char asKey = (unsigned char)wParam;
		//	if (input)
		//	{
		//		input->HandleKeyReleased(asKey);
		//		return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		//	}
		//	// #SD1ToDo: Tell the App (or InputSystem later) about this key-released event...
		//	break;
		//}

		// case WM_KEYUP:
		// {
		// 	EventArgs args;
		// 	args.SetValue("KeyCodeRelease", Stringf("%d", (unsigned char)wParam));
		// 	FireEvent("KeyCodeRelease, args");
		// 	return 0;
		// }

		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_LBUTTONDOWN:
		{
			unsigned char keyCode = KEYCODE_LEFT_MOUSE;
			if (input)
			{
				input->HandleKeyPressed(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_LBUTTONUP:
		{
			unsigned char keyCode = KEYCODE_LEFT_MOUSE;
			if (input)
			{
				input->HandleKeyReleased(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		case WM_RBUTTONDOWN:
		{
			unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
			if (input)
			{
				input->HandleKeyPressed(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
		// Mouse wheel scroll event — fire with delta for DevConsole scrolling
		case WM_MOUSEWHEEL:
		{
			int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			EventArgs args;
			args.SetValue("WheelDelta", Stringf("%d", wheelDelta));
			FireEvent("MouseWheel", args);
			return 0;
		}

		// treat this special mouse button windows message as if it were an ordinary key down for us
		case WM_RBUTTONUP:
		{
			unsigned char keyCode = KEYCODE_RIGHT_MOUSE;
			if (input)
			{
				input->HandleKeyReleased(keyCode);
				return 0;// "consumes" this message(tell windows "Okay, we handled it")
			}
			break;
		}
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}

Window::Window(WindowConfig const& config)
	: m_config( config )
{
	s_theWindow = this;
}

Window::~Window()
{

}

void Window::Startup()
{
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	CreateOSWindow();
}

void Window::CreateOSWindow()
{
	HMODULE applicationInstanceHandle = GetModuleHandle(NULL);

	// Define a window class
	WNDCLASSEX windowClassDescription;

	// define windows style
	DWORD windowStyleFlags;
	DWORD windowStyleExFlags;

	// Get desktop rect, dimensions, aspect
	RECT screenRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &screenRect);
	float desktopWidth = (float)(screenRect.right - screenRect.left);
	float desktopHeight = (float)(screenRect.bottom - screenRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// the windows size we want to show
	RECT clientRect = { 0, 0, 800, 600 };

	if (m_config.m_isFullscreen) // full screen mode
	{
		windowStyleFlags = WS_POPUP;
		windowStyleExFlags = WS_EX_APPWINDOW | WS_EX_TOPMOST;

		memset(&windowClassDescription, 0, sizeof(windowClassDescription));
		windowClassDescription.cbSize = sizeof(windowClassDescription);
		windowClassDescription.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // force the window to redraw the entire window if it's resized
		windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
		windowClassDescription.hInstance = applicationInstanceHandle;
		windowClassDescription.hIcon = NULL; // LoadIcon(nullptr, IDI_APPLICATION); // Use the default application icon
		windowClassDescription.hCursor = NULL;
		windowClassDescription.lpszClassName = TEXT("Simple Window Class");
		windowClassDescription.lpszMenuName = NULL; // No menu for full-screen
		windowClassDescription.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // fill the background with a specific color
		RegisterClassEx(&windowClassDescription);

		// Full screen mode, use desktop dimensions
		clientRect.left = 0;
		clientRect.top = 0;
		clientRect.right = (LONG)desktopWidth;
		clientRect.bottom = (LONG)desktopHeight;
	}
	else // windowed mode
	{
		windowStyleFlags = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
		windowStyleExFlags = WS_EX_APPWINDOW;

		memset(&windowClassDescription, 0, sizeof(windowClassDescription));
		windowClassDescription.cbSize = sizeof(windowClassDescription);
		windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
		windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
		windowClassDescription.hInstance = applicationInstanceHandle;
		windowClassDescription.hIcon = NULL;
		windowClassDescription.hCursor = NULL;
		windowClassDescription.lpszClassName = TEXT("Simple Window Class");
		RegisterClassEx(&windowClassDescription);

		if (m_config.m_size.x < 0 && m_config.m_size.y < 0) // when config is not defining the windows size
		{
			// default style, we take a fraction of the screen to show our application
			// Calculate maximum client size (as some % of desktop size)
			float clientWidth = desktopWidth * m_config.maxClientFractionOfDesktop;
			float clientHeight = desktopHeight * m_config.maxClientFractionOfDesktop;
			if (m_config.m_aspectRatio > desktopAspect)
			{
				// Client window has a wider aspect than desktop; shrink client height to match its width
				clientHeight = clientWidth / m_config.m_aspectRatio;
			}
			else
			{
				// Client window has a taller aspect than desktop; shrink client width to match its height
				clientWidth = clientHeight * m_config.m_aspectRatio;
			}

			// Calculate client rect bounds by centering the client area
			float clientMarginX = 0.5f * (desktopWidth - clientWidth);
			float clientMarginY = 0.5f * (desktopHeight - clientHeight);

			clientRect.left = (int)clientMarginX;
			clientRect.right = clientRect.left + (int)clientWidth;
			clientRect.top = (int)clientMarginY;
			clientRect.bottom = clientRect.top + (int)clientHeight;
		}
		else // we have the windows size
		{
			if (m_config.m_pos.x < 0 && m_config.m_pos.y < 0) // no defined windows pos
			{
				// we center the application
				// if the defined size is larger than the desktop
				if (m_config.m_size.x >= desktopWidth || m_config.m_size.y >= desktopHeight)
				{
					// we will keep the height is all showed, consider most application on window width > height
					float multiplier = (0.9f * desktopHeight) / m_config.m_size.y;
					float clientMarginX = desktopWidth - (multiplier * m_config.m_size.x);
					float clientMarginY = 0.1f * desktopHeight;

					clientRect.left = (long)clientMarginX;
					clientRect.right = clientRect.left + long(multiplier * m_config.m_size.x);
					clientRect.top = (long)clientMarginY;
					clientRect.bottom = clientRect.top + long(0.9f * desktopHeight);
				}
				else
				{
					// Calculate client rect bounds by centering the client area
					float clientMarginX = 0.5f * (desktopWidth - m_config.m_size.x);
					float clientMarginY = 0.5f * (desktopHeight - m_config.m_size.y);

					clientRect.left = (int)clientMarginX;
					clientRect.right = clientRect.left + (int)m_config.m_size.x;
					clientRect.top = (int)clientMarginY;
					clientRect.bottom = clientRect.top + (int)m_config.m_size.y;
				}
			}
			else // defined windows pos
			{
				// pos
				clientRect.left = m_config.m_pos.x;
				clientRect.top = m_config.m_pos.y;

				clientRect.right = m_config.m_size.x + clientRect.left;  // Set right boundary
				clientRect.bottom = m_config.m_size.y + clientRect.top;   // Set bottom boundary
			}
		}

	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	::AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	::MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));

	m_hwnd = static_cast<HWND>( CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		applicationInstanceHandle, 
		NULL) );

	HWND hwnd = static_cast<HWND>(m_hwnd);
	m_displayContext = ::GetDC(hwnd);

	::ShowWindow(hwnd, SW_SHOW);
	::SetForegroundWindow(hwnd);
	::SetFocus(hwnd);

	HCURSOR cursor = ::LoadCursor(NULL, IDC_ARROW);
	::SetCursor(cursor);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// update the window aspect ratio after the window is created
	RECT finalRect;
	if (GetClientRect(static_cast<HWND>(m_hwnd), &finalRect)) 
	{
		int width = finalRect.right - finalRect.left;
		int height = finalRect.bottom - finalRect.top;
		m_config.m_aspectRatio = (float)width / (float)height;
	}
	else 
	{
		ERROR_AND_DIE("Unable to get the finalized window size");
	}
}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{
	// Sleep( 1 );

	// "present" the backBuffer by swapping the front (visible) and back (working) screen buffers
	// SwapBuffers(reinterpret_cast<HDC>(m_displayContext));

}

void Window::ShutDown()
{
	return;
}

WindowConfig const& Window::GetConfig() const
{
	return m_config;
}

float Window::GetAspect() const
{
	return m_config.m_aspectRatio;
}

float Window::GetCurrentAspectRatio() const
{
	RECT clientRect;
	// Get the client area (the drawable area inside the window)
	if (GetClientRect(static_cast<HWND>(m_hwnd), &clientRect))
	{
		// Calculate width and height
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		// Ensure height is non-zero to avoid division by zero
		if (height != 0)
		{
			return static_cast<float>(width) / static_cast<float>(height);
		}
		else
		{
			ERROR_AND_DIE("windows height is 0! Check windows class");
		}
	}
	else
	{
		// Return a default value (aspect ratio of 1) if the client area cannot be retrieved
		ERROR_AND_DIE("Unable to get windows client width and height");
	}
}

IntVec2 Window::GetWindowDimensions() const
{
	RECT clientRec;
	GetClientRect((HWND)m_hwnd, &clientRec);
	IntVec2 clientDimensions;
	clientDimensions.x = (int)(clientRec.right - clientRec.left);
	clientDimensions.y = (int)(clientRec.bottom - clientRec.top);

	return clientDimensions;
}

IntVec2 Window::GetWindowPhysicalPixelsDimensions() const
{
	// Get the client area in logical pixels
	RECT clientRect;
	GetClientRect((HWND)m_hwnd, &clientRect);

	// Get the DPI for the window
	UINT dpi = GetDpiForWindow((HWND)m_hwnd); // Requires Windows 10 or later
	float scalingFactor = dpi / 96.0f; // 96 DPI is the default (100% scaling)

	// Convert logical pixels to physical pixels
	RECT physicalRect;
	physicalRect.left = static_cast<LONG>(clientRect.left * scalingFactor);
	physicalRect.top = static_cast<LONG>(clientRect.top * scalingFactor);
	physicalRect.right = static_cast<LONG>(clientRect.right * scalingFactor);
	physicalRect.bottom = static_cast<LONG>(clientRect.bottom * scalingFactor);

	IntVec2 clientDimensions;
	clientDimensions.x = (int)(physicalRect.right - physicalRect.left);
	clientDimensions.y = (int)(physicalRect.bottom - physicalRect.top);

	return clientDimensions;
}

void* Window::GetDeviceContext() const
{
	return m_displayContext;
}

Window* Window::GetMainWindowPtr()
{
	return s_theWindow;
}


void* Window::GetHwnd() const
{
	return m_hwnd;
}

Vec2 Window::GetNormalizedCursorPos() const
{
	HWND windowHandle = HWND(m_hwnd);
	POINT cursorCoords;
	RECT clientRect;
	::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left
	::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
	::GetClientRect(windowHandle, &clientRect); // size of window interior(0,0 to width, height)
	float cursorX = float(cursorCoords.x) / float(clientRect.right); // normalized x position
	float cursorY = float(cursorCoords.y) / float(clientRect.bottom); // normalized y position
	return Vec2(cursorX, 1.f - cursorY);// we want (0,0) in the bottom-left
}

void Window::ResetWindowsStatus(WindowConfig config)
{
	m_config = config;

	std::wstring windowsTitle = ConvertStringToWstring(m_config.m_windowTitle);
	SetWindowText(static_cast<HWND>(m_hwnd), windowsTitle.c_str());

	if (config.m_isFullscreen)
	{
		SetFullScreen();
	}
	else
	{	
		// if (operator<( config.m_pos, IntVec2() )) // defined windows position
		// {
		// }
		// else if (operator<(config.m_size, IntVec2())) // defined windows size
		// {
		// 
		// }
		// // set windows position and size
		// SetWindowPos(static_cast<HWND>(m_hwnd), nullptr, config.m_pos.x, config.m_pos.y, config.m_size.x, config.m_size.y, SWP_NOZORDER | SWP_NOACTIVATE);
		// // is there any function to set windows size or position alone?
		// SetWindowsSize(m_hwnd, 1024, 768);  // Set width to 1024 and height to 768
		// MoveWindow(hwnd, x, y, width, height, TRUE);
	}
}

void Window::SetFullScreen()
{
	// Get screen resolution
	RECT screenRect;
	GetWindowRect(GetDesktopWindow(), &screenRect);

	// removing stuff: borderless window
	LONG style = GetWindowLong(static_cast<HWND>(m_hwnd), GWL_STYLE);
	style &= ~(WS_CAPTION | WS_THICKFRAME); // Remove title bar and borders
	style &= ~(WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU); // Remove minimize/maximize buttons, system menu
	SetWindowLong(static_cast<HWND>(m_hwnd), GWL_STYLE, style);

	// Remove the extended windows styles (if it was set before)
	LONG exStyle = GetWindowLong(static_cast<HWND>(m_hwnd), GWL_EXSTYLE);
	exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE); // Removes borders
	SetWindowLong(static_cast<HWND>(m_hwnd), GWL_EXSTYLE, exStyle);

	// Set window size to match the screen resolution
	SetWindowPos(static_cast<HWND>(m_hwnd), HWND_TOP,
		0, 0, // Position at the top-left corner
		screenRect.right, screenRect.bottom, // Full screen dimensions
		SWP_NOZORDER | SWP_FRAMECHANGED); // Apply new frame style and resize

	// Make the window visible (if not already)
	ShowWindow(static_cast<HWND>(m_hwnd), SW_SHOWMAXIMIZED);
}

void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;;)
	{
		const BOOL wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}
