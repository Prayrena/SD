// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********
//     See validation_layer_generator.py for modifications
// ************************************************************

// Copyright (c) 2017-2024, The Khronos Group Inc.
// Copyright (c) 2017-2019 Valve Corporation
// Copyright (c) 2017-2019 LunarG, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Mark Young <marky@lunarg.com>
//

#pragma once
#include "xr_generated_dispatch_table.h"
#include "validation_utils.h"
#include "api_layer_platform_defines.h"
#include "xr_dependencies.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>


// Unordered Map associating pointer to a vector of session label information to a session's handle
extern std::unordered_map<XrSession, std::vector<GenValidUsageXrInternalSessionLabel*>*> g_xr_session_labels;


// ---- Core 1.0 commands
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageXrGetInstanceProcAddr(
    XrInstance instance,
    const char* name,
    PFN_xrVoidFunction* function);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance);
XrResult GenValidUsageInputsXrCreateInstance(const XrInstanceCreateInfo* createInfo, XrInstance* instance);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrCreateInstance(
    const XrInstanceCreateInfo* createInfo,
    XrInstance* instance);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrDestroyInstance(
    XrInstance instance);
XrResult GenValidUsageInputsXrDestroyInstance(XrInstance instance);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrDestroyInstance(
    XrInstance instance);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session);
XrResult GenValidUsageInputsXrCreateSession(XrInstance instance, const XrSessionCreateInfo* createInfo, XrSession* session);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrCreateSession(
    XrInstance instance,
    const XrSessionCreateInfo* createInfo,
    XrSession* session);

// ---- XR_LOADER_VERSION_1_0 extension commands

// ---- Core 1.1 commands

// ---- XR_KHR_android_thread_settings extension commands

// ---- XR_KHR_android_surface_swapchain extension commands

// ---- XR_KHR_opengl_enable extension commands

// ---- XR_KHR_opengl_es_enable extension commands

// ---- XR_KHR_vulkan_enable extension commands

// ---- XR_KHR_D3D11_enable extension commands

// ---- XR_KHR_D3D12_enable extension commands

// ---- XR_KHR_visibility_mask extension commands

// ---- XR_KHR_win32_convert_performance_counter_time extension commands

// ---- XR_KHR_convert_timespec_time extension commands

// ---- XR_KHR_loader_init extension commands

// ---- XR_KHR_vulkan_enable2 extension commands

// ---- XR_KHR_locate_spaces extension commands

// ---- XR_EXT_performance_settings extension commands

// ---- XR_EXT_thermal_query extension commands

// ---- XR_EXT_debug_utils extension commands
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrSetDebugUtilsObjectNameEXT(
    XrInstance instance,
    const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XrResult GenValidUsageInputsXrSetDebugUtilsObjectNameEXT(XrInstance instance, const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrSetDebugUtilsObjectNameEXT(
    XrInstance instance,
    const XrDebugUtilsObjectNameInfoEXT* nameInfo);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
    XrDebugUtilsMessengerEXT* messenger);
XrResult GenValidUsageInputsXrCreateDebugUtilsMessengerEXT(XrInstance instance, const XrDebugUtilsMessengerCreateInfoEXT* createInfo, XrDebugUtilsMessengerEXT* messenger);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrCreateDebugUtilsMessengerEXT(
    XrInstance instance,
    const XrDebugUtilsMessengerCreateInfoEXT* createInfo,
    XrDebugUtilsMessengerEXT* messenger);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrDestroyDebugUtilsMessengerEXT(
    XrDebugUtilsMessengerEXT messenger);
XrResult GenValidUsageInputsXrDestroyDebugUtilsMessengerEXT(XrDebugUtilsMessengerEXT messenger);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrDestroyDebugUtilsMessengerEXT(
    XrDebugUtilsMessengerEXT messenger);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrSessionBeginDebugUtilsLabelRegionEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageInputsXrSessionBeginDebugUtilsLabelRegionEXT(XrSession session, const XrDebugUtilsLabelEXT* labelInfo);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrSessionBeginDebugUtilsLabelRegionEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrSessionEndDebugUtilsLabelRegionEXT(
    XrSession session);
XrResult GenValidUsageInputsXrSessionEndDebugUtilsLabelRegionEXT(XrSession session);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrSessionEndDebugUtilsLabelRegionEXT(
    XrSession session);
XRAPI_ATTR XrResult XRAPI_CALL CoreValidationXrSessionInsertDebugUtilsLabelEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);
XrResult GenValidUsageInputsXrSessionInsertDebugUtilsLabelEXT(XrSession session, const XrDebugUtilsLabelEXT* labelInfo);
XRAPI_ATTR XrResult XRAPI_CALL GenValidUsageNextXrSessionInsertDebugUtilsLabelEXT(
    XrSession session,
    const XrDebugUtilsLabelEXT* labelInfo);

