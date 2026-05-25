#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Net/NetSystem.hpp"
#include <string>
#include <vector>
#include <Mutex>

class Timer;
class NamedStrings;
typedef NamedStrings EventArgs;
class BitmapFont;
class Renderer;
class Camera;

extern Renderer* g_theRenderer;

enum class DevConsoleMode
{
	OPENFULL,
	SYSTEMSTATISTICS,
	BOTTOMSCREEN,
	NUM_DEVCONSOLE_MODE
};

struct DevConsoleLine
{
	DevConsoleLine(std::string const& text, Rgba8 const& color, Vec2 alignment = Vec2(0.f, 0.5f), 
		float duration = -1.f, Rgba8 const& disappearColor = Rgba8::WHITE)
		: m_displayInfo(text), m_startColor(color), m_alignment(alignment), m_duration(duration),
		m_endColor(disappearColor)
	{
		m_timeLeft = m_duration;
	}

	std::string		m_displayInfo;
	Rgba8			m_startColor;
	Rgba8			m_currentColor;
	Rgba8			m_endColor;

	double			m_timePrinted = 0.f;
	int				m_frameNumberPrinted = 0;
	float			m_duration = -1.f; // default means infinite
	float			m_timeLeft = -1.f;
	Vec2			m_alignment = Vec2(0.f, 0.5f);
};

// A renderer and camera must be provided
struct DevConsoleConfig
{
	Renderer*		m_renderer = g_theRenderer;
	Camera*			m_camera = nullptr;

	float			m_numLinesOnScreen = 25.f;	// define how many text line to show on the screen, allow half text line or part of it showing on screen
	int				m_maxCommandHistory = 128;
	int				m_maxLineHistory = 2048;	// max display lines kept in memory; oldest lines are trimmed
	BitmapFont*		m_font; // todo: change to std::string fontName
	float			m_fontAspect = 0.6f;
	float			m_lineHeightAndTextBoxRatio = 0.8f;
	float			m_insertionHeightLineHeightRatio = 0.8f;
};

class DevConsole
{
friend class EventSystem;
friend class NetSystem;

public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole() {};
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	bool m_executeFoundSubscriber = false;
	bool m_executeFoundSubscriberButKeyIsWrong = false;

	void Execute(std::string const& consoleCommandText);
	void PrintExecuteCommandOnScreen(std::string consoleCommandText);
	void AddLine(std::string const& text, Rgba8 const& color);
	void AddInstruction(std::string const& text, Rgba8 const& color = INFO_MAJOR);
	// the game will say which camera to use
	void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr); // the game will tell the engine to use the render that the application summon
	void RenderConsole(AABB2 const& bounds, Renderer& renderer);// write over the whole window

	DevConsoleMode GetMode() const;
	void SetMode(DevConsoleMode mode);
	void ToggleMode(); // loop through all the modes	
	void ToggleOpen(); // Toggles between open and closed
	bool IsOpen();

	static const Rgba8 INFO_ERROR;
	static const Rgba8 INFO_WARNING; // potential problem (like an XML file missing)
	static const Rgba8 INFO_MAJOR; // "loading new maps"
	static const Rgba8 INFO_MINOR; // "spawning 3 Scorpio entities"

	static const Rgba8 INPUT_TEXT;
	static const Rgba8 INPUT_INSERTION_POINT;

	// handle key input
	static bool Event_KeyPressed(EventArgs& args);

	// handle char input by appending valid characters to our current input line
	static bool Event_CharInput(EventArgs& args);

	// clear all lines of text
	static bool Command_Clear(EventArgs& args);

	// Display all currently registered commands in the event system
	static bool Command_Help(EventArgs& args);

	// display the message
	static bool Command_Echo(EventArgs& args);

	static bool Command_ControlInstructions(EventArgs& args); 
	static bool Command_ChangeTimeScale(EventArgs& args);

	DevConsoleConfig				m_config;
protected:

	bool							m_isOpen = false; // true if the dev console is currently visible and accept input

	std::vector<DevConsoleLine>		m_lines;	//todo: support a max limited # of lines(e.g. fixed circular buffer)
	mutable std::mutex				m_linesMutex;

	std::vector<DevConsoleLine>		m_controlInstructions;	// store all the control instruction info
	std::vector<DevConsoleLine>		m_registeredEvents;	//todo: support a max limited # of lines(e.g. fixed circular buffer)
	// history of all commands executed
	std::vector<DevConsoleLine>		m_commandHistory;


	std::string						m_inputTexts; // Our current line of input text

	int								m_insertionPointPosIndex; // index of the insertion point in our current input text
	bool							m_insertionPointVisible = true; // true if our insertion point is currently in the visible of blinking
	Timer*							m_insertionPointBlinkTimer;
	float							m_originalInsertionScale;

	DevConsoleMode					m_mode = DevConsoleMode::OPENFULL;
	int								m_frameNumber = 0; // how many frames has the game running

	Timer*							m_systemInfoTimer;
	int								m_fps = 0;
	float							m_systemTime = 0.f;
	float							m_GameTime = 0.f;

	// our current index in our history of commands as we are scrolling
	int								m_historyIndex = -1;

	// scroll offset for viewing older lines with mouse wheel (0 = latest lines visible)
	int								m_scrollOffset = 0;
	static bool Event_MouseWheel(EventArgs& args);
	Rgba8							m_frameColor = Rgba8::PURPLE_BLUE;
	Rgba8							m_instructionColor = Rgba8::LIGHT_ORANGE;
	Rgba8							m_systemColor = Rgba8::LIGHT_ORANGE;
};