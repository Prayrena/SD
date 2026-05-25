#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/core/Image.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/DefaultShader.hpp"
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Math/OpenXRMathUtils.hpp"
#include "ThirdParty/stb/stb_image.h"
#include "Game/EngineBuildPreferences.hpp"

// #define WIN32_LEAN_AND_MEAN  // we set it up in the engine property setting	
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
 
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

const GUID DXGI_DEBUG_ALL = {0xe48ae283, 0xda80, 0x490b, {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8}};

using namespace std;

#if defined(ENGINE_DEBUG_RENDER)
void* m_dxgiDebugModule = nullptr;
void* m_dxgiDebug = nullptr;
#endif

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#if defined(OPAQUE)
#undef OPAQUE
#endif

// extern DevConsole* g_theDevConsole;

//----------------------------------------------------------------------------------------------------------------------------------------------------
const char* defaultShaderSource = R"(
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

	cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

	cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
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

		v2p_t v2p;
		v2p.position = clipPosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		return v2p;
	}

	float4 PixelMain(v2p_t input) : SV_Target0
	{
		float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
		textureColor *= ModelColor;
		textureColor *= input.color;
		clip(textureColor.a - 0.01f);
		return float4(textureColor);
	}

	)";

//----------------------------------------------------------------------------------------------------------------------------------------------------
struct CameraConstants
{
	Mat44 ViewMatrix;
	Mat44 ProjectionMatrix;
};

struct ModelConstants
{
	Mat44 ModelMatrix;
	float ModelColor[4];
};

static const int k_lightingConstantsSlot = 1;
static const int k_PhongLightingConstantsSlot = 1;
static const int k_cameraConstantsSlot = 2;
static const int k_modelConstantsSlot = 3;

Renderer::Renderer(RenderConfig const& config)
	:m_config(config)
{
	
}
 
Renderer::Renderer()
{

}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Startup()
{
#ifndef VR_MODE
	CreateDeviceAndSwapChain();
#else
	
#endif

	GetBackBufferAndCreateRenderTargetView();

	CreateAndBindDefaultShader();
	CreateImmediateVertexBuffer();
	CreateImmediateVertexPCUTBNBuffer();
	CreateCameraConstantBuffer();
	CreateModelConstantBuffer();

	// lighting constant creation for diffuse shader
	CreateLightingConstantBuffer();

	CreatePhongShader();
	// lighting constant creation for Phong shader
	CreatePhongLightingConstantBuffer();

	if (m_config.m_emissiveEnabled)
	{
		CreateBlurDown_BlurUp_CompositeShaders();
		CreateBlurConstantBuffer();
		CreateEmissveAndBluredEmissveTexture();

		CalculateTimesToBlurDown();
		CreateBlurDownAndUpTextures();
	}

	SetModelConstants();

	CreateAllBlendStates();

	CreateAllRasterizerStates();

	CreateStencilTextureAndViewAndAllDepthStencilStates();

	CreateDefaultTexture();
	BindTexture(m_defaultWhiteTexture);

	CreateSamplerState();
	SetSamplerMode(SamplerMode::POINT_CLAMP);

	DetectDXGIMemoryLeak();
}

void Renderer::CreateDeviceAndSwapChain()
{
// render startup
	unsigned int deviceFlags = 0;
#if defined(ENGINE_DEBUG_RENDER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// create device and swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferDesc.Width = m_config.m_window->GetWindowDimensions().x;
	swapChainDesc.BufferDesc.Height = m_config.m_window->GetWindowDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->GetHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, nullptr, 0, D3D11_SDK_VERSION,
		&swapChainDesc, &m_windowsSwapChain, &m_device, nullptr, &m_deviceContext);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create D3D 11 device and swap chain.");
	}
}

void Renderer::GetBackBufferAndCreateRenderTargetView()
{
	HRESULT hr;
	// Get back buffer texture
	ID3D11Texture2D* backBuffer;
	hr = m_windowsSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not get swap chain buffer.")
	}

	hr = m_device->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView);// view is a wrapper 
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not render target view for swap chain buffer.");
	}

	backBuffer->Release();
}

void Renderer::EndFrame()
{
	// double timeAtStart = GetCurrentTimeSeconds();

	// present
	HRESULT hr;
	hr = m_windowsSwapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.");
	}

	// double timeAtEnd = GetCurrentTimeSeconds();
	// double timeElapsed = timeAtEnd - timeAtStart;
	// g_theDevConsole->AddLine(Stringf("RendererEndFrame = %.02f ms", timeElapsed * 1000.0), Rgba8::LIGHT_ORANGE);
}


void Renderer::CreateAndBindDefaultShader()
{
	// create and bind the default shader
	const char* shaderName = "Default";
	m_defaultShader = CreateShader(shaderName, defaultShaderSource);
	BindShader(m_defaultShader);
}

void Renderer::CreateImmediateVertexBuffer()
{
	// create the immediate vertex and specify an initial size big enough for one Vertex_PCU
	size_t vertexSize = sizeof(Vertex_PCU);
	// m_immediateVBO = new VertexBuffer((3 * vertexSize), vertexSize, m_device);
	m_immediateVBO = CreateVertexBuffer(3, vertexSize);
}

