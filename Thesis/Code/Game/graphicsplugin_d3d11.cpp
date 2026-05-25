#include "Game/graphicsplugin_d3d11.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/Image.hpp"
#include "Game/check.h"
#include <directxmath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

const char* XRdefaultShaderSource = R"(
float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	float disp = rangeEnd - rangeStart;
	float proportion = (value - rangeStart) / disp;
	return proportion;
}

float Interpolate(float start, float end, float fractionTowardEnd)
{
	float disp = end - start;
	float distWithinRange = disp * fractionTowardEnd;
	float interpolatedPosition = start + distWithinRange;
	return interpolatedPosition;
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);

	float outValue = Interpolate(outStart, outEnd, proportion);
	return outValue;
}

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

    cbuffer ModelConstantBuffer : register(b0) 
    {
        float4x4 ModelMatrix;
	    // float4 ModelColor;
    };

    cbuffer ViewProjectionConstantBuffer : register(b1) 
    {
        float4x4 ViewProjection;
		// float4x4 ViewMatrix;
		// float4x4 ProjectionMatrix;
    };

  	cbuffer CameraConstants : register(b2)
  {
  	float4x4 ViewMatrix;
  	float4x4 ProjectionMatrix;
  };

struct vs_input_t
	{
		float3 localPosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};

	struct v2p_t
	{
		float4 position : SV_Position;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
	};


	v2p_t VertexMain(vs_input_t input)
	{
		float4 localPosition = float4(input.localPosition, 1);
		float4 worldPosition = mul(ModelMatrix, localPosition);
		float4 renderPosition = mul(ViewMatrix, worldPosition);
		float4 clipPosition = mul(ProjectionMatrix, renderPosition);

		// float4 clipPosition = mul(mul(float4(input.localPosition, 1), ModelMatrix), ViewProjection);
		
		v2p_t v2p;
		v2p.position = clipPosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		textureColor *= input.color;
		clip(textureColor.a - 0.01f);
		return float4(textureColor);
	}

	)";

const int k_modelConstantsSlot = 0;
const int k_cameraConstantsSlot = 2;
const int k_viewProjectionConstantsSlot = 1;

struct ModelConstants
{
	Mat44 ModelMatrix;
	// float ModelColor[4];
	// DirectX::XMFLOAT4X4 ModelMatrix;
};

struct ViewProjectionConstants 
{
	DirectX::XMFLOAT4X4 ViewProjectionMatrix;
};

struct CameraConstants
{
	Mat44 ViewMatrix;
	Mat44 ProjectionMatrix;
};

void XRRenderer::Startup()
{
    CreateModelConstantBuffer();
	CreateCameraConstantBuffer();
    CreateViewProjectionConstantBuffer();
	// create the immediate vertex and specify an initial size big enough for one Vertex_PCU
	size_t vertexSize = sizeof(Vertex_PCU);
	m_immediateVBO = new VertexBuffer(3, vertexSize);
	CreateAndBindDefaultShader();

	CreateAllBlendStates();

	CreateAndSetRasterizerStates();
	CreateAndSetDepthStencilStates();

	CreateDefaultTexture();
	BindTexture(m_defaultTexture);
}

