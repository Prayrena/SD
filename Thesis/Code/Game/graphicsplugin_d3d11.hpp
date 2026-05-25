// Copyright (c) 2017-2024, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThirdParty/OpenXR/Application/pch.h"
#include "ThirdParty/OpenXR/Application/common.h"
#include "geometry.h"
#include "graphicsplugin.h"
#include "options.h"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/EngineCommon.hpp"

#include <common/xr_linear.h>
#include <DirectXColors.h>
#include <D3Dcompiler.h>

#include <vector>

#include "ThirdParty/OpenXR/Application/d3d_common.h"

class	VertexBuffer;
class   ConstantBuffer;
class   Shader;
class   Texture;
class   Image;

#if defined(OPAQUE)
#undef OPAQUE
#endif

enum class BlendMode
{
	OPAQUE,
	ALPHA,
	ADDITIVE,
	COUNT
};

struct Swapchain
{
	XrSwapchain handle;
	int32_t width;
	int32_t height;
};

enum class VertexType
{
	Vertex_PCU,
	Vertex_PCUTBN,
	COUNT
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT
};

enum class DepthMode
{
	DISABLED,
	ENABLED,
	COUNT
};

using namespace Microsoft::WRL;
using namespace DirectX;

namespace
{
    void InitializeD3D11DeviceForAdapter(IDXGIAdapter1* adapter, const std::vector<D3D_FEATURE_LEVEL>& featureLevels,
        ID3D11Device** device, ID3D11DeviceContext** deviceContext) 
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if !defined(NDEBUG)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        // Create the Direct3D 11 API device object and a corresponding context.
        D3D_DRIVER_TYPE driverType = ((adapter == nullptr) ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN);

    TryAgain:
        HRESULT hr = D3D11CreateDevice(adapter, driverType, 0, creationFlags, featureLevels.data(), (UINT)featureLevels.size(),
            D3D11_SDK_VERSION, device, nullptr, deviceContext);
        if (FAILED(hr)) {
            // If initialization failed, it may be because device debugging isn't supported, so retry without that.
            if ((creationFlags & D3D11_CREATE_DEVICE_DEBUG) && (hr == DXGI_ERROR_SDK_COMPONENT_MISSING)) {
                creationFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
                goto TryAgain;
            }

            // If the initialization still fails, fall back to the WARP device.
            // For more information on WARP, see: http://go.microsoft.com/fwlink/?LinkId=286690
            if (driverType != D3D_DRIVER_TYPE_WARP) {
                driverType = D3D_DRIVER_TYPE_WARP;
                goto TryAgain;
            }
        }
    }

} // namespace

class XRRenderer
{
friend class OpenXrProgram;

public:
    XRRenderer(const std::shared_ptr<Options>& options, std::shared_ptr<IPlatformPlugin>)
        : m_clearColor(options->GetBackgroundClearColor()) {}

    ~XRRenderer() = default;

    std::vector<std::string> GetInstanceExtensions() const { return {XR_KHR_D3D11_ENABLE_EXTENSION_NAME}; }

    void Startup();

    void InitializeDevice(XrInstance instance, XrSystemId systemId);

    void CreateSwapchains(XrSession session, XrInstance m_instance, XrSystemId m_systemId, const std::shared_ptr<const Options> options);

    void InitializeResources();

    int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const ;

    const XrBaseInStructure* GetGraphicsBinding() const {
        return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
    }

    std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(
        uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/) ;

    ComPtr<ID3D11DepthStencilView> GetDepthStencilView(ID3D11Texture2D* colorTexture);

	void BeginCamera(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
		int64_t swapchainFormat);