void Renderer::CreateImmediateVertexPCUTBNBuffer()
{
	// create the immediate vertex and specify an initial size big enough for one Vertex_PCUTBN
	size_t vertexSize = sizeof(Vertex_PCUTBN);
	m_immediateVertex_PCUTBN_BO = new VertexBuffer(3, vertexSize, m_device); // the smallest will have three verts
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::CreateBlurDown_BlurUp_CompositeShaders()
{
	m_blurDownShader = CreateShader("..\\..\\Engine\\Code\\Engine\\Shaders\\BlurDown");
	m_blurUpShader = CreateShader("..\\..\\Engine\\Code\\Engine\\Shaders\\BlurUp");
	m_compositeShader = CreateShader("..\\..\\Engine\\Code\\Engine\\Shaders\\Composite");
}

// render emissive texture
void Renderer::CreateEmissveAndBluredEmissveTexture()
{
	m_emissiveRenderTexture = CreateRenderTexture(m_config.m_window->GetWindowDimensions(), "FullScreenEmissiveTexture");
	m_emissiveBlurredRenderTexture = CreateRenderTexture(m_config.m_window->GetWindowDimensions(), "FullScreenBlurredEmissiveTexture");
}

void Renderer::RenderEmissive()
{
	std::vector<Vertex_PCU> screenVerts;
	// Normalized Device Coordinates (NDC):
	// In NDC space, the screen coordinates are normalized to a range between - 1 and 1, where:
	// X ranges from - 1 to + 1 horizontally(left to right)
	// Y ranges from - 1 to + 1 vertically(bottom to top)
	// Z ranges from 0 to 1 (depth value)
	AddVertsForAABB2D(screenVerts, AABB2(Vec2(-1.f, 1.f), Vec2(1.f, -1.f)), Rgba8::BLACK);

	BlurConstants blurDownConstants = GetDefaultBlurDownConstants();

	// For each blur down texture, in order from largest to smallest. 
	for (int i = 0; i < (int)m_blurDownTextures.size(); i++)
	{
		//	For the largest blur down texture, when there is no next larger texture, use the full-size emissive texture to set shader resources - bind texture
		if (i == 0)
		{
			m_deviceContext->OMSetRenderTargets(1, &m_blurDownTextures[0]->m_renderTargetView, nullptr); // draw on this blur down texture

			BindTexture(m_emissiveRenderTexture); // use shader resources

			// Calculate and set the blur constants texel sizes based on the selected shader resource texture size
			blurDownConstants.TexelSize = Vec2( 1.f / m_emissiveRenderTexture->GetDimensions().x, 1.f / m_emissiveRenderTexture->GetDimensions().y );

		}
		else
		{
			m_deviceContext->OMSetRenderTargets(1, &m_blurDownTextures[i]->m_renderTargetView, nullptr); // Set the render target to be the current blur down texture

			BindTexture(m_blurDownTextures[i - 1]); // Set the shader resources to be the previous larger blur down texture

			// Calculate and set the blur constants texel sizes based on the selected shader resource texture size
			blurDownConstants.TexelSize = Vec2(1.f / m_blurDownTextures[i - 1]->GetDimensions().x, 1.f / m_blurDownTextures[i - 1]->GetDimensions().y);
		}

		SetBlurConstantBuffer(blurDownConstants);

		// Set the viewport to the size of the current blur down texture
		// Get the texture description
		D3D11_TEXTURE2D_DESC textureDesc;
		m_blurDownTextures[i]->m_texture->GetDesc(&textureDesc);

		// Define the viewport
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<FLOAT>(textureDesc.Width);
		viewport.Height = static_cast<FLOAT>(textureDesc.Height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// Set the viewport
		m_deviceContext->RSSetViewports(1, &viewport);

		BindShader(m_blurDownShader);

		SetModelConstants();
		SetDepthMode(DepthMode::DISABLED);
		SetBlendMode(BlendMode::OPAQUE);
		SetSamplerMode(SamplerMode::BILINEAR_CLAMP);
		SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);

		DrawVertexArray((int)screenVerts.size(), screenVerts.data());
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// in order from smallest to largest
	for (int j = 0; j < (int)m_blurUpTextures.size(); ++j)
	{
		BlurConstants blurUpConstant = GetDefaultBlurUpConstants();

		// Set the render target to be the current blur up texture
		m_deviceContext->OMSetRenderTargets(1, &m_blurUpTextures[j]->m_renderTargetView, nullptr);

		// Set the shader resources
		// Set texture slot 0 to the same size blur down texture 
		BindTextureToSpecificSlot(m_blurDownTextures[m_blurDownTimes - 2 - j], 0);
		// Set texture slot 1 to the next smaller blur up texture
		if (j == 0)
		{
			// For the smallest blur up texture, when there is no smaller, use the smallest blur down texture
			Texture*& smallestBlurredDownTexture = m_blurDownTextures.back();
			BindTextureToSpecificSlot(smallestBlurredDownTexture, 1);

			// Calculate and set the blur constants texel sizes based on the size of the texture bound to slot 1
			blurUpConstant.TexelSize = Vec2(1.f / smallestBlurredDownTexture->GetDimensions().x, 1.f / smallestBlurredDownTexture->GetDimensions().y);
		}
		else
		{
			BindTextureToSpecificSlot(m_blurUpTextures[j - 1], 1);

			// Calculate and set the blur constants texel sizes based on the size of the texture bound to slot 1
			blurUpConstant.TexelSize = Vec2(1.f / m_blurUpTextures[j - 1]->GetDimensions().x, 1.f / m_blurUpTextures[j - 1]->GetDimensions().y);
		}

		SetBlurConstantBuffer(blurUpConstant);

		// Set the viewport to the size of the current blur down texture
		// Get the texture description
		D3D11_TEXTURE2D_DESC textureDesc;
		m_blurUpTextures[j]->m_texture->GetDesc(&textureDesc);

		// Define the viewport
		D3D11_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = static_cast<FLOAT>(textureDesc.Width);
		viewport.Height = static_cast<FLOAT>(textureDesc.Height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// Set the viewport
		m_deviceContext->RSSetViewports(1, &viewport);

		SetModelConstants();
		SetDepthMode(DepthMode::DISABLED);
		SetBlendMode(BlendMode::OPAQUE);
		SetSamplerMode(SamplerMode::BILINEAR_CLAMP);
		SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);

		BindShader(m_blurUpShader);
		DrawVertexArray((int)screenVerts.size(), screenVerts.data());
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	//  Then just one time, repeat the above steps but
	// 	Replace the blur down texture with the full - size emissive texture.
	BindTextureToSpecificSlot(m_emissiveRenderTexture, 0);
	// 	Replace the same size blur down texture with the full - size blurred emissive texture -- writing to the largest one
	m_deviceContext->OMSetRenderTargets(1, &m_emissiveBlurredRenderTexture->m_renderTargetView, nullptr);
	// 	Replace the next smaller blur up texture with the largest blur up texture.
	BindTextureToSpecificSlot(m_blurUpTextures.back(), 1);

	BlurConstants blurUpConstant = GetDefaultBlurUpConstants();
	blurUpConstant.TexelSize = Vec2(1.f / m_blurUpTextures.back()->GetDimensions().x, 1.f / m_blurUpTextures.back()->GetDimensions().y);
	SetBlurConstantBuffer(blurUpConstant);

	// Set the viewport to the size of the current blur down texture
	// Get the texture description
	D3D11_TEXTURE2D_DESC textureDesc;
	m_emissiveBlurredRenderTexture->m_texture->GetDesc(&textureDesc);

	// Define the viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(textureDesc.Width);
	viewport.Height = static_cast<FLOAT>(textureDesc.Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Set the viewport
	m_deviceContext->RSSetViewports(1, &viewport);

	SetModelConstants();
	SetDepthMode(DepthMode::DISABLED);
	SetBlendMode(BlendMode::OPAQUE);
	SetSamplerMode(SamplerMode::BILINEAR_CLAMP);
	SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);

	BindShader(m_blurUpShader);
	DrawVertexArray((int)screenVerts.size(), screenVerts.data());

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Composite
	// Bind the back buffer as the render target
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	// Bind the blurred emissive texture as a shader resource
	BindTexture(m_emissiveBlurredRenderTexture);

	SetBlendMode(BlendMode::ADDITIVE);
	BindShader(m_compositeShader);
	SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);
	DrawVertexArray((int)screenVerts.size(), screenVerts.data());
}

Texture* Renderer::CreateRenderTexture(IntVec2 const& dimensions, const char* name)
{
	// create bloom textures
	D3D11_TEXTURE2D_DESC renderTextureDesc = {};
	renderTextureDesc.Width = dimensions.x;
	renderTextureDesc.Height = dimensions.y;
	renderTextureDesc.MipLevels = 1;
	renderTextureDesc.ArraySize = 1;
	renderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	renderTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	renderTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	renderTextureDesc.SampleDesc.Count = 1;

	ID3D11Texture2D* d3dTexture = nullptr;
	HRESULT hr = m_device->CreateTexture2D(&renderTextureDesc, nullptr, &d3dTexture);
	if (FAILED(hr)) 
	{
		ERROR_AND_DIE("Failed to create render texture");
	}

	ID3D11ShaderResourceView* d3dShaderResourceView = nullptr;
	hr = m_device->CreateShaderResourceView(d3dTexture, NULL, &d3dShaderResourceView);
	if (FAILED(hr))
	{
		ERROR_AND_DIE("Failed to create render texture");
	}

	ID3D11RenderTargetView* d3dRenderTargetView = nullptr;
	hr = m_device->CreateRenderTargetView(d3dTexture, NULL, &d3dRenderTargetView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Failed to create render target view");
	}

	Texture* renderTexture = new Texture(name, dimensions, d3dTexture, d3dShaderResourceView, d3dRenderTargetView);
	return renderTexture;
}

void Renderer::BindTextureToSpecificSlot(const Texture* texture, unsigned int slot /*= 0*/)
{
	if (!texture)
	{
		if (slot !=0 )
		{
			m_currentTexture = m_defaultBlackTexture;
		}
		else
		{
			m_currentTexture = m_defaultWhiteTexture;
		}
		m_deviceContext->PSSetShaderResources(slot, 1, &m_currentTexture->m_shaderResourceView);
	}
	else
	{
		m_deviceContext->PSSetShaderResources(slot, 1, &texture->m_shaderResourceView);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::CreateStencilTextureAndViewAndAllDepthStencilStates()
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = m_config.m_window->GetWindowDimensions().x;
	textureDesc.Height = m_config.m_window->GetWindowDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.SampleDesc.Count = 1;

	HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &m_depthStencilTexture);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create texture for depth stencil.");
	}

	hr = m_device->CreateDepthStencilView(m_depthStencilTexture, nullptr, &m_depthStencilView);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create depth stencil view.");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;

	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::DISABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Disable failed");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::ENABLED]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::Enabled failed");
	}	
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateDepthStencilState for DepthMode::READ_ONLY_LESS_EQUAL failed");
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	//  Don't write to the depth buffer when rendering the shadow.
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.StencilEnable = TRUE;
	depthStencilDesc.StencilReadMask = 0xff;
	depthStencilDesc.StencilWriteMask = 0xff;

	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[(int)DepthMode::READ_ONLY_LESS_EQUAL_STENCIL_ONCE]);

	UINT stencilRef = 0xffffffff;
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_depthMode], stencilRef);
}

void Renderer::CreatePhongShader()
{
	m_PhongShader = CreateShader("..\\..\\Engine\\Code\\Engine\\Shaders\\Phong", VertexType::Vertex_PCUTBN);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef VR_MODE
void Renderer::InitializeD3D11DeviceForAdapter(IDXGIAdapter1* adapter, const std::vector<D3D_FEATURE_LEVEL>& featureLevels, ID3D11Device** device, ID3D11DeviceContext** deviceContext)
{
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
}

void Renderer::XRBeginFrame(XrSession session)
{
	XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO };
	XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	CHECK_XRCMD(xrWaitFrame(session, &frameWaitInfo, &frameState));

	XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
	CHECK_XRCMD(xrBeginFrame(session, &frameBeginInfo));
}

void Renderer::InitializeDevice(XrInstance instance, XrSystemId systemId)
{
	PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11GraphicsRequirementsKHR = nullptr;
	CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",
		reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11GraphicsRequirementsKHR)));

	// Create the D3D11 device for the adapter associated with the system.
	XrGraphicsRequirementsD3D11KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	CHECK_XRCMD(pfnGetD3D11GraphicsRequirementsKHR(instance, systemId, &graphicsRequirements));
	const ComPtr<IDXGIAdapter1> adapter = GetAdapter(graphicsRequirements.adapterLuid);

	// Create a list of feature levels which are both supported by the OpenXR runtime and this application.
	std::vector<D3D_FEATURE_LEVEL> featureLevels = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
													D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	featureLevels.erase(std::remove_if(featureLevels.begin(), featureLevels.end(),
		[&](D3D_FEATURE_LEVEL fl) { return fl < graphicsRequirements.minFeatureLevel; }),
		featureLevels.end());
	CHECK_MSG(featureLevels.size() != 0, "Unsupported minimum feature level!");

	if (m_device)
	{
		m_device->Release();
	}	
	if (m_deviceContext)
	{
		m_deviceContext->Release();
	}
	InitializeD3D11DeviceForAdapter(adapter.Get(), featureLevels, &m_device,
		&m_deviceContext);

	m_graphicsBinding.device = m_device;
}

Microsoft::WRL::ComPtr<IDXGIAdapter1> Renderer::GetAdapter(LUID adapterId)
{
	// Create the DXGI factory.
	ComPtr<IDXGIFactory1> dxgiFactory;
	CHECK_HRCMD(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory.ReleaseAndGetAddressOf())));

	for (UINT adapterIndex = 0;; adapterIndex++) {
		// EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters to enumerate.
		ComPtr<IDXGIAdapter1> dxgiAdapter;
		CHECK_HRCMD(dxgiFactory->EnumAdapters1(adapterIndex, dxgiAdapter.ReleaseAndGetAddressOf()));

		DXGI_ADAPTER_DESC1 adapterDesc;
		CHECK_HRCMD(dxgiAdapter->GetDesc1(&adapterDesc));
		if (memcmp(&adapterDesc.AdapterLuid, &adapterId, sizeof(adapterId)) == 0) {
			Log::Write(Log::Level::Verbose, Fmt("Using graphics adapter %ws", adapterDesc.Description));
			return dxgiAdapter;
		}
	}
}

