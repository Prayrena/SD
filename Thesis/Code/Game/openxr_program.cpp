#include "Engine/core/Clock.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Model/Material.hpp"
#include "Engine/core/DevConsole.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/core/EventSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "Engine/Physics/ThePhysX.hpp"
#include "Engine\Renderer\DebugRender.hpp"
#include "ThirdParty/OpenXR/include/openxr/openxr.h"
#include "ThirdParty/OpenXR/src/common/xr_linear.h"
#include "ThirdParty/OpenXR/Application/options.h"
#include "platformdata.h"
#include "platformplugin.h"
#include "Game/GameCommon.hpp"
#include "openxr_program.h"
#include "Game/Game.hpp"
#include "Game/HUD.hpp"
#include "Game/VRPlayer.hpp"
#include "Game/WinPlayer.hpp"
#include <array>
#include <cmath>
#include <set>

extern OpenXrProgram* g_theApp;// global variable must be define in the cpp
extern Clock* g_theGameClock;
extern VRPlayer* g_theVRPlayer;
extern WinPlayer* g_theWinPlayer;

Game* g_theGame = nullptr;
Renderer* g_theRenderer = nullptr;
InputSystem* g_theInput = nullptr;
AudioSystem* g_theAudio = nullptr;
Window* g_theWindow = nullptr;
BitmapFont* g_consoleFont = nullptr;
DevConsole* g_theDevConsole = nullptr;
ThePhysX* g_thePhysX = nullptr;

Material* g_materials[MaterialType::NUM_MATERIALS];

extern HUD* g_theHUD;

#if !defined(XR_USE_PLATFORM_WIN32)
#define strcpy_s(dest, source) strncpy((dest), (source), sizeof(dest))
#endif

inline std::string GetXrVersionString(XrVersion ver) {
    return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
}

namespace Math 
{
    namespace Pose 
    {
        XrPosef Identity() 
        {
            XrPosef t{};
            t.orientation.w = 1;
            return t;
        }

        XrPosef Translation(const XrVector3f& translation) 
        {
            XrPosef t = Identity();
            t.position = translation;
            return t;
        }

        XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) 
        {
            XrPosef t = Identity();
            t.orientation.x = 0.f;
            t.orientation.y = std::sin(radians * 0.5f);
            t.orientation.z = 0.f;
            t.orientation.w = std::cos(radians * 0.5f);
            t.position = translation;
            return t;
        }

	    XrPosef RotateCCWAboutZAxis(XrVector3f translation, float z)
	    {
		    float halfAngle = 3.1415926f * 0.5f * z;			

		    XrPosef t = Identity();
		    t.orientation.x = 0.f;
		    t.orientation.y = 0.f;
		    t.orientation.z = std::sin(halfAngle);
		    t.orientation.w = std::cos(halfAngle);
		    t.position = translation;
		    return t;
	    }
    }  // namespace Pose
}  // namespace Math

inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) 
{
    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
    if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) {
        // Render head-locked 2m in front of device.
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f}),
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else {
        throw std::invalid_argument(Fmt("Unknown reference space type '%s'", referenceSpaceTypeStr.c_str()));
    }
    return referenceSpaceCreateInfo;
}


    OpenXrProgram::OpenXrProgram(const std::shared_ptr<Options>& options, const std::shared_ptr<IPlatformPlugin>& platformPlugin)
        : m_options(options),
            m_platformPlugin(platformPlugin),
            m_acceptableBlendModes{XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE,
                                    XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND} 
    {
    }

    OpenXrProgram::~OpenXrProgram()
    {
        if (m_input.actionSet != XR_NULL_HANDLE) {
            for (auto hand : {Side::LEFT, Side::RIGHT}) {
                xrDestroySpace(m_input.handSpace[hand]);
            }
            xrDestroyActionSet(m_input.actionSet);
        }

        // xrDestroySpace(m_worldSpace);

        if (m_openXRAvaible)
        {
			xrDestroySpace(m_viewSpace);

			if (m_appSpace != XR_NULL_HANDLE)
			{
				xrDestroySpace(m_appSpace);
			}

			if (m_session != XR_NULL_HANDLE)
			{
				xrDestroySession(m_session);
			}

			if (m_instance != XR_NULL_HANDLE)
			{
				xrDestroyInstance(m_instance);
			}
        }
    }

    static void LogLayersAndExtensions() 
    {
        // Write out extension properties for a given layer.
        const auto logExtensions = [](const char* layerName, int indent = 0) {
            uint32_t instanceExtensionCount;
            CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));
            std::vector<XrExtensionProperties> extensions(instanceExtensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
            CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t)extensions.size(), &instanceExtensionCount,
                                                                extensions.data()));

            const std::string indentStr(indent, ' ');
            Log::Write(Log::Level::Verbose, Fmt("%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount));
            for (const XrExtensionProperties& extension : extensions) {
                Log::Write(Log::Level::Verbose, Fmt("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName,
                                                    extension.extensionVersion));
            }
        };

        // Log non-layer extensions (layerName==nullptr).
        logExtensions(nullptr);

        // Log layers and any of their extensions.
        {
            uint32_t layerCount;
            CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
            std::vector<XrApiLayerProperties> layers(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
            CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t)layers.size(), &layerCount, layers.data()));

            Log::Write(Log::Level::Info, Fmt("Available Layers: (%d)", layerCount));
            for (const XrApiLayerProperties& layer : layers) {
                Log::Write(Log::Level::Verbose,
                            Fmt("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName,
                                GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description));
                logExtensions(layer.layerName, 4);
            }
        }
    }

    void OpenXrProgram::LogInstanceInfo()
    {
        CHECK(m_instance != XR_NULL_HANDLE);

        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        CHECK_XRCMD(xrGetInstanceProperties(m_instance, &instanceProperties));

        Log::Write(Log::Level::Info, Fmt("Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName,
                                            GetXrVersionString(instanceProperties.runtimeVersion).c_str()));
    }

    void OpenXrProgram::CreateInstanceInternal()
    {
        CHECK(m_instance == XR_NULL_HANDLE);

        // Create union of extensions required by platform and graphics plugins.
        std::vector<const char*> extensions;

        // Transform platform and graphics extension std::strings to C strings.
        const std::vector<std::string> platformExtensions = m_platformPlugin->GetInstanceExtensions();
        std::transform(platformExtensions.begin(), platformExtensions.end(), std::back_inserter(extensions),
                        [](const std::string& ext) { return ext.c_str(); });
        const std::vector<std::string> graphicsExtensions = g_theRenderer->GetInstanceExtensions();
        std::transform(graphicsExtensions.begin(), graphicsExtensions.end(), std::back_inserter(extensions),
                        [](const std::string& ext) { return ext.c_str(); });

        XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        createInfo.next = m_platformPlugin->GetInstanceCreateExtension();
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.enabledExtensionNames = extensions.data();

        strncpy_s(createInfo.applicationInfo.applicationName, "Thesis", 7);

        // Current version is 1.1.x, but hello_xr only requires 1.0.x
        createInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;

		// m_openXRAvaible = false;
		// return;

        XrResult result = xrCreateInstance(&createInfo, &m_instance);
		if (XR_FAILED(result)) 
        {
			m_openXRAvaible = false;

			Log::Write(Log::Level::Error, "XR create instance failed, check if the Meta Quest Link, SteamVR are launched and your headset is connected ");
		}
    }

	XrSpace OpenXrProgram::GetRotatedSpaceForLocatingViews()
	{
        // xrDestroySpace(m_worldSpace);

        XrPosef poseInReferenceSpace = {};
		poseInReferenceSpace.position = { 0, 0, 0 };  // No position offset
		poseInReferenceSpace.orientation = GetXRQuat(Quat::CreateRotationAroundYAxis(g_theVRPlayer->m_orientation.m_yawDegrees));

		// Create a rotated space
		XrReferenceSpaceCreateInfo spaceCreateInfo = {};
		spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;  // Apply rotation in LOCAL space
		spaceCreateInfo.poseInReferenceSpace = poseInReferenceSpace;
			
		xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_rotatedSpace);
        return m_rotatedSpace;
	}

	void OpenXrProgram::CreateInstance()
    {
        LogLayersAndExtensions();

        CreateInstanceInternal();

        if (m_openXRAvaible)
        {
			LogInstanceInfo();
        }
    }

    void OpenXrProgram::LogViewConfigurations()
    {
        CHECK(m_instance != XR_NULL_HANDLE);
        CHECK(m_systemId != XR_NULL_SYSTEM_ID);

        uint32_t viewConfigTypeCount;
        CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, 0, &viewConfigTypeCount, nullptr));
        std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
        CHECK_XRCMD(xrEnumerateViewConfigurations(m_instance, m_systemId, viewConfigTypeCount, &viewConfigTypeCount,
                                                    viewConfigTypes.data()));
        CHECK((uint32_t)viewConfigTypes.size() == viewConfigTypeCount);

        Log::Write(Log::Level::Info, Fmt("Available View Configuration Types: (%d)", viewConfigTypeCount));
        for (XrViewConfigurationType viewConfigType : viewConfigTypes) {
            Log::Write(Log::Level::Verbose, Fmt("  View Configuration Type: %s %s", to_string(viewConfigType),
                                                viewConfigType == m_options->Parsed.ViewConfigType ? "(Selected)" : ""));

            XrViewConfigurationProperties viewConfigProperties{XR_TYPE_VIEW_CONFIGURATION_PROPERTIES};
            CHECK_XRCMD(xrGetViewConfigurationProperties(m_instance, m_systemId, viewConfigType, &viewConfigProperties));

            Log::Write(Log::Level::Verbose,
                        Fmt("  View configuration FovMutable=%s", viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False"));

            uint32_t viewCount;
            CHECK_XRCMD(xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, 0, &viewCount, nullptr));
            if (viewCount > 0) {
                std::vector<XrViewConfigurationView> views(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
                CHECK_XRCMD(
                    xrEnumerateViewConfigurationViews(m_instance, m_systemId, viewConfigType, viewCount, &viewCount, views.data()));

                for (uint32_t i = 0; i < views.size(); i++) {
                    const XrViewConfigurationView& view = views[i];

                    Log::Write(Log::Level::Verbose, Fmt("    View [%d]: Recommended Width=%d Height=%d SampleCount=%d", i,
                                                        view.recommendedImageRectWidth, view.recommendedImageRectHeight,
                                                        view.recommendedSwapchainSampleCount));
                    Log::Write(Log::Level::Verbose,
                                Fmt("    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d", i, view.maxImageRectWidth,
                                    view.maxImageRectHeight, view.maxSwapchainSampleCount));
                }
            } else {
                Log::Write(Log::Level::Error, Fmt("Empty view configuration type"));
            }

            LogEnvironmentBlendMode(viewConfigType);
        }
    }

    void OpenXrProgram::LogEnvironmentBlendMode(XrViewConfigurationType type)
    {
        CHECK(m_instance != XR_NULL_HANDLE);
        CHECK(m_systemId != 0);

        uint32_t count;
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, 0, &count, nullptr));
        CHECK(count > 0);

        Log::Write(Log::Level::Info, Fmt("Available Environment Blend Mode count : (%d)", count));

        std::vector<XrEnvironmentBlendMode> blendModes(count);
        CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, type, count, &count, blendModes.data()));

        bool blendModeFound = false;
        for (XrEnvironmentBlendMode mode : blendModes) {
            const bool blendModeMatch = (mode == m_options->Parsed.EnvironmentBlendMode);
            Log::Write(Log::Level::Info,
                        Fmt("Environment Blend Mode (%s) : %s", to_string(mode), blendModeMatch ? "(Selected)" : ""));
            blendModeFound |= blendModeMatch;
        }
        CHECK(blendModeFound);
    }

	void OpenXrProgram::InitializeSystem()
    {     
		if (m_openXRAvaible)
		{
            CHECK(m_instance != XR_NULL_HANDLE);
            CHECK(m_systemId == XR_NULL_SYSTEM_ID);

            XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
            // systemInfo.formFactor = m_options->Parsed.FormFactor;
            systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
            XrResult result =xrGetSystem(m_instance, &systemInfo, &m_systemId);

			if (result == XR_ERROR_FORM_FACTOR_UNAVAILABLE) 
            {
                xrDestroyInstance(m_instance);
				m_openXRAvaible = false;
				Log::Write(Log::Level::Error, "XR create instance failed, check if Meta Quest Link, SteamVR are launched and Meta Quest is connected ");

				// Log::Write(Log::Level::Warning, "Headset might not be disconnected, running windows debug mode");
			}
        }

        if (m_openXRAvaible)
        {
			Log::Write(Log::Level::Verbose,
				Fmt("Using system %d for form factor %s", m_systemId, to_string(m_options->Parsed.FormFactor)));
			CHECK(m_instance != XR_NULL_HANDLE);
			CHECK(m_systemId != XR_NULL_SYSTEM_ID);
        }
    }

    void OpenXrProgram::InitializeDevice()
    {
        LogViewConfigurations();

        // The graphics API can initialize the graphics device now that the systemId and instance
        // handle are available.
        g_theRenderer->InitializeDevice(m_instance, m_systemId);
    }

    void OpenXrProgram::LogReferenceSpaces()
    {
        CHECK(m_session != XR_NULL_HANDLE);

        uint32_t spaceCount;
        CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, 0, &spaceCount, nullptr));
        std::vector<XrReferenceSpaceType> spaces(spaceCount);
        CHECK_XRCMD(xrEnumerateReferenceSpaces(m_session, spaceCount, &spaceCount, spaces.data()));

        Log::Write(Log::Level::Info, Fmt("Available reference spaces: %d", spaceCount));
        for (XrReferenceSpaceType space : spaces) 
        {
            Log::Write(Log::Level::Info, Fmt("  Name: %s", to_string(space)));
        }

    }

    void OpenXrProgram::InitializeActions()
    {
        // Create an action set.
        {
            XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
            strncpy_s(actionSetInfo.actionSetName, "gameplay", 8);
            strncpy_s(actionSetInfo.localizedActionSetName, "Gameplay", 8);
            actionSetInfo.priority = 0;
            CHECK_XRCMD(xrCreateActionSet(m_instance, &actionSetInfo, &m_input.actionSet));
        }

        // Get the XrPath for the left and right hands - we will use them as subaction paths.
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left", &m_input.handSubactionPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right", &m_input.handSubactionPath[Side::RIGHT]));

        // Create actions.
        {
            // Create an input action for grabbing objects with the left and right hands.
            XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
            strncpy_s(actionInfo.actionName, "grab_object", 11);
            strncpy_s(actionInfo.localizedActionName, "Grab Object", 11);
            actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
            actionInfo.subactionPaths = m_input.handSubactionPath.data();
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.grabAction));

            // Create an input action getting the left and right hand poses.
            actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
            strncpy_s(actionInfo.actionName, "hand_pose", 9);
            strncpy_s(actionInfo.localizedActionName, "Hand Pose", 9);
            actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
            actionInfo.subactionPaths = m_input.handSubactionPath.data();
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.poseAction));

            // Create output actions for vibrating the left and right controller.
            actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
            strncpy_s(actionInfo.actionName, "vibrate_hand", 12);
            strncpy_s(actionInfo.localizedActionName, "Vibrate Hand", 12);
            actionInfo.countSubactionPaths = uint32_t(m_input.handSubactionPath.size());
            actionInfo.subactionPaths = m_input.handSubactionPath.data();
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.vibrateAction));

            // Create input actions for quitting the session using the left and right controller.
            // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
            // We will just suggest bindings for both hands, where possible.
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            strncpy_s(actionInfo.actionName, "quit_session", 16);
            strncpy_s(actionInfo.localizedActionName, "Quit Session", 16);
            actionInfo.countSubactionPaths = 0;
            actionInfo.subactionPaths = nullptr;
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.quitAction));

            // recenter the whole game
			actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
			strncpy_s(actionInfo.actionName, "recenter_world", 15);
			strncpy_s(actionInfo.localizedActionName, "Recenter World", 15);
			actionInfo.countSubactionPaths = 0;
			actionInfo.subactionPaths = nullptr;
			CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.recenterAction));

            // right joystick X
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
			strncpy_s(actionInfo.actionName, "right_joystick_x", 16);
			strncpy_s(actionInfo.localizedActionName, "Right Joystick X", 16);
			actionInfo.countSubactionPaths = 0;  
			actionInfo.subactionPaths = nullptr;
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.right_joystick_x_Action));                
                
            // right joystick Y
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
			strncpy_s(actionInfo.actionName, "right_joystick_y", 16);
			strncpy_s(actionInfo.localizedActionName, "Right Joystick Y", 16);
			actionInfo.countSubactionPaths = 0;  
			actionInfo.subactionPaths = nullptr;
            CHECK_XRCMD(xrCreateAction(m_input.actionSet, &actionInfo, &m_input.right_joystick_y_Action));
        }

        std::array<XrPath, Side::COUNT> selectPath;
        std::array<XrPath, Side::COUNT> squeezeValuePath;
        std::array<XrPath, Side::COUNT> squeezeForcePath;
        std::array<XrPath, Side::COUNT> squeezeClickPath;
        std::array<XrPath, Side::COUNT> posePath;
        std::array<XrPath, Side::COUNT> hapticPath;
        std::array<XrPath, Side::COUNT> menuClickPath;
        std::array<XrPath, Side::COUNT> bClickPath;
        std::array<XrPath, Side::COUNT> triggerValuePath;
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/select/click", &selectPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/select/click", &selectPath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/grip/pose", &posePath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/grip/pose", &posePath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/output/haptic", &hapticPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/output/haptic", &hapticPath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/menu/click", &menuClickPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/menu/click", &menuClickPath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/b/click", &bClickPath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &bClickPath[Side::RIGHT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/trigger/value", &triggerValuePath[Side::LEFT]));
        CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/trigger/value", &triggerValuePath[Side::RIGHT]));

		XrPath rightThumbstickXPath;
		XrResult result = CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/x", &rightThumbstickXPath));
        Log::Write(Log::Level::Info, Stringf("xrStringToPath result: %s (0x%X)", XrResultToString(result), result));

		XrPath rightThumbstickYPath;
		CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/thumbstick/y", &rightThumbstickYPath));

        XrPath YButtonClickPath;
		CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/left/input/y/click", &YButtonClickPath));
		XrPath BButtonClickPath;
		CHECK_XRCMD(xrStringToPath(m_instance, "/user/hand/right/input/b/click", &BButtonClickPath));


        // Suggest bindings for KHR Simple.
        {
            XrPath khrSimpleInteractionProfilePath;
            CHECK_XRCMD(
                xrStringToPath(m_instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{// Fall back to a click input for the grab action.
                                                            {m_input.grabAction, selectPath[Side::LEFT]},
                                                            {m_input.grabAction, selectPath[Side::RIGHT]},
                                                            {m_input.poseAction, posePath[Side::LEFT]},
                                                            {m_input.poseAction, posePath[Side::RIGHT]},
                                                            {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                            {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                            {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                            {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
        }
        // Suggest bindings for the Oculus Touch.
        {
            XrPath oculusTouchInteractionProfilePath;
            CHECK_XRCMD(
                xrStringToPath(m_instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeValuePath[Side::LEFT]},
                                                            {m_input.grabAction, squeezeValuePath[Side::RIGHT]},
                                                            {m_input.poseAction, posePath[Side::LEFT]},
                                                            {m_input.poseAction, posePath[Side::RIGHT]},
                                                            {m_input.quitAction, YButtonClickPath},
                                                            {m_input.recenterAction, BButtonClickPath},
                                                            {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                            {m_input.vibrateAction, hapticPath[Side::RIGHT]},
                                                            {m_input.right_joystick_x_Action, rightThumbstickXPath},
                                                            {m_input.right_joystick_y_Action, rightThumbstickYPath}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
        }
        // Suggest bindings for the Vive Controller.
        {
            XrPath viveControllerInteractionProfilePath;
            CHECK_XRCMD(
                xrStringToPath(m_instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, triggerValuePath[Side::LEFT]},
                                                            {m_input.grabAction, triggerValuePath[Side::RIGHT]},
                                                            {m_input.poseAction, posePath[Side::LEFT]},
                                                            {m_input.poseAction, posePath[Side::RIGHT]},
                                                            {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                            {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                            {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                            {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
        }

        // Suggest bindings for the Valve Index Controller.
        {
            XrPath indexControllerInteractionProfilePath;
            CHECK_XRCMD(
                xrStringToPath(m_instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeForcePath[Side::LEFT]},
                                                            {m_input.grabAction, squeezeForcePath[Side::RIGHT]},
                                                            {m_input.poseAction, posePath[Side::LEFT]},
                                                            {m_input.poseAction, posePath[Side::RIGHT]},
                                                            {m_input.quitAction, bClickPath[Side::LEFT]},
                                                            {m_input.quitAction, bClickPath[Side::RIGHT]},
                                                            {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                            {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
        }

        // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
        {
            XrPath microsoftMixedRealityInteractionProfilePath;
            CHECK_XRCMD(xrStringToPath(m_instance, "/interaction_profiles/microsoft/motion_controller",
                                        &microsoftMixedRealityInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{m_input.grabAction, squeezeClickPath[Side::LEFT]},
                                                            {m_input.grabAction, squeezeClickPath[Side::RIGHT]},
                                                            {m_input.poseAction, posePath[Side::LEFT]},
                                                            {m_input.poseAction, posePath[Side::RIGHT]},
                                                            {m_input.quitAction, menuClickPath[Side::LEFT]},
                                                            {m_input.quitAction, menuClickPath[Side::RIGHT]},
                                                            {m_input.vibrateAction, hapticPath[Side::LEFT]},
                                                            {m_input.vibrateAction, hapticPath[Side::RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            CHECK_XRCMD(xrSuggestInteractionProfileBindings(m_instance, &suggestedBindings));
        }
        XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        actionSpaceInfo.action = m_input.poseAction;
        actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
        actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::LEFT];
        CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::LEFT]));
        actionSpaceInfo.subactionPath = m_input.handSubactionPath[Side::RIGHT];
        CHECK_XRCMD(xrCreateActionSpace(m_session, &actionSpaceInfo, &m_input.handSpace[Side::RIGHT]));

        XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &m_input.actionSet;
        CHECK_XRCMD(xrAttachSessionActionSets(m_session, &attachInfo));
    }

	void OpenXrProgram::UpdatePlayerCurrentHeadPose()
	{
        Quat q = Quat(g_theRenderer->m_views[0].pose.orientation);
		q.x = 0.f;
		q.z = 0.f;
		q = q.GetNormalized();

        Vec3 pos = (Vec3(g_theRenderer->m_views[0].pose.position) + Vec3(g_theRenderer->m_views[1].pose.position)) * 0.5f;

        m_resetGameWorldPose.position = pos + Vec3(0.f, -1.8f, 0.f); // we assume player's eye is 1.5 meter tall, openXR space
        m_resetGameWorldPose.orientation = q;                        // openXR space
        m_eyesCenterPosInGameWorld = Mat44(m_resetGameWorldPose.position).MatMultiply(Mat44(m_resetGameWorldPose.orientation));

        Mat44 transformMat = GetOpenXRToGameMat();
        m_eyesCenterPosInGameWorld = transformMat.SimilarityTransformation(m_eyesCenterPosInGameWorld);
        g_theVRPlayer->UpdateHeadPoseTracking(m_eyesCenterPosInGameWorld);
	}

	void OpenXrProgram::CreateVisualizedSpaces()
    {
        CHECK(m_session != XR_NULL_HANDLE);

		// Fill out an XrReferenceSpaceCreateInfo structure and create a reference XrSpace
        // specifying a Local space with an identity pose as the origin.
		// XrReferenceSpaceCreateInfo referenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		// referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		// referenceSpaceCI.poseInReferenceSpace = Math::Pose::RotateCCWAboutZAxis(Vec3(0.f, -1.f, 0.f).MakeXrVector3f(), 0.f); // notice this is OpenXR space, -1 in y means -1 in z in our world
		// // referenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
		// XrSpace space;
		// XrResult res = xrCreateReferenceSpace(m_session, &referenceSpaceCI, &space);
		// if (XR_SUCCEEDED(res))
		// {
		//     m_worldSpace = space;
		// }
		// else
		// {
		// 	Log::Write(Log::Level::Error, "failed to create reference space");
		// }

		XrReferenceSpaceCreateInfo viewSpaceReferenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		viewSpaceReferenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		viewSpaceReferenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
		XrSpace viewSpace;
		XrResult res = xrCreateReferenceSpace(m_session, &viewSpaceReferenceSpaceCI, &viewSpace);
		if (XR_SUCCEEDED(res))
		{
			m_viewSpace = viewSpace;
		}
		else
		{
			Log::Write(Log::Level::Error, "failed to create reference space");
		}

		// xrDestroySpace(m_viewSpace);

		// // create new view space
		// XrReferenceSpaceCreateInfo viewSpaceReferenceSpaceCI{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		// viewSpaceReferenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		// viewSpaceReferenceSpaceCI.poseInReferenceSpace = Math::Pose::Identity();
		// XrSpace viewSpace;
		// res = xrCreateReferenceSpace(m_session, &viewSpaceReferenceSpaceCI, &viewSpace);
		// if (XR_SUCCEEDED(res))
		// {
		// 	m_viewSpace = viewSpace;
		// }
		// else
		// {
		// 	Log::Write(Log::Level::Error, "failed to create reference space");
		// }
    }

    void OpenXrProgram::InitializeSession()
    {
        CHECK(m_instance != XR_NULL_HANDLE);
        CHECK(m_session == XR_NULL_HANDLE);

        {
            Log::Write(Log::Level::Verbose, Fmt("Creating session..."));

            XrSessionCreateInfo createInfo{XR_TYPE_SESSION_CREATE_INFO};
            createInfo.next = g_theRenderer->GetGraphicsBinding();
            createInfo.systemId = m_systemId;

			XrResult result = xrCreateSession(m_instance, &createInfo, &m_session);
			if (XR_FAILED(result))
			{
				m_openXRAvaible = false;

				Log::Write(Log::Level::Error, "XR create session failed, check if the Meta Quest Link, SteamVR are launched and your headset is connected ");
                return;
			}
        }

        LogReferenceSpaces();
        InitializeActions();
        CreateVisualizedSpaces();

        { 
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(m_options->AppSpace);
            CHECK_XRCMD(xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &m_appSpace));
        }
    }

    void OpenXrProgram::Startup()
    {
		// Create engine subsystems and game
		EventSystemConfig eventConfig;
		g_theEventSystem = new EventSystem(eventConfig);

		InputConfig inputConfig;
		g_theInput = new InputSystem(inputConfig);
        g_theInput->SetDesiredCursorMode(false, false);

		WindowConfig windowConfig;
		windowConfig.m_inputSystem = g_theInput;
		windowConfig.m_windowTitle = "Procedural Animation & Physics Animation";


		m_windowAspectRatio = 2.f;
#ifdef SHIPPING
		m_windowAspectRatio = 1744.f / 1920.f;
#endif
		windowConfig.m_aspectRatio = m_windowAspectRatio;

		g_theWindow = new Window(windowConfig);

		RenderConfig renderConfig;
		renderConfig.m_window = g_theWindow;
        renderConfig.m_drawPlanarShadow = true;
		g_theRenderer = new Renderer(renderConfig);

        if (m_openXRAvaible)
        {
			InitializeDevice();         // todo: if the initialization failed, skip the VR rendering and tracking stuff, we are doing windows debuging
			InitializeSession();
        }

		AudioConfig audioConfig;
		g_theAudio = new AudioSystem(audioConfig);
		g_theAudio->Startup();

		// set up development console
		DevConsoleConfig consoleConfig;
		consoleConfig.m_font = g_consoleFont;
		consoleConfig.m_renderer = g_theRenderer;
		consoleConfig.m_camera = nullptr;
		g_theDevConsole = new DevConsole(consoleConfig);

		g_theWindow->Startup();
		g_theEventSystem->Startup();

        if (m_openXRAvaible)
        {
			g_theRenderer->CreateSwapchains(m_session, m_instance, m_systemId, m_options);
        }
        else
        {
            g_theRenderer->CreateDeviceAndSwapChain();
        }
		g_theRenderer->Startup();


		g_theDevConsole->Startup();
		g_theInput->Startup();
		g_theAudio->Startup();

		g_consoleFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/RobotoMonoSemiBold128.png");
		g_theDevConsole->m_config.m_renderer = g_theRenderer;
		g_theDevConsole->m_config.m_font = g_consoleFont;

		DebugRenderConfig debugRenderConfig;
		debugRenderConfig.m_renderer = g_theRenderer;
		debugRenderConfig.m_font = g_consoleFont;
		DebugRenderSystemStartup(debugRenderConfig);

		// m_attractModeCamera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
		g_theDevConsole->AddInstruction("Type help for a list of commands");
		g_theDevConsole->AddInstruction("Controls", DevConsole::INFO_MINOR);
		g_theDevConsole->AddInstruction("Mouse  - Aim");
		g_theDevConsole->AddInstruction("W / A  - Move");
		g_theDevConsole->AddInstruction("S / D  - Strafe");
		g_theDevConsole->AddInstruction("Q / E  - Roll");
		g_theDevConsole->AddInstruction("Z / C  - Elevate");
		g_theDevConsole->AddInstruction("Shift  - Sprint");
		g_theDevConsole->AddInstruction("~      - Open Dev Console");
		g_theDevConsole->AddInstruction("Escape - Exit Game");
		g_theDevConsole->AddInstruction("Space  - Start Game");

		// set up event system subscription
		SubscribeEventCallbackFunction("quit", OpenXrProgram::Event_Quit);
		// show helper commands at the start when the console is turned on
		FireEvent("ControlInstructions");

        LoadGameShaders();

        // start up the physics system
        PhysXSystemConfig* physXConfig = new PhysXSystemConfig();
        g_thePhysX = new ThePhysX(*physXConfig);
        g_thePhysX->Startup();

		g_theGame = new Game();
		g_theGame->Startup();

        //----------------------------------------------------------------------------------------------------------------------------------------------------

        #ifdef SHIPPING 
			m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(1744.f, 1920.f));
        // #else
		// m_screenCamera.SetOrthoView(Vec2::ZERO, Vec2(200.f, 100.f));
        #endif
    }

	void OpenXrProgram::RunFrame()
	{
		BeginFrame();
        Update();
            
		if (m_openXRAvaible)
		{
			RenderVR();
		}

#ifndef SHIPPING
		g_theRenderer->BeginFrame();
        g_theWinPlayer->Render();
		g_theRenderer->EndFrame();
#endif

        EndFrame();
	}

    void OpenXrProgram::BeginFrame()
    {
		g_theEventSystem->BeginFrame();
		g_theInput->BeginFrame();
		g_theDevConsole->BeginFrame();
		g_theWindow->BeginFrame();
		g_theAudio->BeginFrame();

		DebugRenderBeginFrame();
    }

	void OpenXrProgram::EndFrame()
	{
		g_theEventSystem->EndFrame();
		g_theInput->EndFrame();
		g_theDevConsole->EndFrame();
		g_theWindow->EndFrame();
		g_theAudio->EndFrame();

		DebugRenderEndFrame();
	}

	void OpenXrProgram::Update()
    {
		Clock::GetSystemClock().TickSystemClock();

        if (m_openXRAvaible)
        {
            PollActions();
        }
        g_thePhysX->Update();
        g_theGame->Update();

		if (g_theInput->WasKeyJustPressed(KEYCODE_ESC))
		{
			FireEvent("quit");
		}
    }

	void OpenXrProgram::Shutdown()
	{
		// shut down game and engine subsystem
		g_theGame->Shutdown();
		g_theAudio->Shutdown();
		g_theWindow->ShutDown();
		g_theInput->Shutdown();
		g_theDevConsole->Shutdown();
		g_theEventSystem->Shutdown();
		DebugRenderSystemShutDown();
		g_theRenderer->Shutdown();

		delete g_theAudio;
		g_theAudio = nullptr;

		delete g_theDevConsole;
		g_theDevConsole = nullptr;

		delete g_theRenderer;
		g_theRenderer = nullptr;

		delete g_theWindow;
		g_theWindow = nullptr;

		delete g_theInput;
		g_theInput = nullptr;

		delete g_theEventSystem;
		g_theEventSystem = nullptr;

	}

	// Return event if one is available, otherwise return null.
    const XrEventDataBaseHeader* OpenXrProgram::TryReadNextEvent()
    {
        // It is sufficient to clear the just the XrEventDataBuffer header to
        // XR_TYPE_EVENT_DATA_BUFFER
        XrEventDataBaseHeader* baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&m_eventDataBuffer);
        *baseHeader = {XR_TYPE_EVENT_DATA_BUFFER};
        const XrResult xr = xrPollEvent(m_instance, &m_eventDataBuffer);
        if (xr == XR_SUCCESS) {
            if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
                const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
                Log::Write(Log::Level::Warning, Fmt("%d events lost", eventsLost->lostEventCount));
            }

            return baseHeader;
        }
        if (xr == XR_EVENT_UNAVAILABLE) // when take off the headset, this happens
        {
            return nullptr;
        }
        THROW_XR(xr, "xrPollEvent");
    }

    void OpenXrProgram::PollEvents(bool* exitRenderLoop, bool* requestRestart)
    {
        *exitRenderLoop = *requestRestart = false;

        // Process all pending messages.
        while (const XrEventDataBaseHeader* event = TryReadNextEvent()) 
        {
            switch (event->type) {
                case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                    const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
                    Log::Write(Log::Level::Warning, Fmt("XrEventDataInstanceLossPending by %lld", instanceLossPending.lossTime));
                    *exitRenderLoop = true;
                    *requestRestart = true;
                    return;
                }
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: 
                {
					auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
					HandleSessionStateChangedEvent(sessionStateChangedEvent, exitRenderLoop, requestRestart);
					break;
				}
                case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                    LogActionSourceName(m_input.grabAction, "Grab");
                    LogActionSourceName(m_input.quitAction, "Quit");
                    LogActionSourceName(m_input.poseAction, "Pose");
                    LogActionSourceName(m_input.vibrateAction, "Vibrate");
                    break;
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                default: {
                    Log::Write(Log::Level::Verbose, Fmt("Ignoring event type %d", event->type));
                    break;
                }
            }
        }
    }

    void OpenXrProgram::HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged& stateChangedEvent, bool* exitRenderLoop, bool* requestRestart)
    {
        const XrSessionState oldState = m_sessionState;
        m_sessionState = stateChangedEvent.state;

        Log::Write(Log::Level::Info, Fmt("XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld", to_string(oldState),
                                            to_string(m_sessionState), stateChangedEvent.session, stateChangedEvent.time));

        if ((stateChangedEvent.session != XR_NULL_HANDLE) && (stateChangedEvent.session != m_session)) {
            Log::Write(Log::Level::Error, "XrEventDataSessionStateChanged for unknown session");
            return;
        }

        switch (m_sessionState) 
        {
			case XR_SESSION_STATE_IDLE: 
            {
				Log::Write(Log::Level::Info, "Session is idle (likely headset removed).");
				// Optional: Keep the application running, but handle any low-power/idle actions here
				// m_openXRAvaible = false;
				break;
			}
            case XR_SESSION_STATE_READY: {
                CHECK(m_session != XR_NULL_HANDLE);
                XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                sessionBeginInfo.primaryViewConfigurationType = m_options->Parsed.ViewConfigType;
                CHECK_XRCMD(xrBeginSession(m_session, &sessionBeginInfo));
                m_sessionRunning = true;
                break;
            }
            case XR_SESSION_STATE_STOPPING: {
                CHECK(m_session != XR_NULL_HANDLE);
                m_sessionRunning = false;
                CHECK_XRCMD(xrEndSession(m_session))
                break;
            }
            case XR_SESSION_STATE_EXITING: {
                *exitRenderLoop = true;
                // Do not attempt to restart because user closed this session.
                *requestRestart = false;
                break;
            }
            case XR_SESSION_STATE_LOSS_PENDING: {
                *exitRenderLoop = true;
                // Poll for a new instance.
                *requestRestart = true;
                break;
            }
            default:
                break;
        }
    }

    void OpenXrProgram::LogActionSourceName(XrAction action, const std::string& actionName) const {
        XrBoundSourcesForActionEnumerateInfo getInfo = {XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
        getInfo.action = action;
        uint32_t pathCount = 0;
        CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, 0, &pathCount, nullptr));
        std::vector<XrPath> paths(pathCount);
        CHECK_XRCMD(xrEnumerateBoundSourcesForAction(m_session, &getInfo, uint32_t(paths.size()), &pathCount, paths.data()));

        std::string sourceName;
        for (uint32_t i = 0; i < pathCount; ++i) {
            constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                                                            XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                                            XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

            XrInputSourceLocalizedNameGetInfo nameInfo = {XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
            nameInfo.sourcePath = paths[i];
            nameInfo.whichComponents = all;

            uint32_t size = 0;
            CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, 0, &size, nullptr));
            if (size < 1) {
                continue;
            }
            std::vector<char> grabSource(size);
            CHECK_XRCMD(xrGetInputSourceLocalizedName(m_session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data()));
            if (!sourceName.empty()) {
                sourceName += " and ";
            }
            sourceName += "'";
            sourceName += std::string(grabSource.data(), size - 1);
            sourceName += "'";
        }

        Log::Write(Log::Level::Info,
                    Fmt("%s action is bound to %s", actionName.c_str(), ((!sourceName.empty()) ? sourceName.c_str() : "nothing")));
    }

	bool OpenXrProgram::IsSessionRunning() const { return m_sessionRunning; }

	bool OpenXrProgram::IsSessionFocused() const { return m_sessionState == XR_SESSION_STATE_FOCUSED; }

	void OpenXrProgram::PollActions()
    {
        m_input.handActive = {XR_FALSE, XR_FALSE};

        // Sync actions
        const XrActiveActionSet activeActionSet{m_input.actionSet, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        CHECK_XRCMD(xrSyncActions(m_session, &syncInfo));

        // Get pose and grab action state and start haptic vibrate when hand is 90% squeezed.
        for (auto hand : {Side::LEFT, Side::RIGHT}) 
        {
            XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
            getInfo.action = m_input.grabAction;
            getInfo.subactionPath = m_input.handSubactionPath[hand];

            XrActionStateFloat grabValue{XR_TYPE_ACTION_STATE_FLOAT};
            CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &grabValue));
            if (grabValue.isActive == XR_TRUE) 
            {
                // update the grab value for both hands
                g_theVRPlayer->m_hands[hand].m_grabValue = grabValue.currentState;

                if (grabValue.currentState > 0.5f) {
                    XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
                    vibration.amplitude = 0.05f;
                    vibration.duration = XR_MIN_HAPTIC_DURATION;
                    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

                    XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
                    hapticActionInfo.action = m_input.vibrateAction;
                    hapticActionInfo.subactionPath = m_input.handSubactionPath[hand];
                    CHECK_XRCMD(xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader*)&vibration));
                }
            }

            // update the pose status for both hands, if they are active or not
            getInfo.action = m_input.poseAction;
            XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
            CHECK_XRCMD(xrGetActionStatePose(m_session, &getInfo, &poseState));
            m_input.handActive[hand] = poseState.isActive;
        }

        // There were no subaction paths specified for the quit action, because we don't care which hand did it.
        XrActionStateGetInfo quitActionInfo{XR_TYPE_ACTION_STATE_GET_INFO, nullptr, m_input.quitAction, XR_NULL_PATH};
        XrActionStateBoolean quitValue{XR_TYPE_ACTION_STATE_BOOLEAN};
        CHECK_XRCMD(xrGetActionStateBoolean(m_session, &quitActionInfo, &quitValue));
        if ((quitValue.isActive == XR_TRUE) && (quitValue.changedSinceLastSync == XR_TRUE) && (quitValue.currentState == XR_TRUE)) 
        {
            CHECK_XRCMD(xrRequestExitSession(m_session));
        }

        //----------------------------------------------------------------------------------------------------------------------------------------------------
		// Get right hand joystick value
		for (auto hand : { Side::RIGHT }) // Side::LEFT, we are not using the left hand yet
		{
			XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
			getInfo.action = m_input.right_joystick_x_Action;
			// getInfo.subactionPath = m_input.handSubactionPath[hand]; // we don't have sub action path when we start up

			XrActionStateFloat rightJoystick_X_State{ XR_TYPE_ACTION_STATE_FLOAT };
			XrActionStateFloat rightJoystick_Y_State{ XR_TYPE_ACTION_STATE_FLOAT };

			// Get X value
            CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &rightJoystick_X_State));
            // XrResult result = CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &rightJoystick_X_State));
            // Log::Write(Log::Level::Info, Stringf("right joystick result: %s", XrResultToString(result)));
			float joystickX = rightJoystick_X_State.currentState;
			g_theVRPlayer->m_hands[hand].m_joyStickPos.x = joystickX;
                 
			// Get Y value
			getInfo.action = m_input.right_joystick_y_Action;
            CHECK_XRCMD(xrGetActionStateFloat(m_session, &getInfo, &rightJoystick_Y_State));
			float joystickY = rightJoystick_Y_State.currentState;
			g_theVRPlayer->m_hands[hand].m_joyStickPos.y = joystickY;
		}

        //----------------------------------------------------------------------------------------------------------------------------------------------------
		// reset the app space to reset world center
		XrActionStateGetInfo resetActionInfo{ XR_TYPE_ACTION_STATE_GET_INFO, nullptr, m_input.recenterAction, XR_NULL_PATH };
		XrActionStateBoolean recenterValue{ XR_TYPE_ACTION_STATE_BOOLEAN };
		CHECK_XRCMD(xrGetActionStateBoolean(m_session, &resetActionInfo, &recenterValue));
		UpdatePlayerCurrentHeadPose();
		if ((recenterValue.isActive == XR_TRUE) && (recenterValue.changedSinceLastSync == XR_TRUE) && (recenterValue.currentState == XR_TRUE))
		{


            // g_theGame->ResetGameWorldByHeadPose(m_resetToNewGameWorldMat);

            // this works only once
            // maybe this is because the machine has a default world center
            // but each time we pass in a new pose relative to the last time
            // try record the first time value and every time append on it
                
            // m_lastTimeResetValue = m_resetGameWorldPose;
            // if (m_lastTimeResetValue.position != Vec3() && m_lastTimeResetValue.orientation != Quat())
            // {
            //     m_resetGameWorldPose.position += m_lastTimeResetValue.position;
            // 
            //     Quat inversedLastOrientation = m_lastTimeResetValue.orientation.GetInversed();
            //     m_resetGameWorldPose.orientation = inversedLastOrientation * m_resetGameWorldPose.orientation;
            //     m_resetGameWorldPose.orientation = m_resetGameWorldPose.orientation.GetNormalized();
            // }

            // RecenterPlayer();
            g_theVRPlayer->m_snapTurnMode = !g_theVRPlayer->m_snapTurnMode;
		}
    }


	void OpenXrProgram::RecenterPlayer()
	{
		xrDestroySpace(m_appSpace);
		// Define the pose for the reference space
		XrPosef resetPose;
		resetPose.position = GetXRVec3f(m_resetGameWorldPose.position);
		resetPose.orientation = GetXRQuat(m_resetGameWorldPose.orientation);

		// Create the reference space creation info
		XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = {};
		referenceSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; // e.g., XR_REFERENCE_SPACE_TYPE_LOCAL
		referenceSpaceCreateInfo.poseInReferenceSpace = resetPose; // Custom pose with your position/rotation

		// Create the reference space
		XrResult result = xrCreateReferenceSpace(m_session, &referenceSpaceCreateInfo, &m_appSpace);
		if (XR_FAILED(result))
		{
			// Handle error (e.g., log or throw exception)
			std::cerr << "Failed to create reference space: " << result << std::endl;
		}
	}

    void OpenXrProgram::Render()
    {
        if (m_openXRAvaible)
        {
            RenderVR();
        }
    }

	void OpenXrProgram::RenderVR()
	{
		CHECK(m_session != XR_NULL_HANDLE);

		XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
		XrFrameState frameState{ XR_TYPE_FRAME_STATE };
		CHECK_XRCMD(xrWaitFrame(m_session, &frameWaitInfo, &frameState));

		XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
		CHECK_XRCMD(xrBeginFrame(m_session, &frameBeginInfo));


		std::vector<XrCompositionLayerBaseHeader*> layers;
		XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
		if (frameState.shouldRender == XR_TRUE)
		{
			if (RenderLayer(frameState.predictedDisplayTime, projectionLayerViews, layer))
			{
				layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer));
			}
		}

		XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO };
		frameEndInfo.displayTime = frameState.predictedDisplayTime;
		frameEndInfo.environmentBlendMode = m_options->Parsed.EnvironmentBlendMode;
		frameEndInfo.layerCount = (uint32_t)layers.size();
		frameEndInfo.layers = layers.data();
		CHECK_XRCMD(xrEndFrame(m_session, &frameEndInfo));
	}

	bool OpenXrProgram::RenderLayer(XrTime predictedDisplayTime, std::vector<XrCompositionLayerProjectionView>& projectionLayerViews,
                        XrCompositionLayerProjection& layer) 
    {
        XrResult res;

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        uint32_t viewCapacityInput = (uint32_t)(g_theRenderer->m_views.size());
        uint32_t viewCountOutput;

        XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = m_options->Parsed.ViewConfigType;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = m_appSpace;
        // viewLocateInfo.space = GetRotatedSpaceForLocatingViews();

        res = xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, g_theRenderer->m_views.data());
        CHECK_XRRESULT(res, "xrLocateViews");
        if ((viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
            (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) 
        {
            return false;  // There is no valid tracking poses for the views.
        }

        CHECK(viewCountOutput == viewCapacityInput);
        CHECK(viewCountOutput == g_theRenderer->m_configViews.size());
        CHECK(viewCountOutput == g_theRenderer->m_XRSwapchains.size());

        projectionLayerViews.resize(viewCountOutput);

        if (res == XR_SUCCESS)
        {
			// // Get the original pose from the first view
			// XrPosef eyePose1 = g_theXRRenderer->m_views[0].pose;
			// XrPosef eyePose2 = g_theXRRenderer->m_views[1].pose;
			// 
			// XrQuaternionf viewQuaternion = (g_theXRRenderer->m_views[0].pose.orientation);
			// Vec3 pos = Vec3(g_theXRRenderer->m_views[0].pose.position);
			// Mat44 mat44;
			// XrMatrix4x4f mat = mat44.GetXrMatByMat();
			// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
			// 
			// Log::Write(Log::Level::Info, Stringf("eye0: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
			// Log::Write(Log::Level::Info, Stringf("eye0: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
			// Log::Write(Log::Level::Info, Stringf("eye0: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
			// Log::Write(Log::Level::Info, Stringf("eye0: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
			// Log::Write(Log::Level::Info, Stringf("eye0_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));
			// 
			// viewQuaternion = (g_theXRRenderer->m_views[1].pose.orientation);
			// pos = Vec3(g_theXRRenderer->m_views[1].pose.position);
			// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
			// 
			// Log::Write(Log::Level::Info, Stringf("eye1: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
			// Log::Write(Log::Level::Info, Stringf("eye1: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
			// Log::Write(Log::Level::Info, Stringf("eye1: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
			// Log::Write(Log::Level::Info, Stringf("eye1: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
			// Log::Write(Log::Level::Info, Stringf("eye1_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));


        }
        // AdjustStageSpaceAccordingToPlayerPos(predictedDisplayTime);

        // get the location information for the world space I created
		// XrSpaceLocation localSpaceLocation{ XR_TYPE_SPACE_LOCATION };
		// res = xrLocateSpace(m_worldSpace, m_appSpace, predictedDisplayTime, &localSpaceLocation); 
		// CHECK_XRRESULT(res, "xrLocateSpace");
		// if (XR_UNQUALIFIED_SUCCESS(res))
		// {
        //     // for the ground, render a 2m x 2m cube.
		// 	if ((localSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
		// 		(localSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
		// 	{
        //         // m_resetGameWorldPose = localSpaceLocation.pose;
		// 	}          
		// }
		// else
		// {
		// 	Log::Write(Log::Level::Verbose, Fmt("Unable to locate a visualized reference space in app space: %d", res));
		// }

        //----------------------------------------------------------------------------------------------------------------------------------------------------
        // locate view space
		XrSpaceLocation viewSpaceLocation{ XR_TYPE_SPACE_LOCATION };
		res = xrLocateSpace(m_viewSpace, m_appSpace, predictedDisplayTime, &viewSpaceLocation);
		CHECK_XRRESULT(res, "xrLocateSpace");
		if (XR_UNQUALIFIED_SUCCESS(res))
		{
			// for the ground, render a 2m x 2m cube.
			if ((viewSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
				(viewSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
			{
                g_theHUD->UpdateHUDViewSpacePose(viewSpaceLocation.pose);
				// XrQuaternionf viewQuaternion = (viewSpaceLocation.pose.orientation);
				// Vec3 pos = Vec3(viewSpaceLocation.pose.position);
				// Mat44 mat44;
				// XrMatrix4x4f mat = mat44.GetXrMatByMat();
				// XrMatrix4x4f_CreateFromQuaternion(&mat, &viewQuaternion);
				// 
				// Log::Write(Log::Level::Info, Stringf("viewSpace: m00 = %.02f, m01 = %.02f, m02 = %.02f, m03 = %.02f", mat.m[0], mat.m[1], mat.m[2], mat.m[3]));
				// Log::Write(Log::Level::Info, Stringf("viewSpace: m04 = %.02f, m05 = %.02f, m06 = %.02f, m07 = %.02f", mat.m[4], mat.m[5], mat.m[6], mat.m[7]));
				// Log::Write(Log::Level::Info, Stringf("viewSpace: m08 = %.02f, m09 = %.02f, m10 = %.02f, m11 = %.02f", mat.m[8], mat.m[9], mat.m[10], mat.m[11]));
				// Log::Write(Log::Level::Info, Stringf("viewSpace: m12 = %.02f, m13 = %.02f, m14 = %.02f, m15 = %.02f", mat.m[12], mat.m[13], mat.m[14], mat.m[15]));
				// Log::Write(Log::Level::Info, Stringf("viewSpace_Pos: x = %.02f, y = %.02f, z = %.02f", pos.x, pos.y, pos.z));
			}
		}
		else
		{
			Log::Write(Log::Level::Verbose, Fmt("Unable to locate a visualized reference space in app space: %d", res));
		}


        // Update the space locate information for the player hand class. Only render the hand when it is active
        // true when the application has focus.
        for (auto hand : {Side::LEFT, Side::RIGHT}) 
        {
            XrSpaceLocation handSpaceLocation{XR_TYPE_SPACE_LOCATION};
            res = xrLocateSpace(m_input.handSpace[hand], m_appSpace, predictedDisplayTime, &handSpaceLocation);
            CHECK_XRRESULT(res, "xrLocateSpace");
            if (XR_UNQUALIFIED_SUCCESS(res)) 
            {
                if ((handSpaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (handSpaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) 
                {
					g_theVRPlayer->m_hands[hand].m_isActive = true;

                    g_theVRPlayer->UpdateHandPoseTracking(handSpaceLocation.pose, hand);
                }
            } 
            else 
            {
                // Tracking loss is expected when the hand is not active so only log a message
                // update the information for the hand class
				if (m_input.handActive[hand] == XR_TRUE) 
                {
                    g_theVRPlayer->m_hands[hand].m_isActive = false;

					const char* handName[] = { "left", "right" };
					Log::Write(Log::Level::Verbose, Fmt("Unable to locate %s hand action space in app space: %d", handName[hand], res));
				}
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------------------------------
        // Render view to the appropriate part of the swapchain image.
        for (uint32_t i = 0; i < viewCountOutput; i++) 
        {
            // Each view has a separate swapchain which is acquired, rendered to, and released.
                
            const Swapchain viewSwapchain = g_theRenderer->m_XRSwapchains[i];

            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};

            uint32_t swapchainImageIndex;
            CHECK_XRCMD(xrAcquireSwapchainImage(viewSwapchain.handle, &acquireInfo, &swapchainImageIndex));

            XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            CHECK_XRCMD(xrWaitSwapchainImage(viewSwapchain.handle, &waitInfo));

            projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            projectionLayerViews[i].pose = g_theRenderer->m_views[i].pose; 
            projectionLayerViews[i].fov = g_theRenderer->m_views[i].fov;
            projectionLayerViews[i].subImage.swapchain = viewSwapchain.handle;
            projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
            projectionLayerViews[i].subImage.imageRect.extent = {viewSwapchain.width, viewSwapchain.height};

            const XrSwapchainImageBaseHeader* const swapchainImage = g_theRenderer->m_swapchainImages[viewSwapchain.handle][swapchainImageIndex];
            g_theRenderer->BeginOpenXRCamera(projectionLayerViews[i], swapchainImage, g_theRenderer->m_colorSwapchainFormat, i);

            g_thePhysX->Render();

            Vec3 eyePos = g_theRenderer->m_VRCameraMatrix[i].GetTranslation3D();
			g_theGame->m_phongLighinting->WorldEyePosition = eyePos;
			g_theGame->UpdateAllCrawlersHealthBarTrackingMat(g_theRenderer->m_VRCameraMatrix[i]);

            g_theGame->UpdateFogShaderDataWithNewCameraPos(eyePos);
			g_theGame->Render();
            DebugRenderWorldForVR();

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            CHECK_XRCMD(xrReleaseSwapchainImage(viewSwapchain.handle, &releaseInfo));


			#ifdef SHIPPING // in shipping mode, we will just render a copied image
				if (i == 0)
				{
					g_theInput->SetDesiredCursorMode(false, false);
			        
					// after the image is done rendering, copy the texture image to the windows swap chain
                    if (g_theGame->m_currentState == GameState::ATTRACT)
                    {
						g_theRenderer->ClearScreen(Rgba8::BLACK);
                    }
					else if (g_theGame->m_currentState == GameState::LOBBY)
					{
						g_theRenderer->ClearScreen(Rgba8::BLUE_LIGHT);
					}
					g_theRenderer->BeginCamera(m_screenCamera);
					g_theRenderer->RenderXRSwapchainImageToWindow(swapchainImage, &m_screenCamera);
					g_theRenderer->EndFrame();
				}
			#endif

        }

        layer.space = m_appSpace;
        layer.layerFlags =
            m_options->Parsed.EnvironmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND
                ? XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT
                : 0;
        layer.viewCount = (uint32_t)projectionLayerViews.size();
        layer.views = projectionLayerViews.data();

        return true;
    }

	XrEnvironmentBlendMode OpenXrProgram::GetPreferredBlendMode()
	{
		uint32_t count;
		CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_options->Parsed.ViewConfigType, 0, &count, nullptr));
		CHECK(count > 0);

		std::vector<XrEnvironmentBlendMode> blendModes(count);
		CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_options->Parsed.ViewConfigType, count, &count,
			blendModes.data()));
		for (const auto& blendMode : blendModes) {
			if (m_acceptableBlendModes.count(blendMode)) return blendMode;
		}
		THROW("No acceptable blend mode returned from the xrEnumerateEnvironmentBlendModes");
	}

	XrQuaternionf OpenXrProgram::createQuaternionFromAxisAngle(float x, float y, float z)
	{
		float halfAngle = 3.1415926f * 0.5f;
		float sinHalfAngle = sin(halfAngle);
        XrQuaternionf q;
		q.x = x * sinHalfAngle;
		q.y = y * sinHalfAngle;
		q.z = z * sinHalfAngle;
		q.w = cos(halfAngle);
		return q;
	}

	bool OpenXrProgram::HandleQuitRequested()
	{
		m_isQuitting = true;
		return true;
	}

	bool OpenXrProgram::Event_Quit(EventArgs& args)
	{
		UNUSED(args);
		g_theApp->HandleQuitRequested();
		return true;
	}

	void OpenXrProgram::LoadMaterialAssets()
	{
		g_materials[MaterialType::CRAWLER] = new Material();
		g_materials[MaterialType::CRAWLER]->Load("Data/Materials/M_Crawler.xml");
	}

	void OpenXrProgram::LoadGameShaders()
	{
		m_shaders[WORLD] = g_theRenderer->CreateOrGetShader("Data/Shader/World", VertexType::Vertex_PCU);
	}
