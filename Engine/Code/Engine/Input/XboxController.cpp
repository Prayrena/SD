#include "Engine/Input/XboxController.hpp"

#include <Windows.h> 
#include <Xinput.h> //accessing the controller's raw data
#pragma comment( lib, "xinput9_1_0" ) // Xinput 1_4 doesn't work in older Windows versions; use XInput 9_1_0 explicitly for best compatibility

#include "Engine/Math/MathUtils.hpp"
#include <limits.h>

XboxController::XboxController()
{

}

XboxController::~XboxController()
{

}

//#include "Engine/Input/AnalogJoystick.hpp"
//#include "Engine/Input/KeyButtonState.hpp"


bool XboxController::IsConnected() const
{
	//Read raw controller state via XInput API
	XINPUT_STATE xboxControllerState = {};// initialize the struct for later x input state
	DWORD errorStatus = XInputGetState(m_id, &xboxControllerState);

	// if successfully connect to the controller
	if (errorStatus != ERROR_SUCCESS)
	{
		return true;
	}
	return false;
}

int XboxController::GetControllerID() const
{
	return m_id;
}

AnalogJoystick const& XboxController::GetLeftstick() const
{
	return m_LStick;
}

AnalogJoystick const& XboxController::GetRightstick() const
{
	return m_RStick;
}

float XboxController::GetLeftTrigger() const
{
	return m_LTrigger;
}

float XboxController::GetRightTrigger() const
{
	return m_RTrigger;
}

KeyButtonState const& XboxController::GetButton(XboxButtonID buttonID) const
{
	return m_buttons[buttonID];
}

bool XboxController::IsButtonDown(XboxButtonID buttonID) const
{
	return (m_buttons[buttonID].m_keyPressedThisFrame == true);
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
	// return (m_buttons[buttonID].m_keyPressedThisFrame == true &&
	// 	m_buttons[buttonID].m_keyPressedLastFrame == false);
	if (m_buttons[buttonID].m_keyPressedThisFrame && !m_buttons[buttonID].m_keyPressedLastFrame)
	{
		return true;
	}
	else return false;
}

bool XboxController::WasButtonJustReleased(XboxButtonID buttonID) const
{
	return (m_buttons[buttonID].m_keyPressedThisFrame == false &&
		m_buttons[buttonID].m_keyPressedLastFrame == true);
}

/// <update functions>
/// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void XboxController::Update()
{
	//Read raw controller state via XInput API
	XINPUT_STATE xboxControllerState = {};// initialize the struct for later x input state
	DWORD errorStatus = XInputGetState(m_id, &xboxControllerState);

	// if the controller fail to connect, then it will reset all the bottons
	if ( errorStatus != ERROR_SUCCESS )
	{
		Reset();
		m_isConnected = false;
		return;
	}
	m_isConnected = true;
	
	//Update internal data structure(s) based on raw controller state
	XINPUT_GAMEPAD const& state = xboxControllerState.Gamepad; 

	UpdateJoystick(m_LStick, state.sThumbLX, state.sThumbLY);
	UpdateJoystick(m_RStick, state.sThumbRX, state.sThumbRY);

	UpdateTrigger(m_LTrigger, state.bLeftTrigger);
	UpdateTrigger(m_RTrigger, state.bRightTrigger);

	UpdateButton(XBOX_BUTTON_A,			  state.wButtons, XINPUT_GAMEPAD_A);
	UpdateButton(XBOX_BUTTON_B,			  state.wButtons, XINPUT_GAMEPAD_B);
	UpdateButton(XBOX_BUTTON_X,			  state.wButtons, XINPUT_GAMEPAD_X);
	UpdateButton(XBOX_BUTTON_Y,			  state.wButtons, XINPUT_GAMEPAD_Y);

	UpdateButton(XBOX_BUTTON_DPAD_UP,	  state.wButtons, XINPUT_GAMEPAD_DPAD_UP);
	UpdateButton(XBOX_BUTTON_DPAD_DOWN,   state.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
	UpdateButton(XBOX_BUTTON_DPAD_LEFT,   state.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
	UpdateButton(XBOX_BUTTON_DPAD_RIGHT,  state.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);

	UpdateButton(XBOX_BUTTON_BACK,		  state.wButtons, XINPUT_GAMEPAD_BACK);
	UpdateButton(XBOX_BUTTON_START,		  state.wButtons, XINPUT_GAMEPAD_START);

	UpdateButton(XBOX_BUTTON_LSHOULDER,   state.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
	UpdateButton(XBOX_BUTTON_RSHOULDER,   state.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
	UpdateButton(XBOX_BUTTON_LTHUMB,	  state.wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
	UpdateButton(XBOX_BUTTON_RTHUMB,	  state.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
}

void XboxController::UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY)
{
	float rawInputX = static_cast<float>(rawX);
	float rawInputY = static_cast<float>(rawY);

	float shortMin = static_cast<float>(SHRT_MIN);
	float shortMax = static_cast<float>(SHRT_MAX);
	float RawNormalizedX = RangeMapClamped(rawInputX, shortMin, shortMax, -1.f, 1.f);
	float RawNormalizedY = RangeMapClamped(rawInputY, shortMin, shortMax, -1.f, 1.f);

	out_joystick.UpdatePosition(RawNormalizedX, RawNormalizedY);
}

void XboxController::UpdateTrigger(float& out_triggerValue, unsigned char rawValue)
{
	float rawInputValue = static_cast<float> (rawValue);
	out_triggerValue = RangeMapClamped(rawInputValue, 0.f, 255.f, 0.f, 1.f);
}

void XboxController::UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag)
{
	// make a nickname for the button
	KeyButtonState& button = m_buttons[buttonID];

	// copy outdated this frame info to the last frame boolean
	button.m_keyPressedLastFrame = button.m_keyPressedThisFrame;
	// if the & operation between two short equal to flag, this means the button is pressed
	// button.m_keyPressedThisFrame = ((buttonFlags & buttonFlag) == buttonFlag);
	if ((buttonFlags & buttonFlag) == buttonFlag)
	{
		button.m_keyPressedThisFrame = true;
	}
	else button.m_keyPressedThisFrame = false;
}
/// <summary>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>
void XboxController::Reset()
{
	// reset buttons
	for (int i = 0; i < NUM_XBOX_BUTTONS; ++i)
	{
		// when the controller lose connection, reset all the button state to false
		m_buttons[i].m_keyPressedThisFrame = false;
	}

	// reset triggers
	m_LTrigger = 0.f;
	m_RTrigger = 0.f;

	// reset analog joy stick
	m_LStick.Reset();
	m_RStick.Reset();
}