void Renderer::RenderXRSwapchainImageToWindow(const XrSwapchainImageBaseHeader* swapchainImage, Camera* camera)
{
	// the texture just draw for VR
	ID3D11Texture2D* const openXRTexture = reinterpret_cast<const XrSwapchainImageD3D11KHR*>(swapchainImage)->texture;

	if (!m_XRCopiedTexture)
	{
		// Get the description of the existing texture
		D3D11_TEXTURE2D_DESC openXRTextureDesc;
		openXRTexture->GetDesc(&openXRTextureDesc);

		// Define the new texture's description
		D3D11_TEXTURE2D_DESC renderTextureDesc = {};
		renderTextureDesc.Width = openXRTextureDesc.Width;
		renderTextureDesc.Height = openXRTextureDesc.Height;
		renderTextureDesc.MipLevels = 1; // No mipmaps
		renderTextureDesc.ArraySize = 1;
		renderTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		renderTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // New format (optional)
		renderTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		renderTextureDesc.SampleDesc.Count = 1;
		renderTextureDesc.SampleDesc.Quality = 0;
		renderTextureDesc.CPUAccessFlags = 0;
		renderTextureDesc.MiscFlags = 0;
	
		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// create new texture - do not to create one every frame but reuse it
		ID3D11Texture2D* d3dTexture = nullptr;
		HRESULT hr = m_device->CreateTexture2D(&renderTextureDesc, nullptr, &d3dTexture);
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Failed to create render texture");
		}

		ID3D11ShaderResourceView* d3dShaderResourceView = nullptr;
		hr = m_device->CreateShaderResourceView(d3dTexture, NULL, &d3dShaderResourceView);
		if (FAILED(hr))
		{
			ERROR_AND_DIE("Failed to create render texture");
		}

		ID3D11RenderTargetView* d3dRenderTargetView = nullptr;
		hr = m_device->CreateRenderTargetView(d3dTexture, NULL, &d3dRenderTargetView);
		if (!SUCCEEDED(hr))
		{
			ERROR_AND_DIE("Failed to create render target view");
		}

		IntVec2 openXRImageDimensions((int)openXRTextureDesc.Width, (int)openXRTextureDesc.Height);
		m_XRCopiedTexture = CreateRenderTexture(openXRImageDimensions, "OpenXRImage");

		m_XRCopiedTexture->m_texture = d3dTexture;
		m_XRCopiedTexture->m_shaderResourceView = d3dShaderResourceView;
		m_XRCopiedTexture->m_renderTargetView = d3dRenderTargetView;
	}

	// Copy the contents from the existing texture to the new texture
	m_deviceContext->CopyResource(m_XRCopiedTexture->m_texture, openXRTexture);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	std::vector<Vertex_PCU> screenVerts;
	AddVertsForAABB2D(screenVerts, AABB2(Vec2(0.f, camera->GetCameraBounds().m_maxs.y), Vec2(camera->GetCameraBounds().m_maxs.x, 0.f)), Rgba8::WHITE);

	// Bind the back buffer as the render target
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	// Define the viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	// viewport.Width = static_cast<FLOAT>(m_XRCopiedTexture->m_dimensions.x);
	// viewport.Height = static_cast<FLOAT>(m_XRCopiedTexture->m_dimensions.y);
	viewport.Width = (float)m_config.m_window->GetWindowDimensions().x;
	viewport.Height = (float)m_config.m_window->GetWindowDimensions().y;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// viewport.TopLeftX = 0.f;
	// viewport.TopLeftY = 0.f;

	// Set the viewport
	m_deviceContext->RSSetViewports(1, &viewport);

	// Bind the texture to the screen verts
	BindTexture(m_XRCopiedTexture);
	SetDepthMode(DepthMode::DISABLED);
	SetBlendMode(BlendMode::ALPHA);
	SetModelConstants();
	SetRasterizerMode(RasterizerMode::SOLID_CULL_FRONT);
	BindShader(nullptr);
	DrawVertexArray((int)screenVerts.size(), screenVerts.data());
}

// Create a Swapchain which requires coordinating with the graphics plugin to select the format, getting the system graphics
// properties, getting the view configuration and grabbing the resulting swapchain images.
void Renderer::CreateSwapchains(XrSession XRSession, XrInstance XRInstance, XrSystemId XRSystemId, const std::shared_ptr<const Options> programOptions)
{
	CHECK(XRSession != XR_NULL_HANDLE);
	CHECK(m_XRSwapchains.empty());
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

			m_XRSwapchains.push_back(swapchain);

			uint32_t imageCount;
			CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
			// XXX This should really just return XrSwapchainImageBaseHeader*
			std::vector<XrSwapchainImageBaseHeader*> swapchainImages = AllocateSwapchainImageStructs(imageCount, swapchainCreateInfo);
			CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]));

			m_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// create swapchain for windows
	// render startup
//	unsigned int deviceFlags = 0;
//#if defined(ENGINE_DEBUG_RENDER)
//	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

	// create device and swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferDesc.Width = m_config.m_window->GetWindowDimensions().x;
	swapChainDesc.BufferDesc.Height = m_config.m_window->GetWindowDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->GetHwnd();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	HRESULT hr;
	// Step 1: Query for the IDXGIDevice interface from the D3D11 device
	IDXGIDevice* dxgiDevice = nullptr;
	hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create dxgiDevice.");
	}

	// Step 2: Get the adapter (IDXGIAdapter) associated with the device
	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
	hr = dxgiDevice->GetAdapter(&dxgiAdapter);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create dxgiAdapter.");
	}

	// Step 3: Get the IDXGIFactory from the adapter
	Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
	hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create dxgiFactory.");
	}

	hr = dxgiFactory->CreateSwapChain(m_device, &swapChainDesc, &m_windowsSwapChain);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create swap chain.");
	}
}

int64_t Renderer::SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const
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

	return *swapchainFormatIt; // OpneXR return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB as result
}

std::vector<XrSwapchainImageBaseHeader*> Renderer::AllocateSwapchainImageStructs(uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/)
{
	// Allocate and initialize the buffer of image structs (must be sequential in memory for xrEnumerateSwapchainImages).
	// Return back an array of pointers to each swapchain image struct so the consumer doesn't need to know the type/size.
	std::vector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(capacity, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
	std::vector<XrSwapchainImageBaseHeader*> swapchainImageBase;
	for (XrSwapchainImageD3D11KHR& image : swapchainImageBuffer)
	{
		swapchainImageBase.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image));
	}

	// Keep the buffer alive by moving it into the list of buffers.
	m_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));

	return swapchainImageBase;
}

const XrBaseInStructure* Renderer::GetGraphicsBinding() const
{
	return reinterpret_cast<const XrBaseInStructure*>(&m_graphicsBinding);
}

ComPtr<ID3D11DepthStencilView> Renderer::GetDepthStencilView(ID3D11Texture2D* colorTexture)
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
	// depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// depthDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.SampleDesc.Count = 1;
	ComPtr<ID3D11Texture2D> depthTexture;
	CHECK_HRCMD(m_device->CreateTexture2D(&depthDesc, nullptr, depthTexture.ReleaseAndGetAddressOf()));

	// Create and cache the depth stencil view.
	ComPtr<ID3D11DepthStencilView> depthStencilView;
	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);

	// D3D11_TEXTURE2D_DESC textureDesc = {};
	// textureDesc.Width = m_config.m_window->GetWindowDimensions().x;
	// textureDesc.Height = m_config.m_window->GetWindowDimensions().y;
	// textureDesc.MipLevels = 1;
	// textureDesc.ArraySize = 1;
	// textureDesc.Usage = D3D11_USAGE_DEFAULT;
	// textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// // textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	// textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	// textureDesc.SampleDesc.Count = 1;

	CHECK_HRCMD(m_device->CreateDepthStencilView(depthTexture.Get(), nullptr, depthStencilView.GetAddressOf()));
	depthBufferIt = m_colorToDepthMap.insert(std::make_pair(colorTexture, depthStencilView)).first;

	return depthStencilView;
}

void Renderer::SetClearScreenColor(const Rgba8& clearColor)
{
	//dx11-clear the screen
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_clearScreenColor = ConvertRGBAToVec4(clearColor);
}

void Renderer::BeginOpenXRCamera(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage, int64_t swapchainFormat, uint32_t eyeIndex)
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
	CHECK_HRCMD(m_device->CreateRenderTargetView(colorTexture, &renderTargetViewDesc, renderTargetView.ReleaseAndGetAddressOf()));

	const ComPtr<ID3D11DepthStencilView> depthStencilView = GetDepthStencilView(colorTexture);

	// Clear swapchain and depth buffer. NOTE: This will clear the entire render target view, not just the specified view.
	m_deviceContext->ClearRenderTargetView(renderTargetView.Get(), static_cast<const FLOAT*>(&m_clearScreenColor.x));
	m_deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	ID3D11RenderTargetView* renderTargets[] = { renderTargetView.Get() };
	m_deviceContext->OMSetRenderTargets((UINT)ArraySize(renderTargets), renderTargets, depthStencilView.Get());

	// set DepthStencilState after setting targets
	UINT stencilRef = 0;
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)DepthMode::ENABLED], stencilRef);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// view matrix
	Mat44 openXRToGameMat = GetOpenXRToGameMat(); // convert openXr game coordinate to our game coordinate
	// Mat44 transformMat(layerView.pose); // layerView.pose is the pose of the eye, in OpenXR space

	Vec3 eyePosInOpenXR(layerView.pose.position);
	Mat44 translationMat(eyePosInOpenXR);
	// translationMat = openXRToGameMat.MatMultiply(translationMat);
	// translationMat = openXRToGameMat.MatMultiply(translationMat);
	
	Quat quat(layerView.pose.orientation);
	// Mat44 renderMat = GetGameToDirectXMat();
	// Mat44 renderMat = GetDirectXToGameMat();
	// Mat44 renderMat = GetGameToOpenXRMat();
	// quat = renderMat.TransformQuaternion(quat);
	Mat44 rotationMat(quat);
	// Mat44 rotationMat(layerView.pose.orientation);
	// rotationMat = openXRToGameMat.MatMultiply(rotationMat);

	// rotationMat = openXRToGameMat.SimilarityTransformation(rotationMat);

	// rotationMat = openXRToGameMat.MatMultiply(rotationMat);
	// rotationMat = rotationMat.MatMultiply(m_headOffsetQuat.GetAsMatrix_XFwd_YLeft_ZUp());
	Mat44 transformMat = translationMat.MatMultiply(rotationMat);
	transformMat = openXRToGameMat.MatMultiply(transformMat);

	EulerAngles originalRotation = transformMat.GetEulerAngles();
	originalRotation += m_headOffsetOrientation;
	Mat44 fullRotationMat = originalRotation.GetAsMatrix_XFwd_YLeft_ZUp();
	Vec3 eyePosInGame = transformMat.GetTranslation3D();
	eyePosInGame = m_headOffsetOrientation.GetAsMatrix_XFwd_YLeft_ZUp().TransformVectorQuantity3D(eyePosInGame) + m_headOffsetPos;
	translationMat = Mat44(eyePosInGame);


	transformMat = translationMat.MatMultiply(fullRotationMat);

	m_VRCameraMatrix[eyeIndex] = transformMat;

	// Mat44 headOffsetQuatMat(m_headOffsetQuat);

	Mat44 worldToViewMat = transformMat.GetOrthonormalInverse();
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// projection matrix
	XrMatrix4x4f projectionMatrix;
	XrMatrix4x4f_CreateProjectionFov(&projectionMatrix, GRAPHICS_D3D, layerView.fov, 0.05f, 100.0f);
	Mat44 projectionMat(projectionMatrix);

	// Mat44 directXMat = GetGameToDirectXMat();
	// Mat44 directXMat = GetDirectXToGameMat();
	// Mat44 directXMat = GetGameToOpenXRMat();
	// projectionMat = directXMat.MatMultiply(projectionMat);

	// Mat44 projectionMat = Mat44::CreatePerspectiveProjection(90.f, 1744.f / 1920.f, 0.05f, 100.f); 
	// projectionMat = projectionMat.MatMultiply(directXMat);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Set shaders and constant buffers.
	CameraConstants cameraInfo;
	cameraInfo.ProjectionMatrix = projectionMat;
	cameraInfo.ViewMatrix = worldToViewMat;

	CopyCPUToGPU(&cameraInfo, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(k_cameraConstantsSlot, m_cameraCBO);
}

