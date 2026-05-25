#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/Renderer/Window.hpp"
#include <Windows.h> // just to get the value of each virtual key
//#include "Engine/Input/XboxController.hpp"

extern InputSystem* g_theInput;

const unsigned char KEYCODE_F1 = VK_F1; // 112 == 0x70
const unsigned char KEYCODE_F2 = VK_F2; // 113 == 0x71
const unsigned char KEYCODE_F3 = VK_F3; // 114 == 0x72
const unsigned char KEYCODE_F4 = VK_F4; // 115 == 0x73
const unsigned char KEYCODE_F5 = VK_F5; // 116 == 0x74
const unsigned char KEYCODE_F6 = VK_F6; // 117 == 0x75
const unsigned char KEYCODE_F7 = VK_F7; // 118 == 0x76
const unsigned char KEYCODE_F8 = VK_F8; // 119 == 0x77
const unsigned char KEYCODE_F9 = VK_F9; // 120 == 0x78
const unsigned char KEYCODE_F10 = VK_F10; // 121 == 0x79
const unsigned char KEYCODE_F11 = VK_F11; // 122 == 0x7A
const unsigned char KEYCODE_F12 = VK_F12; // 123 == 0x7B
const unsigned char KEYCODE_ESC = VK_ESCAPE; // 27 == 0x1B

const unsigned char KEYCODE_NUM1 = 0x31; // 1 == 0x31
const unsigned char KEYCODE_NUM2 = 0x32; // 1 == 0x31
const unsigned char KEYCODE_NUM3 = 0x33; // 1 == 0x31
const unsigned char KEYCODE_NUM4 = 0x34; // 1 == 0x31
const unsigned char KEYCODE_NUM5 = 0x35; // 1 == 0x31
const unsigned char KEYCODE_NUM6 = 0x36; // 1 == 0x31
const unsigned char KEYCODE_NUM7 = 0x37; // 1 == 0x31
const unsigned char KEYCODE_NUM8 = 0x38; // 1 == 0x31
const unsigned char KEYCODE_NUM9 = 0x39; // 1 == 0x31

const unsigned char KEYCODE_UPARROW = VK_UP; // 38 == 0x26
const unsigned char KEYCODE_DOWNARROW = VK_DOWN; // 40 == 0x28
const unsigned char KEYCODE_LEFTARROW = VK_LEFT; // 37 == 0x25
const unsigned char KEYCODE_RIGHTARROW = VK_RIGHT; // 39 == 0x27

extern const unsigned char KEYCODE_HOME = VK_HOME;
extern const unsigned char KEYCODE_END = VK_END;

extern const unsigned char KEYCODE_INSERT = VK_INSERT;
extern const unsigned char KEYCODE_DELETE = VK_DELETE;
extern const unsigned char KEYCODE_ENTER = VK_RETURN;
extern const unsigned char KEYCODE_BACKSPACE = VK_BACK;

const unsigned char KEYCODE_LEFT_MOUSE = VK_LBUTTON;
const unsigned char KEYCODE_RIGHT_MOUSE = VK_RBUTTON;
const unsigned char KEYCODE_LEFTBRACKET = 0xDB; // VK_OEM_4, this is the [ / { key
const unsigned char KEYCODE_RIGHTBRACKET = 0xDD; // VK_OEM_6, this is the ] / } key

const unsigned char KEYCODE_LESSTHAN = 0xBC; 
const unsigned char KEYCODE_GREATERTHAN = 0xBE;

const unsigned char KEYCODE_TILDE = 0xC0; // VK_OEM_3, this is the `(grave)/ ~(tilde) key
const unsigned char KEYCODE_SHIFT = VK_SHIFT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
InputSystem::InputSystem( InputConfig const& config )
	: m_config(config)
{
	for (int controllerIndex = 0; controllerIndex < NUM_XBOX_CONTROLLERS; ++controllerIndex)
	{
		m_controllers[controllerIndex].m_id = controllerIndex;
		m_controllers[controllerIndex].m_LStick.SetDeadZoneThresholds(.3f, .95f);
		m_controllers[controllerIndex].m_LStick.SetDeadZoneThresholds(.3f, .95f);
	}
}

InputSystem::~InputSystem()
{

}

void InputSystem::Startup()
{
	SubscribeEventCallbackFunction("KeyPressed", InputSystem::Event_KeyPressed);
	SubscribeEventCallbackFunction("KeyReleased", InputSystem::Event_KeyReleased);

	UpdateCursorInfo(); // set the cursor to aim at the center of the screen
}

void InputSystem::Shutdown()
{

}

void InputSystem::BeginFrame()
{
	UpdateCursorVisibilityStatus();
	UpdateCursorInfo();
	ShowOrHideCursorBasedOnCursorMode();
	UpdateController();
}