// ---- XR_MSFT_spatial_anchor extension commands

// ---- XR_EXT_conformance_automation extension commands

// ---- XR_MSFT_spatial_graph_bridge extension commands

// ---- XR_EXT_hand_tracking extension commands

// ---- XR_MSFT_hand_tracking_mesh extension commands

// ---- XR_MSFT_controller_model extension commands

// ---- XR_MSFT_perception_anchor_interop extension commands

// ---- XR_MSFT_composition_layer_reprojection extension commands

// ---- XR_FB_swapchain_update_state extension commands

// ---- XR_FB_body_tracking extension commands

// ---- XR_MSFT_scene_understanding extension commands

// ---- XR_MSFT_scene_understanding_serialization extension commands

// ---- XR_FB_display_refresh_rate extension commands

// ---- XR_HTCX_vive_tracker_interaction extension commands

// ---- XR_HTC_facial_tracking extension commands

// ---- XR_FB_color_space extension commands

// ---- XR_FB_hand_tracking_mesh extension commands

// ---- XR_FB_spatial_entity extension commands

// ---- XR_FB_foveation extension commands

// ---- XR_FB_keyboard_tracking extension commands

// ---- XR_FB_triangle_mesh extension commands

// ---- XR_FB_passthrough extension commands

// ---- XR_FB_render_model extension commands

// ---- XR_VARJO_environment_depth_estimation extension commands

// ---- XR_VARJO_marker_tracking extension commands

// ---- XR_VARJO_view_offset extension commands

// ---- XR_ML_compat extension commands

// ---- XR_ML_marker_understanding extension commands

// ---- XR_ML_localization_map extension commands

// ---- XR_MSFT_spatial_anchor_persistence extension commands

// ---- XR_MSFT_scene_marker extension commands

// ---- XR_FB_spatial_entity_query extension commands

// ---- XR_FB_spatial_entity_storage extension commands

// ---- XR_OCULUS_audio_device_guid extension commands

// ---- XR_FB_spatial_entity_sharing extension commands

// ---- XR_FB_scene extension commands

// ---- XR_ALMALENCE_digital_lens_control extension commands

// ---- XR_FB_scene_capture extension commands

// ---- XR_FB_spatial_entity_container extension commands

// ---- XR_META_foveation_eye_tracked extension commands

// ---- XR_FB_face_tracking extension commands

// ---- XR_FB_eye_tracking_social extension commands

// ---- XR_FB_passthrough_keyboard_hands extension commands

// ---- XR_FB_haptic_pcm extension commands

// ---- XR_META_passthrough_preferences extension commands

// ---- XR_META_virtual_keyboard extension commands

// ---- XR_OCULUS_external_camera extension commands

// ---- XR_META_performance_metrics extension commands

// ---- XR_FB_spatial_entity_storage_batch extension commands

// ---- XR_FB_spatial_entity_user extension commands

// ---- XR_META_recommended_layer_resolution extension commands

// ---- XR_META_passthrough_color_lut extension commands

// ---- XR_META_spatial_entity_mesh extension commands

// ---- XR_FB_face_tracking2 extension commands