// Function to retrieve the XR Swapchain texture
ID3D11Texture2D* Renderer::GetXRSwapchainTexture(XrSwapchain handle)
{
	XrSwapchainImageD3D11KHR* swapchainImages;
	uint32_t imageCount;

	// Query the number of images in the swapchain
	xrEnumerateSwapchainImages(handle, 0, &imageCount, nullptr);

	// Allocate memory for the swapchain images
	swapchainImages = (XrSwapchainImageD3D11KHR*)malloc(sizeof(XrSwapchainImageD3D11KHR) * imageCount);

	// Populate the swapchain images structure
	for (uint32_t i = 0; i < imageCount; i++)
	{
		swapchainImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR };
	}

	// Get the images from the swapchain
	xrEnumerateSwapchainImages(handle, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages);

	// Get the current image index
	uint32_t imageIndex = 0;
	XrResult result = xrAcquireSwapchainImage(handle, nullptr, &imageIndex);
	if (!XR_SUCCEEDED(result))
	{
		// Wait until the image is available to write to
		// XrSwapchainImageWaitInfo waitInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		// waitInfo.timeout = XR_INFINITE_DURATION;
		// xrWaitSwapchainImage(handle, &waitInfo);
		ERROR_AND_DIE("Cannot acquire XR swap chain image");
	}

	// Return the texture associated with the current image
	ID3D11Texture2D* xrTexture = swapchainImages[imageIndex].texture;

	// Free the swapchain images memory
	free(swapchainImages);

	xrReleaseSwapchainImage(handle, nullptr);

	return xrTexture;
}

void Renderer::EndFrameFromXRtoWindows()
{
	// present
	HRESULT hr;
	hr = m_windowsSwapChain->Present(0, 0);
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		ERROR_AND_DIE("Device has been lost, application will now terminate.");
	}
}

void Renderer::OpenXRShutDown()
{
	// Delete m_XRCopiedTexture
	if (m_XRCopiedTexture) 
	{
		delete m_XRCopiedTexture;
		m_XRCopiedTexture = nullptr;
	}

	// release and clear m_layers
	for (auto* layer : m_layers) 
	{
		delete layer;
	}
	m_layers.clear();

	// clear color to depth map
	m_colorToDepthMap.clear(); // ComPtr auto-releases

	// clear swapchain image buffers
	m_swapchainImageBuffers.clear();

	// clear swapchains
	for (auto& swapchain : m_XRSwapchains)
	{
		xrDestroySwapchain(swapchain.handle);
	}
	m_XRSwapchains.clear();

	// clear views
	m_views.clear();
	m_configViews.clear();

	// delete swapchain images
	// for (auto& pair : m_swapchainImages)
	// {
	// 	for (auto* image : pair.second)
	// 	{
	// 		delete image;
	// 	}
	// }
	// m_swapchainImages.clear();
}

#endif

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::Shutdown()
{
	m_deviceContext->ClearState();
	m_deviceContext->Flush();

	// release all the shaders
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		delete m_loadedShaders[shaderIndex];
	}

	// release all blend states
	for (int i = 0; i < (int)BlendMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_blendStates[i]);
	}

	// release all the sampler mode
	for (int i = 0; i < (int)SamplerMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_samplerStates[i]);
	}

	// release all the rasterizer states
	for (int i = 0; i < (int)(RasterizerMode::COUNT); ++i)
	{
		DX_SAFE_RELEASE(m_rasterizerState[i]);
	}

	// release all depth stencil states
	for (int i = 0; i < (int)DepthMode::COUNT; ++i)
	{
		DX_SAFE_RELEASE(m_depthStencilStates[i]);
	}
	
	// delete all the textures
	for (int i = 0; i < (int)m_loadedTextures.size(); ++i)
	{
		delete(m_loadedTextures[i]);
	}

	// emissive textures
	if (m_config.m_emissiveEnabled)
	{
		delete m_emissiveRenderTexture;
		m_emissiveRenderTexture = nullptr;
		delete m_emissiveBlurredRenderTexture;
		m_emissiveBlurredRenderTexture = nullptr;

		for (int i = 0; i < (int)m_blurDownTextures.size(); ++i)
		{
			if (m_blurDownTextures[i])
			{
				delete m_blurDownTextures[i];
			}
		}
		for (int j = 0; j < (int)m_blurUpTextures.size(); ++j)
		{
			if (m_blurUpTextures[j])
			{
				delete m_blurUpTextures[j];
			}
		}

		delete m_blurCBO;
		m_blurCBO = nullptr;
	}

	// delete all the fonts
	for (int i = 0; i < (int)m_loadedFonts.size(); ++i)
	{
		delete(m_loadedFonts[i]);
	}

	// release the constant buffers
	delete m_cameraCBO;
	m_cameraCBO = nullptr;

	delete m_modelCBO;
	m_modelCBO = nullptr;

	// delete the immediate buffer
	delete m_immediateVBO;
	m_immediateVBO = nullptr;
	delete m_immediateVertex_PCUTBN_BO;
	m_immediateVertex_PCUTBN_BO = nullptr;

	delete m_lightingCBO;
	m_lightingCBO = nullptr;

	delete m_PhongLightingCBO;
	m_PhongLightingCBO = nullptr;

	DX_SAFE_RELEASE(m_windowsSwapChain);
	// for (int i = 0; i < (int)m_XRSwapchains.size(); ++i)
	// {
	// 	if (m_XRSwapchains[i].handle != XR_NULL_HANDLE) 
	// 	{
	// 		// Destroy the swapchain handle
	// 		xrDestroySwapchain(m_XRSwapchains[i].handle);
	// 		// Reset the handle to null to prevent dangling references
	// 		m_XRSwapchains[i].handle = XR_NULL_HANDLE;
	// 	}
	// }
	DX_SAFE_RELEASE(m_renderTargetView);
	DX_SAFE_RELEASE(m_deviceContext);
	DX_SAFE_RELEASE(m_device);
	DX_SAFE_RELEASE(m_depthStencilView);
	DX_SAFE_RELEASE(m_depthStencilTexture);

	# ifdef VR_MODE
	OpenXRShutDown();
	#endif

	ReportErrorLeaksAndReleaseDebugModule();
}

void Renderer::ReportErrorLeaksAndReleaseDebugModule()
{
	// Report error leaks and release debug module
#if defined(ENGINE_DEBUG_RENDER)
	// for void*, does static cast and (point type) method have difference
	((IDXGIDebug*)m_dxgiDebug)->ReportLiveObjects(
		DXGI_DEBUG_ALL,
		(DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL)
	);

	((IDXGIDebug*)m_dxgiDebug)->Release();
	m_dxgiDebug = nullptr;

	::FreeLibrary((HMODULE)m_dxgiDebugModule);
	m_dxgiDebugModule = nullptr;
#endif
}

void Renderer::BeginFrame()
{
	if (m_config.m_emissiveEnabled)
	{
		BeginFrame_BindingRTVsAndDepthStencilView();
	}
	else
	{
		// set render target
		// m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
		// set render target and set depth stencil view
		m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);
	}
}
  
void Renderer::BeginFrame_BindingRTVsAndDepthStencilView()
{
	// Change OMSetRenderTargets to bind the emissive render texture in addition to the back buffer
	ID3D11RenderTargetView* RTVs[] =
	{
		m_renderTargetView,
		m_emissiveRenderTexture->m_renderTargetView
	};
	m_deviceContext->OMSetRenderTargets(2, RTVs, m_depthStencilView);
}

