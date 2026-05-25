// *********** THIS FILE IS GENERATED - DO NOT EDIT ***********
//     See api_dump_generator.py for modifications
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

#include "api_layer_platform_defines.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct XrGeneratedDispatchTable;


// Layer's xrGetInstanceProcAddr
XRAPI_ATTR XrResult XRAPI_CALL ApiDumpLayerXrGetInstanceProcAddr(XrInstance instance,
                                          const char* name, PFN_xrVoidFunction* function);

// Api Dump Log Command
bool ApiDumpLayerRecordContent(std::vector<std::tuple<std::string, std::string, std::string>> contents);

// Api Dump Manual Functions
XrInstance FindInstanceFromDispatchTable(XrGeneratedDispatchTable* dispatch_table);
XRAPI_ATTR XrResult XRAPI_CALL ApiDumpLayerXrCreateInstance(const XrInstanceCreateInfo *info,
                                      XrInstance *instance);
XRAPI_ATTR XrResult XRAPI_CALL ApiDumpLayerXrDestroyInstance(XrInstance instance);

//Dump utility functions
bool ApiDumpDecodeNextChain(XrGeneratedDispatchTable* gen_dispatch_table, const void* value, std::string prefix,
                            std::vector<std::tuple<std::string, std::string, std::string>> &contents);