void XRRenderer::CreateSwapchains(XrSession XRSession, XrInstance XRInstance, XrSystemId XRSystemId, const std::shared_ptr<const Options> programOptions)
{
	CHECK(XRSession != XR_NULL_HANDLE);
	CHECK(m_swapchains.empty());
	CHECK(m_configViews.empty());

	// Read graphics properties for preferred swapchain length and logging.
	XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
	CHECK_XRCMD(xrGetSystemProperties(XRInstance, XRSystemId, &systemProperties));

	// Log system properties.
	Log::Write(Log::Level::Info,
		Fmt("System Properties: Name=%s VendorId=%d", systemProperties.systemName, systemProperties.vendorId));
	Log::Write(Log::Level::Info, Fmt("System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
		systemProperties.graphicsProperties.maxSwapchainImageWidth,
		systemProperties.graphicsProperties.maxSwapchainImageHeight,
		systemProperties.graphicsProperties.maxLayerCount));
	Log::Write(Log::Level::Info, Fmt("System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
		systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False",
		systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));

	// Note: No other view configurations exist at the time this code was written. If this
	// condition is not met, the project will need to be audited to see how support should be
	// added.
	CHECK_MSG(programOptions->Parsed.ViewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		"Unsupported view configuration type");

	// Query and cache view configuration views.
	uint32_t viewCount;
	CHECK_XRCMD(
		xrEnumerateViewConfigurationViews(XRInstance, XRSystemId, programOptions->Parsed.ViewConfigType, 0, &viewCount, nullptr));
	m_configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	CHECK_XRCMD(xrEnumerateViewConfigurationViews(XRInstance, XRSystemId, programOptions->Parsed.ViewConfigType, viewCount,
		&viewCount, m_configViews.data()));

	// Create and cache view buffer for xrLocateViews later.
	m_views.resize(viewCount, { XR_TYPE_VIEW });

	// Create the swapchain and get the images.
	if (viewCount > 0) {
		// Select a swapchain format.
		uint32_t swapchainFormatCount;
		CHECK_XRCMD(xrEnumerateSwapchainFormats(XRSession, 0, &swapchainFormatCount, nullptr));
		std::vector<int64_t> swapchainFormats(swapchainFormatCount);
		CHECK_XRCMD(xrEnumerateSwapchainFormats(XRSession, (uint32_t)swapchainFormats.size(), &swapchainFormatCount,
			swapchainFormats.data()));
		CHECK(swapchainFormatCount == swapchainFormats.size());
		m_colorSwapchainFormat = SelectColorSwapchainFormat(swapchainFormats);

		// Print swapchain formats and the selected one.
		{
			std::string swapchainFormatsString;
			for (int64_t format : swapchainFormats) {
				const bool selected = format == m_colorSwapchainFormat;
				swapchainFormatsString += " ";
				if (selected) {
					swapchainFormatsString += "[";
				}
				swapchainFormatsString += std::to_string(format);
				if (selected) {
					swapchainFormatsString += "]";
				}
			}
			Log::Write(Log::Level::Verbose, Fmt("Swapchain Formats: %s", swapchainFormatsString.c_str()));
		}

		// Create a swapchain for each view.
		for (uint32_t i = 0; i < viewCount; i++) {
			const XrViewConfigurationView& vp = m_configViews[i];
			Log::Write(Log::Level::Info,
				Fmt("Creating swapchain for view %d with dimensions Width=%d Height=%d SampleCount=%d", i,
					vp.recommendedImageRectWidth, vp.recommendedImageRectHeight, vp.recommendedSwapchainSampleCount));

			// Create the swapchain.
			XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
			swapchainCreateInfo.arraySize = 1;
			swapchainCreateInfo.format = m_colorSwapchainFormat;
			swapchainCreateInfo.width = vp.recommendedImageRectWidth;
			swapchainCreateInfo.height = vp.recommendedImageRectHeight;
			swapchainCreateInfo.mipCount = 1;
			swapchainCreateInfo.faceCount = 1;
			swapchainCreateInfo.sampleCount = GetSupportedSwapchainSampleCount(vp);
			swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
			Swapchain swapchain;
			swapchain.width = swapchainCreateInfo.width;
			swapchain.height = swapchainCreateInfo.height;
			CHECK_XRCMD(xrCreateSwapchain(XRSession, &swapchainCreateInfo, &swapchain.handle));

			m_swapchains.push_back(swapchain);

			uint32_t imageCount;
			CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
			// XXX This should really just return XrSwapchainImageBaseHeader*
			std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
			CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]));

			m_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
		}
	}
}

void XRRenderer :: InitializeDevice(XrInstance instance, XrSystemId systemId)
{
    PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",
                                        reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR)));

    // Create the D3D11 device for the adapter associated with the system.
    XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    CHECK_XRCMD(pfnGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));
    const ComPtr<IDXGIAdapter1> adapter = GetAdapter(graphicsRequirements.adapterLuid);

    // Create a list of feature levels which are both supported by the OpenXR runtime and this application.
    std::vector<D3D_FEATURE_LEVEL> featureLevels = {D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
                                                    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
    featureLevels.erase(std::remove_if(featureLevels.begin(), featureLevels.end(),
                                        [&](D3D_FEATURE_LEVEL fl) { return fl < graphicsRequirements.minFeatureLevel; }),
                        featureLevels.end());
    CHECK_MSG(featureLevels.size() != 0, "Unsupported minimum feature level!");

    InitializeD3D11DeviceForAdapter(adapter.Get(), featureLevels, m_device.ReleaseAndGetAddressOf(),
                                    m_deviceContext.ReleaseAndGetAddressOf());

    // InitializeResources();

    m_graphicsBinding.device = m_device.Get();
}