//void Renderer::CreateRenderingContext()
//{
//	// Creates an OpenGL rendering context (RC) and binds it to the current window's device context (DC)
//	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
//	memset(&pixelFormatDescriptor, 0, sizeof(pixelFormatDescriptor));
//	pixelFormatDescriptor.nSize = sizeof(pixelFormatDescriptor);
//	pixelFormatDescriptor.nVersion = 1;
//	pixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
//	pixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
//	pixelFormatDescriptor.cColorBits = 24; // 8 bits each for R, G, B
//	pixelFormatDescriptor.cDepthBits = 24; // 24 bits of precision for our
//	pixelFormatDescriptor.cAccumBits = 0;
//	pixelFormatDescriptor.cStencilBits = 8;
//
//	// Create an OpenGL rendering context (RC) and bind it to the current window's device context ( DC )
//	HDC dc = reinterpret_cast<HDC>(m_config.m_window->GetDeviceContext()); // transform the m_dc type
//	int pixelFormatCode = ChoosePixelFormat( dc, &pixelFormatDescriptor);
//	SetPixelFormat(dc, pixelFormatCode, &pixelFormatDescriptor);
//	HGLRC hglrc = wglCreateContext(dc); // Create a new OpenGL bound to this display context
//	wglMakeCurrent(dc, hglrc);
//	m_rc = hglrc; // This is an OpenGL "Rendering Context" (RC) which means "an instance of OpenGL"???????????????????????????????????????????????????????????????????????????
//}

void Renderer::ClearScreen(Rgba8 const& clearColor)
{
	// transform the input color variable from char to float
	// float r = float(clearColor.r) / 255.f;
	// float g = float(clearColor.g) / 255.f;
	// float b = float(clearColor.b) / 255.f;
	// float a = float(clearColor.a) / 255.f;

	// because gl take in float
	//glClearColor(r, g, b, a);
	//glClear(GL_COLOR_BUFFER_BIT);
	 
	//dx11-clear the screen
	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, colorAsFloats);

	// clear the depth stencil view
	m_deviceContext->ClearDepthStencilView(m_depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	if (m_config.m_emissiveEnabled)
	{
		// Clear all the emissive render textures in addition to the back buffer
		float blackAsFloats[4] = {0.f, 0.f, 0.f, 1.f};
		m_deviceContext->ClearRenderTargetView(m_emissiveRenderTexture->m_renderTargetView, blackAsFloats);
		m_deviceContext->ClearRenderTargetView(m_emissiveBlurredRenderTexture->m_renderTargetView, blackAsFloats);

		// Clear all emissive render textures to opaque black
		for (int i = 0; i < (int)m_blurDownTextures.size(); ++i)
		{
			m_deviceContext->ClearRenderTargetView(m_blurDownTextures[i]->m_renderTargetView, blackAsFloats);
		}	
		for (int j = 0; j < (int)m_blurUpTextures.size(); ++j)
		{
			m_deviceContext->ClearRenderTargetView(m_blurUpTextures[j]->m_renderTargetView, blackAsFloats);
		}
	}
}	

//get the camera's info to define the glOrtho location
void Renderer :: BeginCamera(Camera const& camera)
{
	// for openGL
	// Vec2 BL = camera.GetOrthoBottomLeft();
	// Vec2 TR = camera.GetOrthoTopRight();
	// glLoadIdentity();
	// glOrtho(BL.x, TR.x, BL.y, TR.y, 0.f, 1.f);// stretch everything on the camera to the window, so different camera size match the same window space

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// for dx11
	// copy the camera info to the camera constant
	CameraConstants cameraInfo;
	cameraInfo.ProjectionMatrix = camera.GetProjectionMatrix();
	cameraInfo.ViewMatrix = camera.GetViewMatrix();

	CopyCPUToGPU(&cameraInfo, sizeof(CameraConstants), m_cameraCBO);
	BindConstantBuffer(k_cameraConstantsSlot, m_cameraCBO);
	 
	// dx11 set viewport
	D3D11_VIEWPORT viewport = { 0 };
	if (camera.GetNormalizedViewport() == AABB2()) // for orthographic cameras which has not set the normalized viewport, we give it full windows display
	{
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.Width = (float)m_config.m_window->GetWindowDimensions().x;
		viewport.Height = (float)m_config.m_window->GetWindowDimensions().y;
	}
	else // if the camera has a normalized viewport set up
	{
		// we need a coordinate for the top left, but asking for dimensions for the width and height
		viewport.TopLeftX = GetCameraViewportForD3D11(camera).m_mins.x;
		viewport.TopLeftY = GetCameraViewportForD3D11(camera).m_mins.y;
		viewport.Width = GetCameraViewportForD3D11(camera).GetDimensions().x;
		viewport.Height = GetCameraViewportForD3D11(camera).GetDimensions().y;
	}
	viewport.MinDepth = 0.f;	
	viewport.MaxDepth = 1.f;

	m_deviceContext->RSSetViewports(1, &viewport);
}

void Renderer::EndCamera(const Camera& camera)
{
	UNUSED(camera);
}

// take every three vertex of the array and draw a triangle
// cont* means this function will not change the vertex Array comming from the input
void Renderer :: DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray)
{
	//glBegin(GL_TRIANGLES);
	//for (int vertexIndex = 0; vertexIndex < numVertexes; ++vertexIndex)
	//{
	//	// use a temp variable to shorten the name
	//	//const& means we are only creating a read-only nickname for a read-only variable
	//	Vertex_PCU const& v = vertexArray[vertexIndex];

	//	glColor4ub(v.m_color.r, v.m_color.g, v.m_color.b, v.m_color.a);
	//	glTexCoord2f(v.m_uvTexCoords.x, v.m_uvTexCoords.y);
	//	glVertex3f(v.m_position.x, v.m_position.y, v.m_position.z);
	//}
	//glEnd();

	// if (numVertices == 0) return;

	// dx11 backwards compatibility for Libra and starship
	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArraySize = numVertices * vertexSize;
	CopyCPUToGPU(vertexArray, vertexArraySize, m_immediateVBO);
	DrawVertexBuffer(m_immediateVBO, numVertices, 0);
}

void Renderer::DrawVertexArray(int numVertices, Vertex_PCUTBN const* vertexArray)
{
	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArraySize = numVertices * vertexSize;
	CopyCPUToGPU(vertexArray, vertexArraySize, m_immediateVertex_PCUTBN_BO);
	DrawVertexBuffer(m_immediateVertex_PCUTBN_BO, numVertices, 0);
}

// we are doing this part in the map class at the start up, because the vertexArray and indexArray never changes
// so we don't need to input these two arguments: std::vector<Vertex_PCUTBN> vertexArray, std::vector<unsigned int> indexArray
void Renderer::DrawVertexArrayWithIndexArray(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int numIndexes)
{
	// size_t vertexSize = sizeof(Vertex_PCUTBN);
	// size_t vertexArraySize = numVertices * vertexSize;
	// CopyCPUToGPU(vbo, vertexArraySize, vbo);
	// 
	// size_t indexSize = sizeof(int);
	// size_t indexArraySize = numIndexes * indexSize;
	// CopyCPUToGPU(ibo, indexArraySize, ibo);

	DrawVertexAndIndexBuffer(vbo, ibo, numIndexes);
}

void Renderer::DrawVertexArrayWithIndexArrayToGetPlanarShadow(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int numIndexes)
{
	if (m_config.m_drawPlanarShadow && m_sunDirection.z < 0.f)
	{
		Vec3 groundIBasis(1.0f, 0.0f, 0.0f);
		Vec3 groundJBasis(0.0f, 1.0f, 0.0f);
		Vec3 groundKBasis(-(m_sunDirection.x / m_sunDirection.z), -(m_sunDirection.y / m_sunDirection.z), 0.0f);
		Vec3 groundTranslation(0.0f, 0.0f, 0.005f);
		Mat44 shadowMatrix(groundIBasis, groundJBasis, groundKBasis, groundTranslation);
		shadowMatrix.Append(m_modelMatForPlanarShadow);

		Rgba8 shadowColor(0, 0, 0, 160);
		// Rgba8 shadowColor(0, 0, 0, 255);

		g_theRenderer->BindShader(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL_STENCIL_ONCE);
		g_theRenderer->SetModelConstants(shadowMatrix, shadowColor);

		SetStatesIfChanged();
		DrawVertexAndIndexBuffer(vbo, ibo, numIndexes);
	}
}

void Renderer::CreateBlurConstantBuffer()
{
	m_blurCBO = CreateConstantBuffer(sizeof(BlurConstants));
}

BlurConstants Renderer::GetDefaultBlurDownConstants()
{
	BlurConstants blurConstants;
	//Blur Down
	blurConstants.NumSamples = 13;
	blurConstants.TexelSize = Vec2(1.f, 1.f); // does this mean a size of a single texel?
	blurConstants.LerpT = 1.f;
	blurConstants.Samples[0].Offset = Vec2();
	blurConstants.Samples[0].Weight = 0.0968f;
	blurConstants.Samples[1].Offset = Vec2(1.f, 1.f);
	blurConstants.Samples[1].Weight = 0.129f;
	blurConstants.Samples[2].Offset = Vec2(1.f, -1.f);
	blurConstants.Samples[2].Weight = 0.129f;
	blurConstants.Samples[3].Offset = Vec2(-1.f, -1.f);
	blurConstants.Samples[3].Weight = 0.129f;
	blurConstants.Samples[4].Offset = Vec2(-1.f, 1.f);
	blurConstants.Samples[4].Weight = 0.129f;
	blurConstants.Samples[5].Offset = Vec2(2.f, 0.f);
	blurConstants.Samples[5].Weight = 0.0645f;
	blurConstants.Samples[6].Offset = Vec2(-2.f, 0.f);
	blurConstants.Samples[6].Weight = 0.0645f;
	blurConstants.Samples[7].Offset = Vec2(0.f, 2.f);
	blurConstants.Samples[7].Weight = 0.0645f;
	blurConstants.Samples[8].Offset = Vec2(0.f, -2.f);
	blurConstants.Samples[8].Weight = 0.0645f;
	blurConstants.Samples[9].Offset = Vec2(2.f, 2.f);
	blurConstants.Samples[9].Weight = 0.0323f;
	blurConstants.Samples[10].Offset = Vec2(-2.f, 2.f);
	blurConstants.Samples[10].Weight = 0.0323f;
	blurConstants.Samples[11].Offset = Vec2(2.f, -2.f);
	blurConstants.Samples[11].Weight = 0.0323f;
	blurConstants.Samples[12].Offset = Vec2(-2.f, -2.f);
	blurConstants.Samples[12].Weight = 0.0323f;
	return blurConstants;
}

