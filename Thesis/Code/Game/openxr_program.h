#pragma once
#include "Engine/Input/XRInputSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include <set>

struct Options;
struct IPlatformPlugin;
struct IGraphicsPlugin;
struct XrPosef;
class Shader;
class PlayerHand;

enum TextureID
{
	TESTUV,
	VICTORY_MENU,
	NUM_TEXTURES
};

enum MaterialType
{
	CRAWLER,
	NUM_MATERIALS
};

enum ShaderID
{
	WORLD,
	NUM_SHADERS
};

class OpenXrProgram 
{
friend class PlayerHand;

public:
    OpenXrProgram(const std::shared_ptr<Options>& options, const std::shared_ptr<IPlatformPlugin>& platformPlugin);

    ~OpenXrProgram();

    // Create an Instance and other basic instance-level initialization.
    void CreateInstance();

	bool m_openXRAvaible = true;	// if this is false, the creation of openXR system is failed and we are using this to know if the headset if is not connected

    // Select a System for the view configuration specified in the Options
    void InitializeSystem();

    // Initialize the graphics device for the selected system.
    void InitializeDevice();

    // Create a Session and other basic session-level initialization.
    void InitializeSession();


    // Process any events in the event queue.
    void PollEvents(bool* exitRenderLoop, bool* requestRestart);

    // Manage session lifecycle to track if RenderFrame should be called.
    bool IsSessionRunning() const;

    // Manage session state to track if input should be processed.
    bool IsSessionFocused() const;

    // Sample input actions and generate haptic feedback.
    void PollActions();

    // Create and submit a frame.
    void Render();
	void RenderVR();

	void XREndFrame();

	void Startup();
	void RunFrame();
	void BeginFrame();
	void EndFrame();
	void Update();
	void Shutdown();

	// log info
	void LogViewConfigurations();
	void LogInstanceInfo();
	void LogActionSourceName(XrAction action, const std::string& actionName) const;
	void LogReferenceSpaces();
	void LogEnvironmentBlendMode(XrViewConfigurationType type);

	// input
	void InitializeActions();
	void UpdatePlayerCurrentHeadPose();
	void RecenterPlayer();

	XRPose m_lastTimeResetValue;
	XRPose m_resetGameWorldPose;
	Mat44  m_eyesCenterPosInGameWorld;

	void CreateVisualizedSpaces();
	void CreateInstanceInternal();

	XrSpace GetRotatedSpaceForLocatingViews();
	XrSpace m_rotatedSpace;

	XrEventDataBaseHeader const* TryReadNextEvent();

	void HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart);

	bool RenderLayer(XrTime predictedDisplayTime, std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
		XrCompositionLayerProjection& layer);

    // Get preferred blend mode based on the view configuration specified in the Options
    XrEnvironmentBlendMode GetPreferredBlendMode();

	// Function to create a quaternion from an axis and an angle
	XrQuaternionf createQuaternionFromAxisAngle(float x, float y, float z);

	// debug window
	float  m_windowAspectRatio = 2.f;
	Camera m_windowsCamera;
	Camera m_screenCamera;

	// event system functions
	bool   m_isQuitting = false;

	bool HandleQuitRequested();
	static bool Event_Quit(EventArgs& args);

	void LoadGameShaders();
	Shader* m_shaders[NUM_SHADERS];

private:
	void LoadMaterialAssets();

	const std::shared_ptr<const Options> m_options;
	std::shared_ptr<IPlatformPlugin> m_platformPlugin;
	std::shared_ptr<IGraphicsPlugin> m_graphicsPlugin;
	XrInstance m_instance{ XR_NULL_HANDLE };
	XrSession m_session{ XR_NULL_HANDLE };
	XrSpace m_appSpace{ XR_NULL_HANDLE };
	XrSystemId m_systemId{ XR_NULL_SYSTEM_ID };

	// std::vector<XrSpace> m_visualizedSpaces;
	// XrSpace m_worldSpace;
	XrSpace m_viewSpace;

	// Application's current lifecycle state according to the runtime
	XrSessionState m_sessionState{ XR_SESSION_STATE_UNKNOWN };
	bool m_sessionRunning{ false };

	XrEventDataBuffer m_eventDataBuffer;
	InputState m_input;

	const std::set<XrEnvironmentBlendMode> m_acceptableBlendModes;
};