void XRRenderer :: InitializeResources() 
{
	CreateAndBindDefaultShader();
    //const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(ShaderHlsl, "MainVS", "vs_5_0");
    //CHECK_HRCMD(m_device->CreateVertexShader(vertexShaderBytes->GetBufferPointer(), vertexShaderBytes->GetBufferSize(), nullptr,
    //                                         m_vertexShader.ReleaseAndGetAddressOf()));
    //
    //const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(ShaderHlsl, "MainPS", "ps_5_0");
    //CHECK_HRCMD(m_device->CreatePixelShader(pixelShaderBytes->GetBufferPointer(), pixelShaderBytes->GetBufferSize(), nullptr,
    //                                        m_pixelShader.ReleaseAndGetAddressOf()));
    //
    //const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
    //    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //    {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    //};
    //
    //CHECK_HRCMD(m_device->CreateInputLayout(vertexDesc, (UINT)ArraySize(vertexDesc), vertexShaderBytes->GetBufferPointer(),
    //                                        vertexShaderBytes->GetBufferSize(), &m_inputLayout));

	// const ComPtr<ID3DBlob> vertexShaderBytes = CompileShader(XRdefaultShaderSource, "VertexMain", "vs_5_0");
	// CHECK_HRCMD(m_device->CreateVertexShader(vertexShaderBytes->GetBufferPointer(), vertexShaderBytes->GetBufferSize(), nullptr,
	// 	m_vertexShader.ReleaseAndGetAddressOf()));
	// 
	// const ComPtr<ID3DBlob> pixelShaderBytes = CompileShader(XRdefaultShaderSource, "PixelMain", "ps_5_0");
	// CHECK_HRCMD(m_device->CreatePixelShader(pixelShaderBytes->GetBufferPointer(), pixelShaderBytes->GetBufferSize(), nullptr,
	// 	m_pixelShader.ReleaseAndGetAddressOf()));
	// 
	// const CD3D11_BUFFER_DESC modelConstantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	// CHECK_HRCMD(m_device->CreateBuffer(&modelConstantBufferDesc, nullptr, m_modelCBuffer.ReleaseAndGetAddressOf()));

    // const CD3D11_BUFFER_DESC viewProjectionConstantBufferDesc(sizeof(ViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    // CHECK_HRCMD(
    //     m_device->CreateBuffer(&viewProjectionConstantBufferDesc, nullptr, m_viewProjectionCBuffer.ReleaseAndGetAddressOf()));

	// const D3D11_SUBRESOURCE_DATA vertexBufferData{Geometry::c_cubeVertices};
	// const CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(Geometry::c_cubeVertices), D3D11_BIND_VERTEX_BUFFER);
	// CHECK_HRCMD(m_device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, m_cubeVertexBuffer.ReleaseAndGetAddressOf()));
	// 
	// const D3D11_SUBRESOURCE_DATA indexBufferData{Geometry::c_cubeIndices};
	// const CD3D11_BUFFER_DESC indexBufferDesc(sizeof(Geometry::c_cubeIndices), D3D11_BIND_INDEX_BUFFER);
	// CHECK_HRCMD(m_device->CreateBuffer(&indexBufferDesc, &indexBufferData, m_cubeIndexBuffer.ReleaseAndGetAddressOf()));
}

int64_t XRRenderer :: SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const
{
    // List of supported color swapchain formats.
    constexpr DXGI_FORMAT SupportedColorSwapchainFormats[] = 
    {
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM,
    };

    auto swapchainFormatIt =
        std::find_first_of(runtimeFormats.begin(), runtimeFormats.end(), std::begin(SupportedColorSwapchainFormats),
                            std::end(SupportedColorSwapchainFormats));
    if (swapchainFormatIt == runtimeFormats.end()) 
    {
        THROW("No runtime swapchain format supported for color swapchain");
    }

    return *swapchainFormatIt; // return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB as result
}