BlurConstants Renderer::GetDefaultBlurUpConstants()
{
	BlurConstants blurConstants;
	//Blur Up
	blurConstants.NumSamples = 9;
	blurConstants.LerpT = 0.85f;
	blurConstants.TexelSize = Vec2(1.f, 1.f);;
	blurConstants.Samples[0].Offset = Vec2();
	blurConstants.Samples[0].Weight = 0.25f;
	blurConstants.Samples[1].Offset = Vec2(1.f, 0.f);
	blurConstants.Samples[1].Weight = 0.125f;
	blurConstants.Samples[2].Offset = Vec2(0.f, -1.f);
	blurConstants.Samples[2].Weight = 0.125f;
	blurConstants.Samples[3].Offset = Vec2(-1.f, 0.f);
	blurConstants.Samples[3].Weight = 0.125f;
	blurConstants.Samples[4].Offset = Vec2(0.f, 1.f);
	blurConstants.Samples[4].Weight = 0.125f;
	blurConstants.Samples[5].Offset = Vec2(1.f, 1.f);
	blurConstants.Samples[5].Weight = 0.0625f;
	blurConstants.Samples[6].Offset = Vec2(-1.f, 1.f);
	blurConstants.Samples[6].Weight = 0.0625f;
	blurConstants.Samples[7].Offset = Vec2(1.f, -1.f);
	blurConstants.Samples[7].Weight = 0.0625f;
	blurConstants.Samples[8].Offset = Vec2(1.f, -1.f);
	blurConstants.Samples[8].Weight = 0.0625f;
	return blurConstants;
}

void Renderer::SetBlurConstantBuffer(BlurConstants blurConstant)
{
	CopyCPUToGPU(&blurConstant, sizeof(BlurConstants), m_blurCBO);
	BindConstantBuffer(k_blurConstantSlot, m_blurCBO);
}

void Renderer::CalculateTimesToBlurDown()
{
	int times = 0;
	int heightDimension = m_config.m_window->GetWindowDimensions().y;
	while (heightDimension > k_blurMaxSamples)
	{
		heightDimension = (int)(heightDimension * 0.5f);
		++times;
	}
	++times; // take another time for the smallest blur up texture to sample on
	m_blurDownTimes = times;
}

void Renderer::CreateBlurDownAndUpTextures()
{
	IntVec2 dimensions = m_config.m_window->GetWindowDimensions();
	int height = dimensions.y;
	float ratio = m_config.m_window->GetAspect();

	for (int i = 0; i < m_blurDownTimes; ++i)
	{
		height = (int)(height * 0.5f);
		int width = (int)(height * ratio);
		Texture* blurredDownTexture = CreateRenderTexture(IntVec2(width, height), Stringf("blurredDownTexture_%i", i).c_str());
		m_blurDownTextures.push_back(blurredDownTexture);
	}

	for (int j = 0; j < (m_blurDownTimes - 1 ); ++j)
	{
		height = height * 2;
		int width = (int)(height * ratio);
		Texture* blurredUpTexture = CreateRenderTexture(IntVec2(width, height), Stringf("blurredUpTexture_%i", j).c_str());
		m_blurUpTextures.push_back(blurredUpTexture);
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
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

Image* Renderer::CreateImageFromFile(char const* imageFilePath)
{
	Image* newImage = new Image(imageFilePath);
	return newImage;
}

Texture* Renderer::GetTextureForFileName(char const* imageFilePath)
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

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	// IntVec2 dimensions;		// This will be filled in for us to indicate image width & height
	// int bytesPerTexel = 0; // This will be filled in for us to indicate how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)
	// int numComponentsRequested = 0; // don't care; we support 3 (24-bit RGB) or 4 (32-bit RGBA)

	// because we are using image class and stbi there to handle the texture data, we don't deal with it here
	// // Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	// stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	// unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, numComponentsRequested);
	//  
	// // Check if the load was successful
	// GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));
	
	// // Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	// stbi_image_free(texelData);

	Image* newImagePtr = CreateImageFromFile(imageFilePath);
	Texture* newTexture = CreateTextureFromImage(*newImagePtr);
	 
	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

Texture* Renderer::CreateTextureFromImage(const Image& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_name = image.GetImageFilePath(); // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = image.GetDimensions();

	D3D11_TEXTURE2D_DESC textureDesc = {0};
	textureDesc.Width = image.GetDimensions().x;
	textureDesc.Height = image.GetDimensions().y;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
#ifdef SHIPPING
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
#else
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif
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

	// because of useage and bind flags, we cannot do this
	// hr = m_device->CreateRenderTargetView(newTexture->m_texture, NULL, &newTexture->m_renderTargetView);
	// if (!SUCCEEDED(hr))
	// {
	// 	ERROR_AND_DIE(Stringf("CreateShaderResourceView failed for image file \"%\".",
	// 		image.GetImageFilePath().c_str()));
	// }

	return newTexture;
}

BitmapFont* Renderer::GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension)
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

void Renderer::SetBlendMode(BlendMode blendMode)
{
	switch (blendMode)
	{
	case BlendMode::OPAQUE: {m_desiredBlendMode = BlendMode::OPAQUE; } break;
	case BlendMode::ALPHA: {m_desiredBlendMode = BlendMode::ALPHA; } break;
	case BlendMode::ADDITIVE: {m_desiredBlendMode = BlendMode::ADDITIVE; } break;
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode)); } break;
	}

	// if (blendMode == BlendMode::ALPHA)
	// {
	// 	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// }
	// else if (blendMode == BlendMode::ADDITIVE)
	// {
	// 	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	// }
	// else
	// {
	// 	ERROR_AND_DIE(Stringf("Unknown / unsupported blend mode #%i", blendMode));
	// }
}

void Renderer::SetDepthMode(DepthMode depthMode)
{
	switch (depthMode)
	{
	case DepthMode::DISABLED: {m_desiredDepthMode = DepthMode::DISABLED; }
		break;
	case DepthMode::ENABLED: {m_desiredDepthMode = DepthMode::ENABLED; }
	break;	
	case DepthMode::READ_ONLY_LESS_EQUAL : { m_desiredDepthMode = DepthMode::READ_ONLY_LESS_EQUAL; }
	break;	
	case DepthMode::READ_ONLY_LESS_EQUAL_STENCIL_ONCE: { m_desiredDepthMode = DepthMode::READ_ONLY_LESS_EQUAL_STENCIL_ONCE; }
	break;
	default: {m_desiredDepthMode = DepthMode::ENABLED; }
		break;
	}
}

void Renderer::SetRasterizerMode(RasterizerMode rasterizerState)
{
	switch (rasterizerState)
	{
	case RasterizerMode::SOLID_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_NONE; }
		break;
	case RasterizerMode::SOLID_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK; }
		break;
	case RasterizerMode::SOLID_CULL_FRONT: {m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_FRONT; }
		break;
	case RasterizerMode::WIREFRAME_CULL_NONE: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE; }
		break;
	case RasterizerMode::WIREFRAME_CULL_BACK: {m_desiredRasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK; }
		break;	
	default: {ERROR_AND_DIE(Stringf("Unknown / unsupported rasterizer State #%i", rasterizerState)); }
		break;
	}
}

void Renderer::CreateSamplerState()
{
	// create the point clamp sampler state
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT hr;
	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)SamplerMode::POINT_CLAMP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for samplerMode::POINT_CLAMP failed.")
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// create the bilinear wrap sampler state
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	hr = m_device->CreateSamplerState(&samplerDesc,
		&m_samplerStates[(int)SamplerMode::BILINEAR_WRAP]);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for samplerMode::POINT_WRAP failed.")
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// Create a sampler state description
	D3D11_SAMPLER_DESC cullFrontSamplerDesc = {};
	cullFrontSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;    // Bilinear filtering
	cullFrontSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;      // Clamp on U axis
	cullFrontSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;      // Clamp on V axis
	cullFrontSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;      // Clamp on W axis (for 3D textures)

	// // Set the default border color (only relevant if you use BORDER address mode)
	// samplerDesc.BorderColor[0] = 0.0f;
	// samplerDesc.BorderColor[1] = 0.0f;
	// samplerDesc.BorderColor[2] = 0.0f;
	// samplerDesc.BorderColor[3] = 0.0f;
	// 
	// // Set the minimum and maximum LOD (level of detail)
	// samplerDesc.MinLOD = 0.0f;                               // Lowest mip level (0 = most detailed)
	// samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;                  // Use all available mip levels
	// 
	// // Mip LOD bias and max anisotropy
	// samplerDesc.MipLODBias = 0.0f;
	// samplerDesc.MaxAnisotropy = 1;                           // No anisotropic filtering (1 means disabled)
	// 
	// // Comparison function (not relevant for normal sampling, mainly for shadow maps)
	// samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;     // Disable comparison for this sampler

	// Create the sampler state
	hr = m_device->CreateSamplerState(&cullFrontSamplerDesc, &m_samplerStates[(int)SamplerMode::BILINEAR_CLAMP]);

	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("CreateSamplerState for samplerMode::POINT_CLAMP failed.")
	}
}

void Renderer::SetSamplerMode(SamplerMode samplerMode)
{
	switch (samplerMode)
	{
	case SamplerMode::POINT_CLAMP: { m_desiredSamplerMode = SamplerMode::POINT_CLAMP; } break;
	case SamplerMode::BILINEAR_WRAP: { m_desiredSamplerMode = SamplerMode::BILINEAR_WRAP; } break;
	default: { m_desiredSamplerMode = SamplerMode::POINT_CLAMP; } break;
	}
}