void InputSystem::ShowOrHideCursorBasedOnCursorMode()
{
	// only show the cursor when we are not hide our cursor and our cursor is inside the client area
	if (m_cursorState.m_desiredHiddenMode != m_cursorState.m_hiddenMode)
	{
		if (m_cursorState.m_desiredHiddenMode)
		{
			// the display count is decremented by one
			// successfully means exit the while loop
			while (ShowCursor(false) > 0)
			{
			}
		}
		else
		{
			//  the display count is incremented by one
			while (ShowCursor(true) <= 0)
			{
			}
		}
		m_cursorState.m_hiddenMode = m_cursorState.m_desiredHiddenMode;
	}
	
	EnsureCursorVisibility(!m_cursorState.m_hiddenMode);
}

void InputSystem::EnsureCursorVisibility(bool visibility)
{
	if (visibility)
	{
		while (ShowCursor(true) <= 0)
		{
		}
	}
	else
	{
		while (ShowCursor(false) > 0)
		{
		}
	}
}

void InputSystem::UpdateCursorVisibilityStatus()
{
	HWND winPtr = ::GetActiveWindow();
	// The mouse should be visible and absolute if any of the following are true
	// The window does not have focus.
	// The dev console is open.
	// these are override logic
	if (!winPtr || g_theDevConsole->IsOpen())
	{
		// recorded previous state
		if (!m_isRecorded)
		{
			m_recordedCursorHiddenMode = m_cursorState.m_desiredHiddenMode;
			m_recordedCursorRelativeMode = m_cursorState.m_relativeMode;
			m_isRecorded = true;
		}

		// override logic
		m_cursorState.m_desiredHiddenMode = false;
		m_cursorState.m_relativeMode = false;
	}
	else
	{
		if (m_isRecorded) // resume previous cursor setting
		{
			m_cursorState.m_desiredHiddenMode = m_recordedCursorHiddenMode;
			m_cursorState.m_relativeMode = m_recordedCursorRelativeMode;
			m_isRecorded = false;
		}
	}
	// the mouse should be hidden and relative while playing the game
	// but some games are not (Doomenstein vs Vaporum)
	//else
	//{
	//	m_cursorState.m_desiredHiddenMode = true;
	//	m_cursorState.m_relativeMode = true;
	//}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// override logic
	// allow the game to override the hidden mode and relative mode
	// if (m_overrideMode)
	// {
	// 	m_cursorState.m_desiredHiddenMode = !m_recordedCursorHiddenMode;
	// 	m_cursorState.m_relativeMode = m_recordedCursorRelativeMode;
	// }
}

void InputSystem::UpdateCursorInfo()
{
	// if we are in relative mode
	if (m_cursorState.m_relativeMode)
	{
		HWND windowHandle = GetActiveWindow();
		POINT cursorCoords;
		RECT clientRect;
		::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left
		::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
		::GetClientRect(windowHandle, &clientRect); // size of window interior(0,0 to width, height)

		float desktopWidth = (float)(clientRect.right - clientRect.left);
		float desktopHeight = (float)(clientRect.bottom - clientRect.top);

		IntVec2 clientCenter((int)(desktopWidth * 0.5f), (int)(desktopHeight * 0.5f));
		IntVec2 cursorPos((int)cursorCoords.x, (int)cursorCoords.y);

		// return the current frame cursor delta in pixels in client space
		// however, if last frame player switch to other app and switch back, it should consider the delta should be consider as zero
		if (m_cursorState.m_desiredHiddenMode == true && m_cursorState.m_hiddenMode == false)
		{
			m_cursorState.m_cusorClientDelta = IntVec2();
		}
		else
		{
			m_cursorState.m_cusorClientDelta = cursorPos - m_cursorClientPosLastFrame;
		}
		m_cursorState.m_cursorClientPosition = cursorPos;

		// set the cursor back to center, in screen space
		POINT clientCenterInScreen;
		clientCenterInScreen.x = (LONG)clientCenter.x;
		clientCenterInScreen.y = (LONG)clientCenter.y;
		::ClientToScreen(windowHandle, &clientCenterInScreen);
		::SetCursorPos(clientCenterInScreen.x, clientCenterInScreen.y);

		// and record this frame to use it as last frame pos for the update
		::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left, the pos might not be exact as the center
		::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
		m_cursorClientPosLastFrame.x = (int)(cursorCoords.x);
		m_cursorClientPosLastFrame.y = (int)(cursorCoords.y);
	}
	// if we are not in relative mode
	else
	{
		HWND windowHandle = GetActiveWindow();
		POINT cursorCoords;
		::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left
		::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
		IntVec2 cursorPos((int)cursorCoords.x, (int)cursorCoords.y);

		m_cursorState.m_cusorClientDelta = IntVec2(0, 0);
		m_cursorState.m_cursorClientPosition = cursorPos;
	}

	// however if the window loses focus
	HWND winPtr = ::GetActiveWindow();
	if (!winPtr)
	{
		m_cursorState.m_cusorClientDelta = IntVec2(0, 0);
	}
}


void InputSystem::SetCursorMode(bool hiddenMode, bool relativeMode)
{
	g_theInput->m_cursorState.m_desiredHiddenMode = hiddenMode;
	g_theInput->m_cursorState.m_relativeMode = relativeMode;
}

IntVec2 InputSystem::GetCursorClientDelta() const
{
	return m_cursorState.m_cusorClientDelta;
}