std::vector<XrSwapchainImageBaseHeader*> XRRenderer :: AllocateSwapchainImageStructs(
    uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/) 
{
    // Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
    // Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
    std::vector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(capacity, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    std::vector<XrSwapchainImageBaseHeader*> swapchainImageBase;
    for (XrSwapchainImageD3D11KHR& image : swapchainImageBuffer) 
    {
        swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
    }

    // Keep the buffer alive by moving it into the list of buffers.
    m_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

    return swapchainImageBase;
}

ComPtr<ID3D11DepthStencilView> XRRenderer::GetDepthStencilView(ID3D11Texture2D* colorTexture)
{
    // If a depth-stencil view has already been created for this back-buffer, use it.
    auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
    if (depthBufferIt != m_colorToDepthMap.end()) 
	{
        return depthBufferIt->second;
    }

    // This back-buffer has no corresponding depth-stencil texture, so create one with matching dimensions.
    D3D11_TEXTURE2D_DESC colorDesc;
    colorTexture->GetDesc(&colorDesc);

    D3D11_TEXTURE2D_DESC depthDesc{};
    depthDesc.Width = colorDesc.Width;
    depthDesc.Height = colorDesc.Height;
    depthDesc.ArraySize = colorDesc.ArraySize;
    depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthDesc.BindFlags =D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    depthDesc.SampleDesc.Count = 1;
    ComPtr<ID3D11Texture2D> depthTexture;
    CHECK_HRCMD(m_device->CreateTexture2D(&depthDesc, nullptr, depthTexture.ReleaseAndGetAddressOf()));

    // Create and cache the depth stencil view.
    ComPtr<ID3D11DepthStencilView> depthStencilView;
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
    CHECK_HRCMD(m_device->CreateDepthStencilView(depthTexture.Get(), &depthStencilViewDesc, depthStencilView.GetAddressOf()));
    depthBufferIt = m_colorToDepthMap.insert(std::make_pair(colorTexture, depthStencilView)).first;

    return depthStencilView;
}

void XRRenderer::BeginCamera(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
	int64_t swapchainFormat)
{
	CHECK(layerView.subImage.imageArrayIndex == 0);  // Texture arrays not supported.

	ID3D11Texture2D* const colorTexture = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture;

	CD3D11_VIEWPORT viewport((float)layerView.subImage.imageRect.offset.x, (float)layerView.subImage.imageRect.offset.y,
		(float)layerView.subImage.imageRect.extent.width,
		(float)layerView.subImage.imageRect.extent.height);
	m_deviceContext->RSSetViewports(1, &viewport);

	// Create RenderTargetView with original swapchain format (swapchain is typeless).
	ComPtr<ID3D11RenderTargetView> renderTargetView;
	const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, (DXGI_FORMAT)swapchainFormat);
	CHECK_HRCMD(
		m_device->CreateRenderTargetView(colorTexture, &renderTargetViewDesc, renderTargetView.ReleaseAndGetAddressOf()));

	const ComPtr<ID3D11DepthStencilView> depthStencilView = GetDepthStencilView(colorTexture);

	// Clear swapchain and depth buffer. NOTE: This will clear the entire render target view, not just the specified view.
	m_deviceContext->ClearRenderTargetView(renderTargetView.Get(), static_cast<const FLOAT*>(&m_clearScreenColor.x));
	m_deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11RenderTargetView* renderTargets[] = { renderTargetView.Get() };
	m_deviceContext->OMSetRenderTargets((UINT)ArraySize(renderTargets), renderTargets, depthStencilView.Get());

	// m_deviceContext->RSSetState(m_rasterizerState[0]);

    //----------------------------------------------------------------------------------------------------------------------------------------------------
    // constant buffer setting
	// the world to render matrix 
	// the way of using DiretX math library is following:
	// Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3());
	// XrMatrix4x4f renderMatrix = renderMat.GetXrMatByMat();
	// XMMATRIX renderXMMat(LoadXrMatrix(renderMatrix));
	// 
	// const XMMATRIX spaceToView = XMMatrixInverse(nullptr, LoadXrPose(layerView.pose)); // layerView.pose is the pose of the eye
	// XMMATRIX worldToView = renderXMMat * spaceToView;
	// XrMatrix4x4f projectionMatrix;
	// XrMatrix4x4f_CreateProjectionFov(&projectionMatrix, GRAPHICS_D3D, layerView.fov, 0.05f, 100.0f);
	// 
	// // Set shaders and constant buffers.
	// ViewProjectionConstantBuffer viewProjection;
	// XMStoreFloat4x4(&viewProjection.ViewProjection, XMMatrixTranspose(worldToView * LoadXrMatrix(projectionMatrix)));
	// // m_deviceContext->UpdateSubresource(m_viewProjectionCBuffer.Get(), 0, nullptr, &viewProjection, 0, 0);
	// SetViewProjectionConstants(viewProjection.ViewProjection);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// the way of using our own math library is following
	Mat44 renderMat(Vec3(0.f, 0.f, -1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3()); // convert openXr game coordinate to our game coordinate

	Mat44 worldToViewMat(layerView.pose); // layerView.pose is the pose of the eye
	worldToViewMat = worldToViewMat.GetOrthonormalInverse();
	worldToViewMat = worldToViewMat.MatMultiply(renderMat);

	XrMatrix4x4f projectionMatrix;
	XrMatrix4x4f_CreateProjectionFov(&projectionMatrix, GRAPHICS_D3D, layerView.fov, 0.05f, 100.0f);
	Mat44 projectionMat(projectionMatrix);
	// projectionMat = projectionMat.MatMultiply(renderMat);

	// Set shaders and constant buffers.
	CameraConstants cameraInfo;
	cameraInfo.ProjectionMatrix = projectionMat;
	cameraInfo.ViewMatrix = worldToViewMat;

	CopyCPUToGPU(&cameraInfo, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(k_cameraConstantsSlot, m_cameraCBO);

	// BindShader(nullptr);
}

void XRRenderer::DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray)
{
	// draw vertex buffer
	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArraySize = numVertices * vertexSize;
	CopyCPUToGPU(vertexArray, vertexArraySize, m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, numVertices, 0);
}

void XRRenderer::RenderView(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
                int64_t swapchainFormat, const std::vector<Cube>& cubes) 
{
    CHECK(layerView.subImage.imageArrayIndex == 0);  // Texture arrays not supported.

    ID3D11Texture2D* const colorTexture = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture;

    CD3D11_VIEWPORT viewport((float)layerView.subImage.imageRect.offset.x, (float)layerView.subImage.imageRect.offset.y,
                                (float)layerView.subImage.imageRect.extent.width,
                                (float)layerView.subImage.imageRect.extent.height);
    m_deviceContext->RSSetViewports(1, &viewport);

    // Create RenderTargetView with original swapchain format (swapchain is typeless).
    ComPtr<ID3D11RenderTargetView> renderTargetView;
    const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, (DXGI_FORMAT)swapchainFormat);
    CHECK_HRCMD(
        m_device->CreateRenderTargetView(colorTexture, &renderTargetViewDesc, renderTargetView.ReleaseAndGetAddressOf()));

    const ComPtr<ID3D11DepthStencilView> depthStencilView = GetDepthStencilView(colorTexture);

    // Clear swapchain and depth buffer. NOTE: This will clear the entire render target view, not just the specified view.
    m_deviceContext->ClearRenderTargetView(renderTargetView.Get(), static_cast<const FLOAT*>(m_clearColor.data()));
    m_deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    ID3D11RenderTargetView* renderTargets[] = {renderTargetView.Get()};
    m_deviceContext->OMSetRenderTargets((UINT)ArraySize(renderTargets), renderTargets, depthStencilView.Get());

    const XMMATRIX spaceToView = XMMatrixInverse(nullptr, LoadXrPose(layerView.pose));
    XrMatrix4x4f projectionMatrix;
    XrMatrix4x4f_CreateProjectionFov(&projectionMatrix, GRAPHICS_D3D, layerView.fov, 0.05f, 100.0f);

    // Set shaders and constant buffers.
    ViewProjectionConstantBuffer viewProjection;
    XMStoreFloat4x4(&viewProjection.ViewProjection, XMMatrixTranspose(spaceToView * LoadXrMatrix(projectionMatrix)));
    m_deviceContext->UpdateSubresource(m_viewProjectionCBuffer.Get(), 0, nullptr, &viewProjection, 0, 0);

    ID3D11Buffer* const constantBuffers[] = {m_modelCBuffer.Get(), m_viewProjectionCBuffer.Get()};
    m_deviceContext->VSSetConstantBuffers(0, (UINT)ArraySize(constantBuffers), constantBuffers);
    m_deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Set cube primitive data.
    const UINT strides[] = {sizeof(Geometry::Vertex)};
    const UINT offsets[] = {0};
    ID3D11Buffer* vertexBuffers[] = {m_cubeVertexBuffer.Get()};
    m_deviceContext->IASetVertexBuffers(0, (UINT)ArraySize(vertexBuffers), vertexBuffers, strides, offsets);
    m_deviceContext->IASetInputLayout(m_inputLayout.Get());

    // Render each cube
    for (const Cube& cube : cubes) {
        // Compute and update the model transform.
        ModelConstantBuffer model;
        XMStoreFloat4x4(&model.Model,
                        XMMatrixTranspose(XMMatrixScaling(cube.Scale.x, cube.Scale.y, cube.Scale.z) * LoadXrPose(cube.Pose)));
        m_deviceContext->UpdateSubresource(m_modelCBuffer.Get(), 0, nullptr, &model, 0, 0);

        // Draw the cube.
        m_deviceContext->DrawIndexed((UINT)ArraySize(Geometry::c_cubeIndices), 0, 0);
    }
}