void Renderer::DetectDXGIMemoryLeak()
{
	// create debug module
#if defined(ENGINE_DEBUG_RENDER)
	m_dxgiDebugModule = (void*)::LoadLibraryA("dxgidebug.dll");
	if (m_dxgiDebugModule == nullptr)
	{
		ERROR_AND_DIE("Could not load dxgidebug.dll");
	}

	typedef HRESULT(WINAPI* GetDebugModuleCB)(REFIID, void**);
	(
		(GetDebugModuleCB)::GetProcAddress((HMODULE)m_dxgiDebugModule, "DXGIGetDebugInterface")
		)
		(__uuidof(IDXGIDebug), &m_dxgiDebug);

	if (m_dxgiDebug == nullptr)
	{
		ERROR_AND_DIE("Could not load debug module.");
	}
#endif
}

BitmapFont* Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	// See if we already have this font previously loaded
	BitmapFont* existingFont = GetBitMapFontForFileName(bitmapFontFilePathWithNoExtension);
	if (existingFont)
	{
		return existingFont;
	}
	// Never seen this font before!  Let's load it.
	BitmapFont* font = CreateBitmapFont(bitmapFontFilePathWithNoExtension);
	return font;
}

BitmapFont* Renderer::CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension, IntVec2 rowAndColumns /*= IntVec2(16, 16)*/)
{
	Texture* fontTexture = CreateOrGetTextureFromFile(bitmapFontFilePathWithNoExtension);
	BitmapFont* font = new BitmapFont(bitmapFontFilePathWithNoExtension, *fontTexture, rowAndColumns); // todo:??? need to use 'new' or it will be store on stack memory and disppear after the end of this function right?
	m_loadedFonts.push_back(font);
	return font;
}

Shader* Renderer::CreateOrGetShader(char const* shaderPath, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// convert the c-style string to C++ style string
	// std::string newShaderName;
	// newShaderName = std::string(shaderPath);
	HashedCaseInsensitiveString newShaderName = HashedCaseInsensitiveString(shaderPath);

	// check to see if the shader has been loaded before
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (newShaderName == m_loadedShaders[shaderIndex]->m_config.m_shaderPath || newShaderName == m_loadedShaders[shaderIndex]->m_config.m_shaderName)
		{
			// if so, skip the step of shader creation
			// just set is as current shader
			return m_loadedShaders[shaderIndex];
		}
	}

	// if the name of the shader does not exist, create a new shader
	ShaderConfig newConfig;
	Shader* newShader = new Shader(newConfig);
	newShader = CreateShader(shaderPath, vertexType);
	return newShader;
}