Vec2 InputSystem::GetNormalizedCursorPos() const
{
	HWND windowHandle = GetActiveWindow();
	// HWND windowHandle = (HWND)Window::s_theWindow;
	POINT cursorCoords;
	RECT clientRect;
	::GetCursorPos(&cursorCoords);					// in screen coordinates, (0,0) top-left
	::ScreenToClient(windowHandle, &cursorCoords);	// relative to the window interior
	::GetClientRect(windowHandle, &clientRect);		// size of window interior(0,0 to width, height)

	float cursorX = float(cursorCoords.x) / float(clientRect.right); // normalized x position
	float cursorY = float(cursorCoords.y) / float(clientRect.bottom); // normalized y position
	return Vec2(cursorX, 1.f - cursorY);			// we want (0,0) in the bottom-left
}

bool InputSystem::IsCursorInThisNormalizedRegion(AABB2 region) const
{
	// check if player click in the widget bounds
	Vec2 cursorPos = GetNormalizedCursorPos();
	if (cursorPos > region.m_mins &&
		cursorPos < region.m_maxs)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool InputSystem::IsCursorInThisPhysicalScreenRegion(AABB2 const& region, Window const* windows) const
{
	// check if player click in the widget bounds
	Vec2 cursorPos = GetNormalizedCursorPos();
	IntVec2 screenDimensions = windows->GetWindowDimensions();
	cursorPos.x = cursorPos.x * screenDimensions.x;
	cursorPos.y = cursorPos.y * screenDimensions.y;

	if (cursorPos > region.m_mins &&
		cursorPos < region.m_maxs)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void InputSystem::SetDesiredCursorMode(bool hiddenMode, bool relativeMode)
{
	m_cursorState.m_desiredHiddenMode = hiddenMode;
	m_cursorState.m_relativeMode = relativeMode;
}

void InputSystem::UpdateController()
{
	for (int XControllerIndex = 0; XControllerIndex < NUM_XBOX_CONTROLLERS; ++XControllerIndex)
	{
		 // update the controller caring not about if it is connected, the update function will update the connection status
		 m_controllers[XControllerIndex].Update();
	}
}

void InputSystem::EndFrame()
{
	// and record this frame to use it as last frame pos for the update
	//POINT cursorCoords;
	//HWND windowHandle = GetActiveWindow();
	//::GetCursorPos(&cursorCoords); // in screen coordinates, (0,0) top-left, the pos might not be exact as the center
	//::ScreenToClient(windowHandle, &cursorCoords); // relative to the window interior
	//m_cursorClientPosLastFrame.x = (int)(cursorCoords.x);
	//m_cursorClientPosLastFrame.y = (int)(cursorCoords.y);

	// copy over key states to last frame
	for (int i = 0; i < NUM_KEYCODES; ++i)
	{
		m_keyStates[i].m_keyPressedLastFrame = m_keyStates[i].m_keyPressedThisFrame;
	}
	// copy over xbox controller button states to last frame
	// for (int XControllerIndex = 0; XControllerIndex < NUM_XBOX_CONTROLLERS; ++XControllerIndex)
	// {
	// 	if (m_controllers[XControllerIndex].m_isConnected)
	// 	{
	// 		for (int i = 0; i < NUM_XBOX_BUTTONS; ++i)
	// 		{
	// 			m_controllers[XControllerIndex].m_buttons[i].m_keyPressedLastFrame = m_controllers[XControllerIndex].m_buttons[i].m_keyPressedThisFrame;
	// 		}
	// 	}
	// }

}

bool InputSystem::WasKeyJustPressed(unsigned char keyCode)
{
return (m_keyStates[keyCode].m_keyPressedThisFrame == true &&
	m_keyStates[keyCode].m_keyPressedLastFrame == false);
}

bool InputSystem::WasKeyJustReleased(unsigned char keyCode)
{
	return (m_keyStates[keyCode].m_keyPressedThisFrame == false &&
		m_keyStates[keyCode].m_keyPressedLastFrame == true);
}

bool InputSystem::IsKeyDown(unsigned char keyCode)
{
	return m_keyStates[keyCode].m_keyPressedThisFrame == true;
}

void InputSystem::HandleKeyPressed(unsigned char keyCode)
{
	// use the array of bool to detect whether the key is pressed down
	// only the app could change this array of bool
	m_keyStates[keyCode].m_keyPressedThisFrame = true;
}

void InputSystem::HandleKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].m_keyPressedThisFrame = false;
}

XboxController const& InputSystem::GetController(int controllerID)
{
	return m_controllers[controllerID];
}

STATIC bool InputSystem::Event_KeyPressed(EventArgs& args)
{
	if (!g_theInput)
	{
		return false;
	}
	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);
	g_theInput->HandleKeyPressed(keyCode);
	return true;
}

STATIC bool InputSystem::Event_KeyReleased(EventArgs& args)
{
	if (!g_theInput)
	{
		return false;
	}
	unsigned char keyCode = (unsigned char)args.GetValue("KeyCode", -1);
	g_theInput->HandleKeyReleased(keyCode);
	return true;
}