    void DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray);

	void RenderView(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
		int64_t swapchainFormat, const std::vector<Cube>& cubes) ;

    uint32_t GetSupportedSwapchainSampleCount(const XrViewConfigurationView&)  { return 1; }

    void UpdateOptions(const std::shared_ptr<Options>& options)  { m_clearColor = options->GetBackgroundClearColor(); }

    XMFLOAT4X4 GetViewToRenderMatrix();

    void SetClearScreenColor(const Rgba8& clearColor);

    //----------------------------------------------------------------------------------------------------------------------------------------------------
	// change rendering mode and states
    void        CreateAllBlendStates();
	void		SetBlendMode(BlendMode blendMode);
	void		SetDepthMode(DepthMode depthMode);
	void		SetRasterizerMode(RasterizerMode rasterizerState);

    void SetStatesIfChanged();
	BlendMode		  m_desiredBlendMode = BlendMode::ALPHA;
	ID3D11BlendState* m_currentBlendState = nullptr;
	ID3D11BlendState* m_blendStates[(int)(BlendMode::COUNT)] = {};

    //----------------------------------------------------------------------------------------------------------------------------------------------------
	// depth mode
	void						CreateAndSetDepthStencilStates();
	// stencil depth property
	DepthMode					m_depthMode = DepthMode::ENABLED;
	DepthMode					m_desiredDepthMode = DepthMode::ENABLED;
	ID3D11DepthStencilState* m_depthStencilStates[(int)(DepthMode::COUNT)] = {};
	ID3D11DepthStencilView* m_depthStencilView = nullptr;
	ID3D11Texture2D* m_depthStencilTexture = nullptr;

    //----------------------------------------------------------------------------------------------------------------------------------------------------
	// rasterizer state
	void						CreateAndSetRasterizerStates();

	RasterizerMode				m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	RasterizerMode				m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	ID3D11RasterizerState*      m_rasterizerState[(int)(RasterizerMode::COUNT)];


    //----------------------------------------------------------------------------------------------------------------------------------------------------
	void		CreateDefaultTexture();
    Texture*    CreateTextureFromImage(Image const& image);
    Texture*	GetTextureForFileName(char const* imageFilePath);
	Image*      CreateImageFromFile(char const* imageFilePath);
	void		BindTexture(Texture const* texture);

    std::vector<Texture*>	m_loadedTextures;
	Texture const*          m_defaultTexture = nullptr;
	Texture const*          m_currentTexture = nullptr;

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont* GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension);

    std::vector<BitmapFont*>	m_loadedFonts;

	Texture* CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture* CreateTextureFromFile(char const* imageFilePath);
	BitmapFont* CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension);

    //----------------------------------------------------------------------------------------------------------------------------------------------------
    // shader
    void CreateAndBindDefaultShader();
	Shader* CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType = VertexType::Vertex_PCU);
	void		  BindShader(Shader* shader);
	bool		  CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
		char const* shaderSource, char const* entryPoint, char const* target);

    //----------------------------------------------------------------------------------------------------------------------------------------------------
    ConstantBuffer* CreateConstantBuffer(const size_t size);
	void			CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo);
    void	CreateModelConstantBuffer();
    void    CreateViewProjectionConstantBuffer();
    void    CreateCameraConstantBuffer();
	// void    SetModelConstants(const Mat44& modelMatrix = Mat44(), const Rgba8& modelColor = Rgba8::WHITE);
	void    SetModelConstants(/*DirectX::XMFLOAT4X4 mat*/ const Mat44& modelMatrix = Mat44());
	void    SetViewProjectionConstants(DirectX::XMFLOAT4X4 mat);
	void	BindConstantBuffer(int slot, ConstantBuffer* cbo);


    //----------------------------------------------------------------------------------------------------------------------------------------------------
    VertexBuffer* CreateVertexBuffer(const size_t size, size_t stride = sizeof(Vertex_PCU));
	void CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo);
    void DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset = 0);

    private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_deviceContext;
    XrGraphicsBindingD3D11KHR m_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    std::list<std::vector<XrSwapchainImageD3D11KHR>> m_swapchainImageBuffers;
	ComPtr<ID3D11VertexShader> m_vertexShader;
	ComPtr<ID3D11PixelShader> m_pixelShader;
	ComPtr<ID3D11InputLayout> m_inputLayout;
	ComPtr<ID3D11Buffer> m_modelCBuffer;
	ComPtr<ID3D11Buffer> m_viewProjectionCBuffer;
	ComPtr<ID3D11Buffer> m_cubeVertexBuffer;
	ComPtr<ID3D11Buffer> m_cubeIndexBuffer;

	VertexBuffer* m_immediateVBO = nullptr; // backward compatibility for Libra and Starship
    ConstantBuffer* m_modelCBO = nullptr;
    ConstantBuffer* m_viewProjectionCBO = nullptr; // it is the same for camera CBO
    ConstantBuffer* m_cameraCBO = nullptr;

    std::vector<Shader*> m_loadedShaders;// cache pattern
	Shader* m_currentShader = nullptr;
	Shader* m_defaultShader = nullptr;

    // depth
	// DepthMode					m_depthMode = DepthMode::ENABLED;
	// ID3D11DepthStencilState* m_depthStencilStates[(int)(DepthMode::COUNT)] = {};
	// ID3D11DepthStencilView* m_depthStencilView = nullptr;
	// ID3D11Texture2D* m_depthStencilTexture = nullptr;

    // swapchain
    XrSession m_session{ XR_NULL_HANDLE };
    std::vector<Swapchain> m_swapchains;
    std::vector<XrView> m_views;
    std::vector<XrViewConfigurationView> m_configViews;
    std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
    int64_t m_colorSwapchainFormat{ -1 };

    // Map color buffer to associated depth buffer. This map is populated on demand.
    std::map<ID3D11Texture2D*, ComPtr<ID3D11DepthStencilView>> m_colorToDepthMap;
    std::array<float, 4> m_clearColor;
    Vec4 m_clearScreenColor;
}; 