Shader* Renderer::CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// create a new shader config and a shader
	ShaderConfig config;
	config.m_shaderPath = HashedCaseInsensitiveString(shaderPath);

	// use the shader path to get a name for the shader
	string shaderPathStr(shaderPath);
	Strings pathStrs;
	string symbol("\\");
	SplitStringOnDelimiter(pathStrs, shaderPathStr, symbol);
	config.m_shaderName = HashedCaseInsensitiveString(pathStrs.back());

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
			ERROR_AND_DIE(Stringf("Could not create Vertex_PCUTBN layout."));
		}
	}break;
	case VertexType::Vertex_PCUTBN:
	{
		D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
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

Shader* Renderer::CreateShader(char const* shaderPath, VertexType vertexType /* = VertexType::Vertex_PCU*/)
{
	// get the file path
	// if the we try to create the default shader, it is going to read from the renderer folder file
	// if we try to create new shader, it is going to read from the data/shaders folder
	std::string fileName = std::string(shaderPath);
	fileName.append(".hlsl");

	// read the file and get the shader source code 
	std::string shaderString;
	FileReadToString(shaderString, fileName);
	defaultShaderSource = shaderString.c_str();

	// create and return new shader
	Shader* newShader;
	newShader = CreateShader(shaderPath, defaultShaderSource, vertexType);
	m_currentShader = newShader;
	return newShader;
}

// outByteCode is a machine code that could read by machine, therefore needed to be compile by the DirectX 3D
bool Renderer::CompileShaderToByteCode(std::vector<unsigned char>&outByteCode, char const* name, char const* shaderSource, char const* entryPoint, char const* target)
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

void Renderer::BindShader(Shader* shader)
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

// the size here means how many vertices are there in the buffer
VertexBuffer* Renderer::CreateVertexBuffer(const size_t size, size_t stride /*= sizeof(Vertex_PCU)*/, bool isLinePrimitive /*= false*/, ID3D11Device* device /*= nullptr*/)
{
	// create a local D3D11_BUFFER_DESC variable
	// create vertex buffer
	if (!device)
	{
		device = m_device;
	}

	VertexBuffer* newBuffer = new VertexBuffer(size, stride, isLinePrimitive, device);
	// Using std::make_unique to create a unique pointer for VertexBuffer
	// std::unique_ptr<VertexBuffer> newBuffer = std::make_unique<VertexBuffer>(size, stride, isLinePrimitive, device);
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

IndexBuffer* Renderer::CreateIndexBuffer(const size_t size, size_t stride /*sizeof(int)*/, ID3D11Device* device /*nullptr*/)
{
	// create a local D3D11_BUFFER_DESC variable
	// create index buffer
	if (!device)
	{
		device = m_device;
	}
	IndexBuffer* newBuffer = new IndexBuffer(size, stride, device);
	UINT indexBufferSize = (UINT)(size * stride);
	D3D11_BUFFER_DESC bufferDesc = { 0 };// trying to initialize all the variable inside the struct as default
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = indexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr;
	hr = m_device->CreateBuffer(&bufferDesc, nullptr, &newBuffer->m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create index buffer.");
	}

	return newBuffer;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Renderer::CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo)
{
	// Check if the exiting immediate vertex buffer is large enough for the data being passed in
	if ( ((vbo->m_size) * (vbo->m_stride)) < size)
	{
		size_t stride = vbo->m_stride;
		delete vbo;

		// recreate the vertex buffer so it is sufficiently large and not introduce memory leaks
		vbo = CreateVertexBuffer(size, stride);
	}

	// copy the vertex buffer data from the CPU to GPU
	// copy vertices
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(vbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(vbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(cbo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(cbo->m_buffer, 0);
}

void Renderer::CopyCPUToGPU(void const* data, size_t size, IndexBuffer*& ibo)
{
	// Check if the exiting immediate vertex buffer is large enough for the data being passed in
	if (ibo->m_size < size)
	{
		delete ibo;
		ibo = CreateIndexBuffer(size);
	}

	// copy the Index buffer data from the CPU to GPU
	// copy vertices
	D3D11_MAPPED_SUBRESOURCE resource;
	m_deviceContext->Map(ibo->m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
	memcpy(resource.pData, data, size);
	m_deviceContext->Unmap(ibo->m_buffer, 0);
}

// pass value to Shader
void Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	m_deviceContext->VSSetConstantBuffers(slot, 1, &cbo->m_buffer);
	m_deviceContext->PSSetConstantBuffers(slot, 1, &cbo->m_buffer);
}

void Renderer::BindIndexBuffer(IndexBuffer* ibo, int indexOffset /*= 0*/)
{
	m_deviceContext->IASetIndexBuffer(ibo->m_buffer, DXGI_FORMAT_R32_UINT, indexOffset);
}

void Renderer::BindVertexBuffer(VertexBuffer* vbo, int vertexOffset /*= 0*/)
{
	UINT stride = vbo->m_stride;
	UINT startOffset = vertexOffset;
	m_deviceContext->IASetVertexBuffers(0, 1, &vbo->m_buffer, &stride, &startOffset);

	if (vbo->m_isLinePrimitive)
	{
		m_deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	}
	else
	{
		m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
}

void Renderer::DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset /*= 0*/)
{
	BindVertexBuffer(vbo);
	SetStatesIfChanged();
	m_deviceContext->Draw(vertexCount, vertexOffset);
}

void Renderer::DrawVertexAndIndexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, int indexCount, 
	int indexOffset /*= 0*/, int vertexOffset /*= 0*/)
{
	BindVertexBuffer(vbo, vertexOffset);
	BindIndexBuffer(ibo, indexOffset);
	SetStatesIfChanged();
	m_deviceContext->DrawIndexed(indexCount, 0, 0);
}

Shader* Renderer::GetLoadedShader(char const* shaderName)
{
	HashedCaseInsensitiveString shaderNameHCIS(shaderName);

	// check to see if the shader has been loaded before
	for (int shaderIndex = 0; shaderIndex < (int)m_loadedShaders.size(); ++shaderIndex)
	{
		if (shaderNameHCIS == m_loadedShaders[shaderIndex]->m_config.m_shaderPath)
		{
			// if so, skip the step of shader creation
			// just set is as current shader			
			return m_loadedShaders[shaderIndex];
		}
	}

	return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void Renderer::CreateCameraConstantBuffer()
{
	m_cameraCBO = CreateConstantBuffer(sizeof(CameraConstants));
}

void Renderer::CreateLightingConstantBuffer()
{
	m_lightingCBO = CreateConstantBuffer(sizeof(LightingConstants));
}

void Renderer::SetLightingConstants(Vec3 lightDirection, float sunIntensity, float ambientIntensity)
{
	LightingConstants lightingInfo;
	lightingInfo.SunDirection = lightDirection.GetNormalized();
	lightingInfo.SunIntensity = sunIntensity;
	lightingInfo.AmbientIntensity = ambientIntensity;

	CopyCPUToGPU(&lightingInfo, sizeof(LightingConstants), m_lightingCBO);
	BindConstantBuffer(k_lightingConstantsSlot, m_lightingCBO);
}

void Renderer::SetLightingConstants(LightingConstants newSetting)
{
	newSetting.SunDirection = newSetting.SunDirection.GetNormalized();
	CopyCPUToGPU(&newSetting, sizeof(LightingConstants), m_lightingCBO);
	BindConstantBuffer(k_lightingConstantsSlot, m_lightingCBO);
}

void Renderer::CreatePhongLightingConstantBuffer()
{
	m_PhongLightingCBO = CreateConstantBuffer(sizeof(PhongLightingConstants));
}

void Renderer::SetPhongLightingConstants(PhongLightingConstants const& lightingConstants /*= PhongLightingConstants()*/)
{
	CopyCPUToGPU(&lightingConstants, sizeof(PhongLightingConstants), m_PhongLightingCBO);
	BindConstantBuffer(k_lightingConstantsSlot, m_PhongLightingCBO);
}

Vec2 Renderer::GetRenderWindowDimensions() const
{
	float width = (float)m_config.m_window->GetWindowDimensions().x;
	float height = (float)m_config.m_window->GetWindowDimensions().y;
	return Vec2(width, height);
}

// translate the engine view port to directX viewport
AABB2 Renderer::GetCameraViewportForD3D11(Camera const camera) const
{
	float width = (float)m_config.m_window->GetWindowDimensions().x;
	float height = (float)m_config.m_window->GetWindowDimensions().y;

	AABB2 engineViewport = camera.GetNormalizedViewport();
	AABB2 D3D11Viewport;
	D3D11Viewport.m_mins.x = engineViewport.m_mins.x;
	D3D11Viewport.m_maxs.y = 1.f - engineViewport.m_mins.y;
	D3D11Viewport.m_maxs.x = engineViewport.m_maxs.x;
	D3D11Viewport.m_mins.y = 1.f - engineViewport.m_maxs.y;

	D3D11Viewport.m_mins.x *= width;
	D3D11Viewport.m_maxs.x *= width;
	D3D11Viewport.m_mins.y *= height;
	D3D11Viewport.m_maxs.y *= height;
	return D3D11Viewport;
}

void Renderer::CreateModelConstantBuffer()
{
	m_modelCBO = CreateConstantBuffer(sizeof(ModelConstants));
}

ConstantBuffer* Renderer::CreateConstantBuffer(const size_t size)
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

void Renderer::SetModelConstants(const Mat44& modelMatrix /*= Mat44()*/, const Rgba8& modelColor /*= Rgba8::WHITE*/)
{
	ModelConstants modelInfo;
	modelInfo.ModelMatrix = modelMatrix;

	modelColor.GetAsFloats(modelInfo.ModelColor);

	CopyCPUToGPU(&modelInfo, sizeof(modelInfo), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}

void Renderer::SetModelConstantsTesting(const Mat44& modelMatrix /*= Mat44()*/, const Rgba8& modelColor /*= Rgba8::WHITE*/)
{
	ModelConstants modelInfo;
	modelInfo.ModelMatrix = modelMatrix;
	modelColor.GetAsFloats(modelInfo.ModelColor);

	CopyCPUToGPU(&modelInfo, sizeof(modelInfo), m_modelCBO);
	BindConstantBuffer(k_modelConstantsSlot, m_modelCBO);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

void Renderer::CreateAllRasterizerStates()
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
	// Step 1: Create a rasterizer state description
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;        // Solid fill mode
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;        // Cull front-facing polygons
	// rasterDesc.FrontCounterClockwise = false;      // Clockwise vertices are front-facing (adjust based on winding order)
	// rasterDesc.DepthBias = 0;                      // No depth bias
	// rasterDesc.SlopeScaledDepthBias = 0.0f;
	// rasterDesc.DepthBiasClamp = 0.0f;
	// rasterDesc.DepthClipEnable = true;             // Enable depth clipping
	// rasterDesc.ScissorEnable = false;              // Disable scissor testing
	// rasterDesc.MultisampleEnable = false;          // Disable multisampling
	// rasterDesc.AntialiasedLineEnable = false;      // Disable antialiased lines

	// Step 2: Create the rasterizer state
	hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerState[(int)(RasterizerMode::SOLID_CULL_FRONT)]);
	if (!SUCCEEDED(hr)) 
	{
		ERROR_AND_DIE("Failed to create rasterizer state.");
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

void Renderer::SetStatesIfChanged()
{
	// check if the desired blend mode is the same as current blend state
	if (m_currentBlendState != m_blendStates[(int)m_desiredBlendMode])
	{
		m_currentBlendState = m_blendStates[(int)m_desiredBlendMode];

		float blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		UINT sampleMask = 0xffffffff;
		m_deviceContext->OMSetBlendState(m_currentBlendState, blendFactor, sampleMask);
	}

	// set the depth mode if is changed
	if (m_desiredDepthMode != m_depthMode)
	{
		// UINT stencilRef = 0xffffffff;
		m_deviceContext->OMSetDepthStencilState(m_depthStencilStates[(int)m_desiredDepthMode], 0);
		m_depthMode = m_desiredDepthMode;
	}
	
	// sampler state
	if (m_currentSamplerState != m_samplerStates[(int)m_desiredSamplerMode])
	{
		m_deviceContext->PSSetSamplers(0, 1, &m_samplerStates[(int)m_desiredSamplerMode]);
		m_currentSamplerState = m_samplerStates[(int)m_desiredSamplerMode];
	}

	// rasterizer mode
	if (m_desiredRasterizerMode != m_rasterizerMode)
	{
		m_deviceContext->RSSetState(m_rasterizerState[(int)m_desiredRasterizerMode]);
		m_rasterizerMode = m_desiredRasterizerMode;
	}
}

void Renderer::CreateAllBlendStates()
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

// used when rendering in openGL environment
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("CreateTextureFromData failed for \"%s\" - texelData was null!", name));
	GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4, Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", name, bytesPerTexel));
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", name, dimensions.x, dimensions.y));

	Texture* newTexture = new Texture();
	newTexture->m_name = name; // NOTE: m_name must be a std::string, otherwise it may point to temporary data!
	newTexture->m_dimensions = dimensions;

	//// Enable OpenGL texturing
	//glEnable(GL_TEXTURE_2D);

	//// Tell OpenGL that our pixel data is single-byte aligned
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//// Ask OpenGL for an unused texName (ID number) to use for this texture
	//glGenTextures(1, (GLuint*)&newTexture->m_textureID);

	//// Tell OpenGL to bind (set) this as the currently active texture
	//glBindTexture(GL_TEXTURE_2D, newTexture->m_textureID);

	//// Set texture clamp vs. wrap (repeat) default settings
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_CLAMP or GL_REPEAT
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_CLAMP or GL_REPEAT

	//// Set magnification (texel > pixel) and minification (texel < pixel) filters
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // one of: GL_NEAREST, GL_LINEAR, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR

	//// Pick the appropriate OpenGL format (RGB or RGBA) for this texel data
	//GLenum bufferFormat = GL_RGBA; // the format our source pixel data is in; any of: GL_RGB, GL_RGBA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, ...
	//if (bytesPerTexel == 3)
	//{
	//	bufferFormat = GL_RGB;
	//}
	//GLenum internalFormat = bufferFormat; // the format we want the texture to be on the card; technically allows us to translate into a different texture format as we upload to OpenGL

	//// Upload the image texel data (raw pixels bytes) to OpenGL under this textureID
	//glTexImage2D(			// Upload this pixel data to our new OpenGL texture
	//	GL_TEXTURE_2D,		// Creating this as a 2d texture
	//	0,					// Which mipmap level to use as the "root" (0 = the highest-quality, full-res image), if mipmaps are enabled
	//	internalFormat,		// Type of texel format we want OpenGL to use for this texture internally on the video card
	//	dimensions.x,		// Texel-width of image; for maximum compatibility, use 2^N + 2^B, where N is some integer in the range [3,11], and B is the border thickness [0,1]
	//	dimensions.y,		// Texel-height of image; for maximum compatibility, use 2^M + 2^B, where M is some integer in the range [3,11], and B is the border thickness [0,1]
	//	0,					// Border size, in texel (must be 0 or 1, recommend 0)
	//	bufferFormat,		// Pixel format describing the composition of the pixel data in buffer
	//	GL_UNSIGNED_BYTE,	// Pixel color components are unsigned bytes (one byte per color channel/component)
	//	texelData);		// Address of the actual pixel data bytes/buffer in system memory

	m_loadedTextures.push_back(newTexture);
	return newTexture;
}

void Renderer::BindTexture(Texture const* texture)
{
	// if the texture does not exist, bind it to default texture
	if (!texture)
	{
		//glEnable(GL_TEXTURE_2D);
		//glBindTexture(GL_TEXTURE_2D, texture->m_textureID);
		m_currentTexture = m_defaultWhiteTexture;
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

void Renderer::BindDiffuseSpecularNormalTextures(Texture const* texture_d, Texture const* texture_s, Texture const* texture_n)
{
	ID3D11ShaderResourceView* textures[3];

	// If the first texture does not exist, bind it to the default texture
	if (!texture_d)
	{
		m_currentTexture = m_defaultWhiteTexture;
		textures[0] = m_defaultWhiteTexture->m_shaderResourceView;  // Slot 0 (diffuseTexture)
	}
	else
	{
		m_currentTexture = texture_d;
		textures[0] = m_currentTexture->m_shaderResourceView;  // Slot 0 (diffuseTexture)
	}

	// if the texture does not exist, bind it to default texture
	if (!texture_s)
	{
		textures[1] = m_defaultBlackTexture->m_shaderResourceView;   // Slot 2 (specGlossEmitTexture)
	}
	else
	{
		textures[1] = texture_s->m_shaderResourceView;          // Slot 2 (specGlossEmitTexture)
	}

	// if the texture does not exist, bind it to default texture
	if (!texture_n)
	{
		textures[2] = m_defaultBlackTexture->m_shaderResourceView;   // Slot 2 (specGlossEmitTexture)
	}
	else
	{
		textures[2] = texture_n->m_shaderResourceView;          // Slot 2 (specGlossEmitTexture)
	}

	// Bind both textures to the respective slots
	m_deviceContext->PSSetShaderResources(0, 3, textures);  // Bind to slot 0 and 2
}

void Renderer::CreateDefaultTexture()
{
	Image* whiteImage = new Image(IntVec2(2, 2), Rgba8::WHITE);
	m_defaultWhiteTexture = CreateTextureFromImage(*whiteImage);
	m_loadedTextures.push_back(const_cast<Texture*>(m_defaultWhiteTexture));	
	
	Image* blackImage = new Image(IntVec2(2, 2), Rgba8::BLACK);
	m_defaultBlackTexture = CreateTextureFromImage(*blackImage);
	m_loadedTextures.push_back(const_cast<Texture*>(m_defaultBlackTexture));
}