// Union/Structure Output Helper function prototypes
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrApiLayerProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExtensionProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrApplicationInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInstanceCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInstanceProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataBuffer* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemGetInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemGraphicsProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemTrackingProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSessionCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVector3f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceVelocity* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrQuaternionf* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPosef* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrReferenceSpaceCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExtent2Df* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionSpaceCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceLocation* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewConfigurationProperties* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewConfigurationView* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageBaseHeader* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageAcquireInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageWaitInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageReleaseInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSessionBeginInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrameWaitInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrameState* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrameBeginInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerBaseHeader* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrameEndInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewLocateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewState* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFovf* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrView* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionSetCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionCreateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionSuggestedBinding* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInteractionProfileSuggestedBinding* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSessionActionSetsAttachInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInteractionProfileState* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionStateGetInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionStateBoolean* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionStateFloat* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVector2f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionStateVector2f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionStatePose* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActiveActionSet* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActionsSyncInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBoundSourcesForActionEnumerateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInputSourceLocalizedNameGetInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHapticActionInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHapticBaseHeader* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBaseInStructure* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBaseOutStructure* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrOffset2Di* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExtent2Di* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRect2Di* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainSubImage* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerProjectionView* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerProjection* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerQuad* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataBaseHeader* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataEventsLost* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataInstanceLossPending* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSessionStateChanged* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataReferenceSpaceChangePending* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataInteractionProfileChanged* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHapticVibration* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrOffset2Df* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRect2Df* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVector4f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrColor4f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrColor3f* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExtent3Df* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpheref* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBoxf* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrustumf* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrUuid* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpacesLocateInfo* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceLocationData* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceLocations* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceVelocityData* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceVelocities* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerCubeKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInstanceCreateInfoAndroidKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerDepthInfoKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVulkanSwapchainFormatListCreateInfoKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerCylinderKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerEquirectKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingOpenGLWin32KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WIN32)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingOpenGLXlibKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XLIB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingOpenGLXcbKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_XCB)
#if defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingOpenGLWaylandKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL) && defined(XR_USE_PLATFORM_WAYLAND)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageOpenGLKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsRequirementsOpenGLKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingOpenGLESAndroidKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageOpenGLESKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsRequirementsOpenGLESKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingVulkanKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageVulkanKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsRequirementsVulkanKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_D3D11)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingD3D11KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageD3D11KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D11)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsRequirementsD3D11KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D11)
#if defined(XR_USE_GRAPHICS_API_D3D12)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingD3D12KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageD3D12KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
#if defined(XR_USE_GRAPHICS_API_D3D12)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsRequirementsD3D12KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_D3D12)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVisibilityMaskKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVisibilityMaskChangedKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerColorScaleBiasKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLoaderInitInfoBaseHeaderKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLoaderInitInfoAndroidKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVulkanInstanceCreateInfoKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVulkanDeviceCreateInfoKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVulkanGraphicsDeviceGetInfoKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerEquirect2KHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBindingModificationBaseHeaderKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBindingModificationsKHR* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataPerfSettingsEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDebugUtilsObjectNameInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDebugUtilsLabelEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDebugUtilsMessengerCallbackDataEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDebugUtilsMessengerCreateInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemEyeGazeInteractionPropertiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEyeGazeSampleTimeEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSessionCreateInfoOverlayEXTX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataMainSessionVisibilityChangedEXTX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorSpaceCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerImageLayoutFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerAlphaBlendFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewConfigurationDepthRangeEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_EGL)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGraphicsBindingEGLMNDX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_EGL)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialGraphNodeSpaceCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialGraphStaticNodeBindingCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialGraphNodeBindingPropertiesGetInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialGraphNodeBindingPropertiesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemHandTrackingPropertiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackerCreateInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointsLocateInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointLocationEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointVelocityEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointLocationsEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointVelocitiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemHandTrackingMeshPropertiesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshSpaceCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshUpdateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshIndexBufferMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshVertexMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshVertexBufferMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandMeshMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandPoseTypeInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationSessionBeginInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationStateMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationFrameStateMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationLayerInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationFrameEndInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSecondaryViewConfigurationSwapchainCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrControllerModelKeyStateMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrControllerModelNodePropertiesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrControllerModelPropertiesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrControllerModelNodeStateMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrControllerModelStateMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewConfigurationViewFovEPIC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_WIN32) && defined(XR_USE_PLATFORM_WIN32)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHolographicWindowAttachmentMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_WIN32) && defined(XR_USE_PLATFORM_WIN32)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerReprojectionInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerReprojectionPlaneOverrideMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_ANDROID) && defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrAndroidSurfaceSwapchainCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_ANDROID) && defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainStateBaseHeaderFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerSecureContentFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodyJointLocationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemBodyTrackingPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodyTrackerCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodySkeletonJointFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodySkeletonFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodyJointsLocateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBodyJointLocationsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInteractionProfileDpadBindingEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrInteractionProfileAnalogThresholdVALVE* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandJointsMotionRangeInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrUuidMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneObserverCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneSphereBoundMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneOrientedBoxBoundMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneFrustumBoundMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneBoundsMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrNewSceneComputeInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVisualMeshComputeLodInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentsMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentsGetInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentLocationMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentLocationsMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentsLocateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneObjectMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneObjectsMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneComponentParentFilterInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneObjectTypesFilterInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrScenePlaneMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrScenePlanesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrScenePlaneAlignmentFilterInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshBuffersGetInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshBuffersMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshVertexBufferMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshIndicesUint32MSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMeshIndicesUint16MSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSerializedSceneFragmentDataGetInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDeserializeSceneFragmentMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneDeserializeInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataDisplayRefreshRateChangedFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViveTrackerPathsHTCX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataViveTrackerConnectedHTCX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemFacialTrackingPropertiesHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFacialExpressionsHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFacialTrackerCreateInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemColorSpacePropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVector4sFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingMeshFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingScaleFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingAimStateFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandCapsuleFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingCapsulesStateFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemSpatialEntityPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceComponentStatusSetInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceComponentStatusFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpatialAnchorCreateCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceSetStatusCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationProfileCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainCreateInfoFoveationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainStateFoveationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationLevelProfileCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemKeyboardTrackingPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrKeyboardTrackingDescriptionFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrKeyboardSpaceCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrKeyboardTrackingQueryFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrTriangleMeshCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemPassthroughPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemPassthroughProperties2FB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughLayerCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerPassthroughFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGeometryInstanceCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGeometryInstanceTransformFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughStyleFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorMapMonoToRgbaFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorMapMonoToMonoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughBrightnessContrastSaturationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataPassthroughStateChangedFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRenderModelPathInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRenderModelPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRenderModelBufferFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRenderModelLoadInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemRenderModelPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRenderModelCapabilitiesRequestFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrViewLocateFoveatedRenderingVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveatedViewConfigurationViewVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemFoveatedRenderingPropertiesVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerDepthTestVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemMarkerTrackingPropertiesVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataMarkerTrackingUpdateVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerSpaceCreateInfoVARJO* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFrameEndInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrGlobalDimmerFrameEndInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_PLATFORM_ML)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCoordinateSpaceCreateInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_ML)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemMarkerUnderstandingPropertiesML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorCreateInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorArucoInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorSizeInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorAprilTagInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorCustomProfileInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorSnapshotInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerDetectorStateML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMarkerSpaceCreateInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLocalizationMapML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataLocalizationChangedML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLocalizationMapQueryInfoBaseHeaderML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrMapLocalizationRequestInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLocalizationMapImportInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLocalizationEnableEventsInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorPersistenceNameMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorPersistenceInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorFromPersistedAnchorCreateInfoMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMarkerMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMarkersMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMarkerTypeFilterMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMarkerQRCodeMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneMarkerQRCodesMSFT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceQueryInfoBaseHeaderFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceFilterInfoBaseHeaderFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceQueryInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceStorageLocationFilterInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceUuidFilterInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceComponentFilterInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceQueryResultFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceQueryResultsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceQueryResultsAvailableFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceQueryCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceSaveInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceEraseInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceSaveCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceEraseCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainImageFoveationVulkanFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
#if defined(XR_USE_PLATFORM_ANDROID) && defined(XR_USE_PLATFORM_ANDROID)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainStateAndroidSurfaceDimensionsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_PLATFORM_ANDROID) && defined(XR_USE_PLATFORM_ANDROID)
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_GRAPHICS_API_OPENGL_ES)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainStateSamplerOpenGLESFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_OPENGL_ES) && defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_GRAPHICS_API_VULKAN) && defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSwapchainStateSamplerVulkanFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN) && defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceShareInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceShareCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerSpaceWarpInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemSpaceWarpPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHapticAmplitudeEnvelopeVibrationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrOffset3DfFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRect3DfFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSemanticLabelsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRoomLayoutFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrBoundary2DFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSemanticLabelsSupportInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDigitalLensControlALMALENCE* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSceneCaptureCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSceneCaptureRequestInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceContainerFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationEyeTrackedProfileCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationEyeTrackedStateMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemFoveationEyeTrackedPropertiesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemFaceTrackingPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceTrackerCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceExpressionInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceExpressionStatusFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceExpressionWeightsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEyeGazeFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEyeTrackerCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEyeGazesInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemEyeTrackingPropertiesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEyeGazesFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughKeyboardHandsIntensityFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerSettingsFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHapticPcmVibrationFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrDevicePcmSampleRateStateFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerDepthTestFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrLocalDimmingFrameEndInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughPreferencesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemVirtualKeyboardPropertiesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardSpaceCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardLocationInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardModelVisibilitySetInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardAnimationStateMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardModelAnimationStatesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardTextureDataMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardInputInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVirtualKeyboardTextContextChangeInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVirtualKeyboardCommitTextMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVirtualKeyboardBackspaceMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVirtualKeyboardEnterMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVirtualKeyboardShownMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataVirtualKeyboardHiddenMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExternalCameraIntrinsicsOCULUS* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExternalCameraExtrinsicsOCULUS* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrExternalCameraOCULUS* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#if defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrVulkanSwapchainCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
#endif // defined(XR_USE_GRAPHICS_API_VULKAN)
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPerformanceMetricsStateMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPerformanceMetricsCounterMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceListSaveInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataSpaceListSaveCompleteFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceUserCreateInfoFB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemHeadsetIdPropertiesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRecommendedLayerResolutionMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrRecommendedLayerResolutionGetInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorLutDataMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorLutCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorLutUpdateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorMapLutMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorMapInterpolatedLutMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemPassthroughColorLutPropertiesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceTriangleMeshGetInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpaceTriangleMeshMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemFaceTrackingProperties2FB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceTrackerCreateInfo2FB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceExpressionInfo2FB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFaceExpressionWeights2FB* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthProviderCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthSwapchainCreateInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthSwapchainStateMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthImageAcquireInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthImageViewMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthImageMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEnvironmentDepthHandRemovalSetInfoMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemEnvironmentDepthPropertiesMETA* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughCreateInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughColorHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPassthroughMeshTransformInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrCompositionLayerPassthroughHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationApplyInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationConfigurationHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationDynamicModeInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFoveationCustomModeInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemAnchorPropertiesHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorNameHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSpatialAnchorCreateInfoHTC* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActiveActionSetPriorityEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrActiveActionSetPrioritiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemForceFeedbackCurlPropertiesMNDX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrForceFeedbackCurlApplyLocationMNDX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrForceFeedbackCurlApplyLocationsMNDX* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingDataSourceInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrHandTrackingDataSourceStateEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemPlaneDetectionPropertiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorCreateInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorBeginInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorGetInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorLocationEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorLocationsEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrPlaneDetectorPolygonBufferEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFutureCancelInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFuturePollInfoEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFutureCompletionBaseHeaderEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFutureCompletionEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrFuturePollResultEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataUserPresenceChangedEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrSystemUserPresencePropertiesEXT* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataHeadsetFitChangedML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrEventDataEyeCalibrationChangedML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);
bool ApiDumpOutputXrStruct(XrGeneratedDispatchTable* gen_dispatch_table, const XrUserCalibrationEnableEventsInfoML* value,
                           std::string prefix, std::string type_string, bool is_pointer,
                           std::vector<std::tuple<std::string, std::string, std::string>> &contents);