// ---- XR_META_environment_depth extension commands

// ---- XR_QCOM_tracking_optimization_settings extension commands

// ---- XR_HTC_passthrough extension commands

// ---- XR_HTC_foveation extension commands

// ---- XR_HTC_anchor extension commands

// ---- XR_MNDX_force_feedback_curl extension commands

// ---- XR_EXT_plane_detection extension commands

// ---- XR_EXT_future extension commands

// ---- XR_ML_user_calibration extension commands

// Current API version of the Core Validation API Layer
#define XR_CORE_VALIDATION_API_VERSION XR_CURRENT_API_VERSION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// Externs for Core Validation
extern InstanceHandleInfo g_instance_info;
extern HandleInfo<XrSession> g_session_info;
extern HandleInfo<XrSpace> g_space_info;
extern HandleInfo<XrAction> g_action_info;
extern HandleInfo<XrSwapchain> g_swapchain_info;
extern HandleInfo<XrActionSet> g_actionset_info;
extern HandleInfo<XrDebugUtilsMessengerEXT> g_debugutilsmessengerext_info;
extern HandleInfo<XrSpatialAnchorMSFT> g_spatialanchormsft_info;
extern HandleInfo<XrSpatialGraphNodeBindingMSFT> g_spatialgraphnodebindingmsft_info;
extern HandleInfo<XrHandTrackerEXT> g_handtrackerext_info;
extern HandleInfo<XrBodyTrackerFB> g_bodytrackerfb_info;
extern HandleInfo<XrSceneObserverMSFT> g_sceneobservermsft_info;
extern HandleInfo<XrSceneMSFT> g_scenemsft_info;
extern HandleInfo<XrFacialTrackerHTC> g_facialtrackerhtc_info;
extern HandleInfo<XrFoveationProfileFB> g_foveationprofilefb_info;
extern HandleInfo<XrTriangleMeshFB> g_trianglemeshfb_info;
extern HandleInfo<XrPassthroughFB> g_passthroughfb_info;
extern HandleInfo<XrPassthroughLayerFB> g_passthroughlayerfb_info;
extern HandleInfo<XrGeometryInstanceFB> g_geometryinstancefb_info;
extern HandleInfo<XrMarkerDetectorML> g_markerdetectorml_info;
extern HandleInfo<XrExportedLocalizationMapML> g_exportedlocalizationmapml_info;
extern HandleInfo<XrSpatialAnchorStoreConnectionMSFT> g_spatialanchorstoreconnectionmsft_info;
extern HandleInfo<XrSpaceUserFB> g_spaceuserfb_info;
extern HandleInfo<XrFaceTrackerFB> g_facetrackerfb_info;
extern HandleInfo<XrEyeTrackerFB> g_eyetrackerfb_info;
extern HandleInfo<XrVirtualKeyboardMETA> g_virtualkeyboardmeta_info;
extern HandleInfo<XrPassthroughColorLutMETA> g_passthroughcolorlutmeta_info;
extern HandleInfo<XrFaceTracker2FB> g_facetracker2fb_info;
extern HandleInfo<XrEnvironmentDepthProviderMETA> g_environmentdepthprovidermeta_info;
extern HandleInfo<XrEnvironmentDepthSwapchainMETA> g_environmentdepthswapchainmeta_info;
extern HandleInfo<XrPassthroughHTC> g_passthroughhtc_info;
extern HandleInfo<XrPlaneDetectorEXT> g_planedetectorext_info;void GenValidUsageCleanUpMaps(GenValidUsageXrInstanceInfo *instance_info);


// Function to convert XrObjectType to string
std::string GenValidUsageXrObjectTypeToString(const XrObjectType& type);

// Function to record all the core validation information
extern void CoreValidLogMessage(GenValidUsageXrInstanceInfo *instance_info, const std::string &message_id,
                                GenValidUsageDebugSeverity message_severity, const std::string &command_name,
                                std::vector<GenValidUsageXrObjectInfo> objects_info, const std::string &message);

