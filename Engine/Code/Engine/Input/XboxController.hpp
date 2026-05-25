#pragma once
#include "Engine/Input/AnalogJoystick.hpp"
#include "Engine/Input/KeyButtonState.hpp"

//class AnalogJoystick;
//class KeyButtonState;

enum XboxButtonID
{
	XBOX_BUTTON_START,
	XBOX_BUTTON_BACK,

	XBOX_BUTTON_A,
	XBOX_BUTTON_B,
	XBOX_BUTTON_X,
	XBOX_BUTTON_Y,

	XBOX_BUTTON_DPAD_RIGHT,
	XBOX_BUTTON_DPAD_LEFT,
	XBOX_BUTTON_DPAD_UP,
	XBOX_BUTTON_DPAD_DOWN,

	XBOX_BUTTON_LSHOULDER,
	XBOX_BUTTON_RSHOULDER,

	XBOX_BUTTON_LTHUMB,
	XBOX_BUTTON_RTHUMB,

	NUM_XBOX_BUTTONS
};

class XboxController
{
	friend class InputSystem;// let the input system access the private values

public:
	XboxController();
	~XboxController();
	bool				  IsConnected() const;
	int					  GetControllerID() const;
	AnalogJoystick const& GetLeftstick() const;
	AnalogJoystick const& GetRightstick() const;
	float				  GetLeftTrigger() const;
	float				  GetRightTrigger() const;
	KeyButtonState const& GetButton(XboxButtonID buttonID) const;
	bool				  IsButtonDown(XboxButtonID buttonID) const;
	bool				  WasButtonJustPressed(XboxButtonID buttonID) const;
	bool				  WasButtonJustReleased(XboxButtonID buttonID) const;
		      
private:
	void Update();
	void Reset();
	void UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY);
	void UpdateTrigger(float& out_triggerValue, unsigned char rawValue);
	void UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag);

private:
	int	 		   m_id = -1;
	bool		   m_isConnected = true;
	float		   m_LTrigger = 0.f;
	float		   m_RTrigger = 0.f;
	KeyButtonState m_buttons[NUM_XBOX_BUTTONS]; // for each of the item inside the array, it contains two bool

	AnalogJoystick m_LStick;
	AnalogJoystick m_RStick;
};