// Externs for API dump
extern std::unordered_map<XrInstance, XrGeneratedDispatchTable*> g_instance_dispatch_map;
extern std::mutex g_instance_dispatch_mutex;
extern std::unordered_map<XrSession, XrGeneratedDispatchTable*> g_session_dispatch_map;
extern std::mutex g_session_dispatch_mutex;
extern std::unordered_map<XrSpace, XrGeneratedDispatchTable*> g_space_dispatch_map;
extern std::mutex g_space_dispatch_mutex;
extern std::unordered_map<XrAction, XrGeneratedDispatchTable*> g_action_dispatch_map;
extern std::mutex g_action_dispatch_mutex;
extern std::unordered_map<XrSwapchain, XrGeneratedDispatchTable*> g_swapchain_dispatch_map;
extern std::mutex g_swapchain_dispatch_mutex;
extern std::unordered_map<XrActionSet, XrGeneratedDispatchTable*> g_actionset_dispatch_map;
extern std::mutex g_actionset_dispatch_mutex;
extern std::unordered_map<XrDebugUtilsMessengerEXT, XrGeneratedDispatchTable*> g_debugutilsmessengerext_dispatch_map;
extern std::mutex g_debugutilsmessengerext_dispatch_mutex;
extern std::unordered_map<XrSpatialAnchorMSFT, XrGeneratedDispatchTable*> g_spatialanchormsft_dispatch_map;
extern std::mutex g_spatialanchormsft_dispatch_mutex;
extern std::unordered_map<XrSpatialGraphNodeBindingMSFT, XrGeneratedDispatchTable*> g_spatialgraphnodebindingmsft_dispatch_map;
extern std::mutex g_spatialgraphnodebindingmsft_dispatch_mutex;
extern std::unordered_map<XrHandTrackerEXT, XrGeneratedDispatchTable*> g_handtrackerext_dispatch_map;
extern std::mutex g_handtrackerext_dispatch_mutex;
extern std::unordered_map<XrBodyTrackerFB, XrGeneratedDispatchTable*> g_bodytrackerfb_dispatch_map;
extern std::mutex g_bodytrackerfb_dispatch_mutex;
extern std::unordered_map<XrSceneObserverMSFT, XrGeneratedDispatchTable*> g_sceneobservermsft_dispatch_map;
extern std::mutex g_sceneobservermsft_dispatch_mutex;
extern std::unordered_map<XrSceneMSFT, XrGeneratedDispatchTable*> g_scenemsft_dispatch_map;
extern std::mutex g_scenemsft_dispatch_mutex;
extern std::unordered_map<XrFacialTrackerHTC, XrGeneratedDispatchTable*> g_facialtrackerhtc_dispatch_map;
extern std::mutex g_facialtrackerhtc_dispatch_mutex;
extern std::unordered_map<XrFoveationProfileFB, XrGeneratedDispatchTable*> g_foveationprofilefb_dispatch_map;
extern std::mutex g_foveationprofilefb_dispatch_mutex;
extern std::unordered_map<XrTriangleMeshFB, XrGeneratedDispatchTable*> g_trianglemeshfb_dispatch_map;
extern std::mutex g_trianglemeshfb_dispatch_mutex;
extern std::unordered_map<XrPassthroughFB, XrGeneratedDispatchTable*> g_passthroughfb_dispatch_map;
extern std::mutex g_passthroughfb_dispatch_mutex;
extern std::unordered_map<XrPassthroughLayerFB, XrGeneratedDispatchTable*> g_passthroughlayerfb_dispatch_map;
extern std::mutex g_passthroughlayerfb_dispatch_mutex;
extern std::unordered_map<XrGeometryInstanceFB, XrGeneratedDispatchTable*> g_geometryinstancefb_dispatch_map;
extern std::mutex g_geometryinstancefb_dispatch_mutex;
extern std::unordered_map<XrMarkerDetectorML, XrGeneratedDispatchTable*> g_markerdetectorml_dispatch_map;
extern std::mutex g_markerdetectorml_dispatch_mutex;
extern std::unordered_map<XrExportedLocalizationMapML, XrGeneratedDispatchTable*> g_exportedlocalizationmapml_dispatch_map;
extern std::mutex g_exportedlocalizationmapml_dispatch_mutex;
extern std::unordered_map<XrSpatialAnchorStoreConnectionMSFT, XrGeneratedDispatchTable*> g_spatialanchorstoreconnectionmsft_dispatch_map;
extern std::mutex g_spatialanchorstoreconnectionmsft_dispatch_mutex;
extern std::unordered_map<XrSpaceUserFB, XrGeneratedDispatchTable*> g_spaceuserfb_dispatch_map;
extern std::mutex g_spaceuserfb_dispatch_mutex;
extern std::unordered_map<XrFaceTrackerFB, XrGeneratedDispatchTable*> g_facetrackerfb_dispatch_map;
extern std::mutex g_facetrackerfb_dispatch_mutex;
extern std::unordered_map<XrEyeTrackerFB, XrGeneratedDispatchTable*> g_eyetrackerfb_dispatch_map;
extern std::mutex g_eyetrackerfb_dispatch_mutex;
extern std::unordered_map<XrVirtualKeyboardMETA, XrGeneratedDispatchTable*> g_virtualkeyboardmeta_dispatch_map;
extern std::mutex g_virtualkeyboardmeta_dispatch_mutex;
extern std::unordered_map<XrPassthroughColorLutMETA, XrGeneratedDispatchTable*> g_passthroughcolorlutmeta_dispatch_map;
extern std::mutex g_passthroughcolorlutmeta_dispatch_mutex;
extern std::unordered_map<XrFaceTracker2FB, XrGeneratedDispatchTable*> g_facetracker2fb_dispatch_map;
extern std::mutex g_facetracker2fb_dispatch_mutex;
extern std::unordered_map<XrEnvironmentDepthProviderMETA, XrGeneratedDispatchTable*> g_environmentdepthprovidermeta_dispatch_map;
extern std::mutex g_environmentdepthprovidermeta_dispatch_mutex;
extern std::unordered_map<XrEnvironmentDepthSwapchainMETA, XrGeneratedDispatchTable*> g_environmentdepthswapchainmeta_dispatch_map;
extern std::mutex g_environmentdepthswapchainmeta_dispatch_mutex;
extern std::unordered_map<XrPassthroughHTC, XrGeneratedDispatchTable*> g_passthroughhtc_dispatch_map;
extern std::mutex g_passthroughhtc_dispatch_mutex;
extern std::unordered_map<XrPlaneDetectorEXT, XrGeneratedDispatchTable*> g_planedetectorext_dispatch_map;
extern std::mutex g_planedetectorext_dispatch_mutex;
void ApiDumpCleanUpMapsForTable(XrGeneratedDispatchTable *table);