void XRRenderer::SetClearScreenColor(const Rgba8& clearColor)
{
	//dx11-clear the screen
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_clearScreenColor = ConvertRGBAToVec4(clearColor);
}

void XRRenderer::CreateAllBlendStates()
{
	// create blend state for opaque rendering
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
	blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
	blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	HRESULT hr;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::OPAQUE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::OPAQUE failed");
	}

	// create alpha state for opaque rendering
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ALPHA)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ALPHA failed");
	}

	// create alpha state for additive rendering
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	hr = m_device->CreateBlendState(&blendDesc, &m_blendStates[(int)(BlendMode::ADDITIVE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateBlendState for BlendMode::ADDITIVE failed");
	}

}

void XRRenderer::SetBlendMode(BlendMode blendMode)
{
	switch (blendMode)
	{
	case BlendMode::OPAQUE: {m_desiredBlendMode = BlendMode::OPAQUE; } break;
	case BlendMode::ALPHA: {m_desiredBlendMode = BlendMode::ALPHA; } break;
	case BlendMode::ADDITIVE: {m_desiredBlendMode = BlendMode::ADDITIVE; } break;
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode)); } break;
	}

}

void XRRenderer::SetRasterizerMode(RasterizerMode rasterizerState)
{
	switch (rasterizerState)
	{
	case RasterizerMode::SOLID_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_NONE; }
										break;
	case RasterizerMode::SOLID_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK; }
										break;
	case RasterizerMode::WIREFRAME_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE; }
											break;
	case RasterizerMode::WIREFRAME_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK; }
											break;
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported rasterizer State #%i", rasterizerState)); }
		   break;
	}
}

