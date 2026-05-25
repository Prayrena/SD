#pragma once
#include "Engine/Input/XboxController.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <string>

// for other files to include this hpp file to have access to all virtual key value
// global const
extern const unsigned char KEYCODE_F1;
extern const unsigned char KEYCODE_F2;
extern const unsigned char KEYCODE_F3;
extern const unsigned char KEYCODE_F4;
extern const unsigned char KEYCODE_F5;
extern const unsigned char KEYCODE_F6;
extern const unsigned char KEYCODE_F7;
extern const unsigned char KEYCODE_F8;
extern const unsigned char KEYCODE_F9;
extern const unsigned char KEYCODE_F10;
extern const unsigned char KEYCODE_F11;
extern const unsigned char KEYCODE_F12;
extern const unsigned char KEYCODE_ESC;

extern const unsigned char KEYCODE_NUM1;
extern const unsigned char KEYCODE_NUM2;
extern const unsigned char KEYCODE_NUM3;
extern const unsigned char KEYCODE_NUM4;
extern const unsigned char KEYCODE_NUM5;
extern const unsigned char KEYCODE_NUM6;
extern const unsigned char KEYCODE_NUM7; 
extern const unsigned char KEYCODE_NUM8;
extern const unsigned char KEYCODE_NUM9;

extern const unsigned char KEYCODE_UPARROW; 
extern const unsigned char KEYCODE_DOWNARROW;
extern const unsigned char KEYCODE_LEFTARROW;
extern const unsigned char KEYCODE_RIGHTARROW;

extern const unsigned char KEYCODE_HOME;
extern const unsigned char KEYCODE_END;

extern const unsigned char KEYCODE_INSERT;
extern const unsigned char KEYCODE_DELETE;
extern const unsigned char KEYCODE_ENTER;
extern const unsigned char KEYCODE_BACKSPACE;

extern const unsigned char KEYCODE_LEFT_MOUSE;
extern const unsigned char KEYCODE_RIGHT_MOUSE;
extern const unsigned char KEYCODE_LEFTBRACKET;
extern const unsigned char KEYCODE_RIGHTBRACKET;

extern const unsigned char KEYCODE_LESSTHAN;
extern const unsigned char KEYCODE_GREATERTHAN;

extern const unsigned char KEYCODE_TILDE;
extern const unsigned char KEYCODE_SHIFT;

constexpr int NUM_KEYCODES = 256;
constexpr int NUM_XBOX_CONTROLLERS = 4;

class Window;

struct InputConfig
{
	InputSystem*	m_inputSystem = nullptr;
	std::string		m_windowTitle = "untitled App";
	float			m_clientAspect = 2.f;
	bool			m_isFullscreen = false;
};

struct CursorState
{
	IntVec2 m_cusorClientDelta;
	IntVec2 m_cursorClientPosition;

	// set the desired mode and current mode differently so it the show cursor is set at the start
	bool m_hiddenMode = true;
	bool m_desiredHiddenMode = false;
	bool m_relativeMode = true;
};

class InputSystem
{
friend class Widget;
friend struct TextLine;

public:
	InputSystem( InputConfig const& config );
	~InputSystem();
	void Startup();
	void Shutdown();
	void BeginFrame();
	void UpdateController();
	void EndFrame();

	bool WasKeyJustPressed(unsigned char keyCode);
	bool WasKeyJustReleased(unsigned char keyCode);
	bool IsKeyDown(unsigned char keyCode);
	void HandleKeyPressed(unsigned char keyCode);
	void HandleKeyReleased(unsigned char keyCode);
	XboxController const& GetController(int controllerID);

	// static function for the event system
	static bool Event_KeyPressed(EventArgs& args);
	static bool Event_KeyReleased(EventArgs& args);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// cursor related functions
	void ShowOrHideCursorBasedOnCursorMode();
	void EnsureCursorVisibility(bool visibility);
	void UpdateCursorInfo();
	void UpdateCursorVisibilityStatus();

	// mouse related functions
	// hidden mode controls whether the cursor is visible or not.
	// Relative mode will calculate a cursor client delta and reset the cursor to the client region center each frame
	// Both of these together can be used to implement an FPS-style mouse look camera
	void SetCursorMode(bool hiddenMode, bool relativeMode);

	// returns the current frame cursor delta in pixels, relative to the client region
	// only valid in relative mode, will be zero otherwise
	IntVec2 GetCursorClientDelta() const;

	// Returns the cursor position, normalized to the range [0, 1], relative to the client region
	// with the y-axis inverted to map from windows conventions to game screen camera conventions
	Vec2 GetNormalizedCursorPos() const;
	bool IsCursorInThisNormalizedRegion(AABB2 region) const;
	bool IsCursorInThisPhysicalScreenRegion(AABB2 const& region, Window const* windows) const;

	CursorState m_cursorState;
	IntVec2		m_cursorClientPosLastFrame;

	// set override cursor mode
	void		SetDesiredCursorMode(bool hiddenMode, bool relativeMode);
	bool		m_isRecorded = false;
	bool		m_recordedCursorHiddenMode = false;
	bool		m_recordedCursorRelativeMode = false;

protected:
	KeyButtonState m_keyStates[NUM_KEYCODES];
	XboxController m_controllers[NUM_XBOX_CONTROLLERS];
	InputConfig	   m_config;
};