void XRRenderer::SetStatesIfChanged()
{
	// check if the desired blend mode is the same as current blend state
	if (m_currentBlendState != m_blendStates[(int)m_desiredBlendMode])
	{
		m_currentBlendState = m_blendStates[(int)m_desiredBlendMode];

		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		UINT sampleMask = 0xffffffff;
		m_deviceContext->OMSetBlendState(m_currentBlendState, blendFactor, sampleMask);
	}


	// rasterizer mode
	if (m_desiredRasterizerMode != m_rasterizerMode)
	{
		m_deviceContext->RSSetState(m_rasterizerState[(int)m_desiredRasterizerMode]);
		m_rasterizerMode = m_desiredRasterizerMode;
	}
}

void XRRenderer::CreateAndSetDepthStencilStates()
{
	//----------------------------------------------------------------------------------------------------------------------------------------------------
// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	HRESULT hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Disable failed");
	}

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::ENABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Enabled failed");
	}

	UINT stencilRef = 0xffffffff;
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_depthMode], stencilRef);
}

//void XRRenderer::CreateStencilTextureAndViewAndAllDepthStencilStates()
//{
//	D3D11_TEXTURE2D_DESC textureDesc = {};
//	textureDesc.Width = m_config.m_window->GetClientDimensions().x;
//	textureDesc.Height = m_config.m_window->GetClientDimensions().y;
//	textureDesc.MipLevels = 1;
//	textureDesc.ArraySize = 1;
//	textureDesc.Usage = D3D11_USAGE_DEFAULT;
//	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
//	textureDesc.SampleDesc.Count = 1;
//
//	HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &m_depthStencilTexture);
//	if (!SUCCEEDED(hr))
//	{
//		ERROR_AND_DIE("Could not create texture for depth stencil.");
//	}
//
//	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilView);
//	if (!SUCCEEDED(hr))
//	{
//		ERROR_AND_DIE("Could not create depth stencil view.");
//	}
//
//	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
//	depthStencilDesc.DepthEnable = TRUE;
//
//	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
//	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
//	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
//	if (!SUCCEEDED(hr))
//	{
//		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Disable failed");
//	}
//
//	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
//	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
//	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::ENABLED]);
//	if (!SUCCEEDED(hr))
//	{
//		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Enabled failed");
//	}
//
//	UINT stencilRef = 0xffffffff;
//	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_depthMode], stencilRef);
//
//}
//
void XRRenderer::CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo)
{
	if ((vbo->m_size * vbo->m_stride) < size)
	{
		size_t stride = vbo->m_stride;
		delete vbo;
		vbo = CreateVertexBuffer(size, stride);
	}

	// copy the vertex buffer data from the CPU to GPU
	// copy vertices
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void XRRenderer::CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void XRRenderer::DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset /*= 0*/)
{
	UINT stride = vbo->m_stride;
	UINT startOffset = vertexOffset;
	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &stride, &startOffset);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	SetStatesIfChanged();

    m_deviceContext->Draw(vertexCount, vertexOffset);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// font
BitmapFont* XRRenderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	// See if we already have this font previously loaded
	BitmapFont* existingFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);
	if (existingFont)
	{
		return existingFont;
	}
	// Never seen this font before!  Let's load it.
	BitmapFont* font = CreateBitmapFont(bitmapFontFilePathWithNoExtension);
	m_loadedFonts.push_back(font);
	return font;
}

BitmapFont* XRRenderer::GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension)
{
	// see if can find the load font with the same file path name
	for (int i = 0; i < (int)m_loadedFonts.size(); i++)
	{
		if (!strcmp(m_loadedFonts[i]->m_fontFilePathNameWithNoExtension.c_str(), bitmapFontFilePathWithNoExtension))
		{
			return m_loadedFonts[i];
		}
	}
	return nullptr;
}

BitmapFont* XRRenderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	Texture* fontTexture = CreateOrGetTextureFromFile(bitmapFontFilePathWithNoExtension);
	BitmapFont* font = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture); 
	return font;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------
// texture get and creation
Texture* XRRenderer::GetTextureForFileName(char const* imageFilePath)
{
	for (int i = 0; i < (int)m_loadedTextures.size(); i++)
	{
		if (!strcmp(m_loadedTextures[i]->GetImageFilePath().c_str(), imageFilePath))
		{
			return m_loadedTextures[i];
		}
	}

	return nullptr;
}

Image* XRRenderer::CreateImageFromFile(char const* imageFilePath)
{
	Image* newImage = new Image(imageFilePath);
	return newImage;
}

Texture* XRRenderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	// See if we already have this texture previously loaded
	Texture* existingTexture = GetTextureForFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}
	else
	{
		// Never seen this texture before!  Let's load it.
		Texture* newTexture = CreateTextureFromFile(imageFilePath);
		return newTexture;
	}
}

Texture* XRRenderer::CreateTextureFromFile(char const* imageFilePath)
{
	Image* newImagePtr = CreateImageFromFile(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(*newImagePtr);

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void XRRenderer::CreateAndBindDefaultShader()
{
	// create and bind the default shader
	const char* shaderName = "Default";
	m_defaultShader = CreateShader(shaderName, XRdefaultShaderSource);
	BindShader(m_defaultShader);
}

Shader* XRRenderer::CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType /*= VertexType::Vertex_PCU*/)
{
	// create a new shader config and a shader
	ShaderConfig config;
	config.m_shaderPath = shaderPath;
	Shader* newShader = new Shader(config); // need to "new" to keep it in heap memory, otherwise it will be deleted after the function is excuted

	// compile byteCode and create the shader's vertex shader
	std::vector<unsigned char> vertexShaderByteCode;
	if (!CompileShaderToByteCode(vertexShaderByteCode, "VertexShader", shaderSource, "VertexMain", "vs_5_0"))
	{
		ERROR_AND_DIE(Stringf("Could not compile vertex shader."));
	}
	else
	{
		HRESULT hr;
		hr = m_device->CreateVertexShader(
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			NULL, &newShader->m_vertexShader
		);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create vertex shader."));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------	
	// compile byteCode and create the shader's pixel shader
	std::vector<unsigned char> pixelShaderByteCode;
	if (!CompileShaderToByteCode(pixelShaderByteCode, "PixelShader", shaderSource, "PixelMain", "ps_5_0"))
	{
		ERROR_AND_DIE(Stringf("Could not compile pixel shader."));
	}
	else
	{
		//create pixel shader
		HRESULT hr;
		hr = m_device->CreatePixelShader(
			pixelShaderByteCode.data(),
			pixelShaderByteCode.size(),
			NULL, &newShader->m_pixelShader
		);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create pixel shader."));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// create local array of input element descriptions that defines the vertex layout
	// based on the chosen vertex type, we have different input element description
	HRESULT hr;
	UINT numElements;
	switch (vertexType)
	{
	case VertexType::Vertex_PCU:
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		numElements = ARRAYSIZE(inputElementDesc);
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayoutForVertex);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create Vertex_PCU layout."));
		}
	}break;
	case VertexType::Vertex_PCUTBN:
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		numElements = ARRAYSIZE(inputElementDesc);
		hr = m_device->CreateInputLayout(
			inputElementDesc, numElements,
			vertexShaderByteCode.data(),
			vertexShaderByteCode.size(),
			&newShader->m_inputLayoutForVertex);

		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE(Stringf("Could not create Vertex_PCUTBN layout."));
		}
	}break;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add this new shader to loaded shaders and return it
	m_loadedShaders.push_back(newShader);
	return newShader;

}

void XRRenderer::BindShader(Shader* shader)
{
	// if the new binding shader is nullptr, then use current shader
	if (shader == nullptr)
	{
		shader = m_defaultShader;
		m_currentShader = m_defaultShader;
	}
	else if (m_currentShader == shader)
	{
		return;
	}
	else
	{
		// if the shader do exist, update the current shader info
		m_currentShader = shader;
	}

	m_deviceContext->IASetInputLayout(shader->m_inputLayoutForVertex);
	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);
}

bool XRRenderer::CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name, char const* shaderSource, char const* entryPoint, char const* target)
{
	// Compile vertex shader
	DWORD shaderFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#if defined(ENGINE_DEBUG_RENDER)
	shaderFlags = D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#endif
	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	HRESULT hr;
	hr = D3DCompile(
		shaderSource, strlen(shaderSource),
		name, nullptr, nullptr,
		entryPoint, target, shaderFlags, 0,
		&shaderBlob, &errorBlob
	);

	if (SUCCEEDED(hr))
	{
		outByteCode.resize(shaderBlob->GetBufferSize());
		memcpy(
			outByteCode.data(),
			shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize()
		);
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
			return false;
		}

		// std::string nameOfShader;
		// nameOfShader = std::string(name);
		// customized the error handling to show which kind of shader is not compiling
		// ERROR_AND_DIE(Stringf("Could not compile %s shader", nameOfShader.c_str()));// this way is how to treat C++ style string
		// ERROR_AND_DIE(Stringf("Could not compile %s shader", name));// todo:??? why this also works
	}

	shaderBlob->Release();
	if (errorBlob != NULL)
	{
		errorBlob->Release();
		return false;	// return false when there is an error
	}
	return true;	// return true when the compile is successful

}

void XRRenderer::CreateAndSetRasterizerStates()
{
	// set rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc = {  };// trying to initialize all the variable inside the struct as default

	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.f;
	rasterizerDesc.SlopeScaledDepthBias = 0.f;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.ScissorEnable = false;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = true;
	HRESULT hr;

	// change fill and cull mode based on the rasterizer mode setting and create all the rasterizer state
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::SOLID_CULL_NONE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_NONE state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::WIREFRAME_CULL_NONE)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_NONE state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::WIREFRAME_CULL_BACK)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create WIREFRAME_CULL_BACK state.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::SOLID_CULL_BACK)]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create SOLID_CULL_BACK state.");
	}

	m_deviceContext->RSSetState(m_rasterizerState[(int)(m_rasterizerMode)]);
}

void XRRenderer::CreateDefaultTexture()
{
	Image* defaultImage = new Image(IntVec2(2, 2), Rgba8::WHITE);
	m_defaultTexture = CreateTextureFromImage(*defaultImage);
	m_loadedTextures.push_back(const_cast<Texture*>(m_defaultTexture));
}

Texture* XRRenderer::CreateTextureFromImage(Image const& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_name = image.GetImageFilePath(); // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = image.GetDimensions();

	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData;
	textureData.pSysMem = image.GetRawData();
	textureData.SysMemPitch = 4 * image.GetDimensions().x;

	HRESULT hr;
	hr = m_device->CreateTexture2D(&textureDesc, &textureData, &newTexture->m_texture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateTextureFromImage function failed for image file \"%\".",
			image.GetImageFilePath().c_str()));
	}

	hr = m_device->CreateShaderResourceView(newTexture->m_texture, NULL, &newTexture->m_shaderResourceView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%\".",
			image.GetImageFilePath().c_str()));
	}

	return newTexture;

}

void XRRenderer::BindTexture(Texture const* texture)
{
	// if the texture does not exist, bind it to default texture
	if (!texture)
	{
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, texture->m_textureID);
		m_currentTexture = m_defaultTexture;
		m_deviceContext->PSSetShaderResources(0, 1, &m_currentTexture->m_shaderResourceView);
	}
	// if the binding texture do exist
	else
	{
		// if the new binding texture is the same, no need to bind it again
		if (texture == m_currentTexture)
		{
			return;
		}
		else // new texture don't exist, switch to new texture
		{
			//glDisable(GL_TEXTURE_2D);
			m_currentTexture = texture;
			m_deviceContext->PSSetShaderResources(0, 1, &m_currentTexture->m_shaderResourceView);
		}
	}
}

ConstantBuffer* XRRenderer::CreateConstantBuffer(const size_t size)
{
	// create a local D3D11_BUFFER_DESC variable
	// create vertex buffer
	ConstantBuffer* newBuffer = new ConstantBuffer(size);

	D3D11_BUFFER_DESC bufferDesc = { };
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = (UINT)size;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}

	return newBuffer;
}

void XRRenderer::CreateModelConstantBuffer()
{
	m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));
}

void XRRenderer::CreateViewProjectionConstantBuffer()
{
	m_viewProjectionCBO = CreateConstantBuffer(sizeof(ModelConstants));
}

void XRRenderer::CreateCameraConstantBuffer()
{
	m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
}

//void XRRenderer::SetModelConstants(const Mat44& modelMatrix /*= Mat44()*/, const Rgba8& modelColor /*= Rgba8::WHITE*/)
//{
//	ModelConstants modelInfo;
//	// modelInfo.ModelMatrix = modelMatrix;
//	// modelColor.GetAsFloats(modelInfo.ModelColor);
//	UNUSED(modelMatrix);
//	UNUSED(modelColor);
//	CopyCPUToGPU(&modelInfo, sizeof(modelInfo), m_modelCBO);
//	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
//}

void XRRenderer::SetModelConstants(/*DirectX::XMFLOAT4X4 mat*/ const Mat44& mat)
{
	ModelConstants modelMat;
	modelMat.ModelMatrix = mat;

	CopyCPUToGPU(&modelMat, sizeof(modelMat), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}

void XRRenderer::SetViewProjectionConstants(DirectX::XMFLOAT4X4 mat)
{
	ViewProjectionConstants viewProjectionMat;
	viewProjectionMat.ViewProjectionMatrix = mat;

	CopyCPUToGPU(&viewProjectionMat, sizeof(viewProjectionMat), m_viewProjectionCBO);
	BindConstantBuffer(k_viewProjectionConstantsSlot, m_viewProjectionCBO);
}

void XRRenderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

VertexBuffer* XRRenderer::CreateVertexBuffer(const size_t size, size_t stride /*= sizeof(Vertex_PCU)*/)
{
	VertexBuffer* newBuffer = new VertexBuffer(size, stride);
	UINT vertexBufferSize = (UINT)(size * stride);
	D3D11_BUFFER_DESC bufferDesc = { 0 };// trying to initialize all the variable inside the struct as default
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = vertexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}

	return newBuffer;
}
