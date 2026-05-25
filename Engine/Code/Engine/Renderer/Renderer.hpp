#pragma once
#include "Engine/core/Rgba8.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Game/EngineBuildPreferences.hpp"
#include <vector>

//----------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef VR_MODE
// #define XR_USE_GRAPHICS_API_D3D11
// #define XR_USE_PLATFORM_WIN32
// #define XR_KHR_D3D11_ENABLE_EXTENSION_NAME "XR_KHR_D3D11_enable"

#include "ThirdParty/OpenXR/Application/pch.h"
#include "ThirdParty/OpenXR/Application/common.h"
#include "ThirdParty/OpenXR/Application/options.h"
#include "ThirdParty/OpenXR/include/openxr/openxr_platform.h"
using namespace Microsoft::WRL;

	struct  IDXGIAdapter1;

	#include <wrl/client.h>
	#include <DirectXMath.h>
	#include <DirectXColors.h>
	#include <D3Dcompiler.h>


struct Swapchain
{
	XrSwapchain handle;
	int32_t width;
	int32_t height;
};

#endif
//----------------------------------------------------------------------------------------------------------------------------------------------------
class	Window;
class	Texture;
class	Image;
class	VertexBuffer;
class	IndexBuffer;
class	ConstantBuffer;
struct	ID3D11RasterizerState;
struct	ID3D11RenderTargetView;
struct	ID3D11Device;
struct	ID3D11DeviceContext;
struct	IDXGISwapChain;
struct  ID3D11BlendState;
struct	ID3D11SamplerState;
struct  ID3D11DepthStencilState;
struct  ID3D11DepthStencilView;

#if defined(OPAQUE)
#undef OPAQUE
#endif

struct LightingConstants
{
	LightingConstants(Vec3 direction, float sunIntensity, float ambientIntensity)
		: SunDirection(direction)
		, SunIntensity(sunIntensity)
		, AmbientIntensity(ambientIntensity)
	{}
	LightingConstants() {}
	~LightingConstants() {}

	Vec3 SunDirection;
	float SunIntensity;
	float AmbientIntensity;
	float EmptySpace[3]; // GPU requires every constant buffer has the multiple times size of 16
};

struct PhongLightingDebug
{
	int RenderAmbient = true;
	int RenderDiffuse = true;
	int RenderSpecular = true;
	int RenderEmissive = true;
	int UseDiffuseMap = true;
	int UseNormalMap = true;
	int UseSpecularMap = true;
	int UseGlossinessMap = true;
	int UseEmissiveMap = true;
	float Padding[3];
};

struct PhongLightingConstants
{
	PhongLightingConstants() {}
	~PhongLightingConstants() {}

	Vec3  SunDirection = Vec3(0.f, 0.f, -1.f);
	float SunIntensity = 0.5f;
	float AmbientIntensity = 0.5f;
	Vec3  WorldEyePosition = Vec3::ZERO;

	float MinFalloff = 0.f;
	float MaxFalloff = 0.1f;	
	float MinFalloffMultiplier = 0.f;
	float MaxFalloffMultiplier = 1.f;

	PhongLightingDebug lighingDebug;
};

enum class BlendMode
{
	OPAQUE,
	ALPHA,
	ADDITIVE,
	COUNT
};

enum class VertexType
{
	Vertex_PCU,
	Vertex_PCUTBN,
	Vertex_PCUTBN_J,	// for drawing character animation
	COUNT
};

enum class SamplerMode
{
	POINT_CLAMP,	// for bit font map
	BILINEAR_WRAP,	// for texture rendering on objects
	BILINEAR_CLAMP, // for emissive texture
	COUNT
};

enum class RasterizerMode
{
	SOLID_CULL_NONE,
	SOLID_CULL_BACK,
	SOLID_CULL_FRONT,
	WIREFRAME_CULL_NONE,
	WIREFRAME_CULL_BACK,
	COUNT
};

enum class DepthMode
{
	DISABLED,
	ENABLED,
	READ_ONLY_LESS_EQUAL,					// for the particles rendering
	READ_ONLY_LESS_EQUAL_STENCIL_ONCE,		// for shadow rendering
	COUNT
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
struct ModelConstantsArray
{
	Mat44 ModelMatrixArray[64];	// we assume a max of 64 joints in a character
	float ModelColor[256];		// 256 = 64 * 4
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
struct BlurSample
{
	Vec2 Offset;
	float Weight;
	int Padding;
};

static const int k_blurMaxSamples = 64;
struct BlurConstants
{
	Vec2 TexelSize;
	float LerpT;
	int NumSamples;
	BlurSample Samples[k_blurMaxSamples];
};
static const int k_blurConstantSlot = 5;

//----------------------------------------------------------------------------------------------------------------------------------------------------
struct RenderConfig
{
	Window* m_window = nullptr;
	bool m_emissiveEnabled = false;
	bool m_drawPlanarShadow = false;
};

class Renderer 
{
public:
	Renderer( RenderConfig const& config );
	Renderer();

	void Startup();
	void BeginFrame();
	void BeginFrame_BindingRTVsAndDepthStencilView();
	void EndFrame();
	void Shutdown();
	void ReportErrorLeaksAndReleaseDebugModule();
	 
	void ClearScreen(const Rgba8& clearColor);
	void BeginCamera(const Camera& camera);
	void EndCamera(const Camera& camera);
	void DrawVertexArray(int numVertices, Vertex_PCU const* vertexArray);
	void DrawVertexArray(int numVertices, Vertex_PCUTBN const* vertexArray);
	void DrawVertexArrayWithIndexArray(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int numIndexes);

	void DrawVertexArrayWithIndexArrayToGetPlanarShadow(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int numIndexes);
	Mat44 m_modelMatForPlanarShadow;
	Vec3 m_sunDirection = Vec3::ZERO;

	void CreateDeviceAndSwapChain();
	void GetBackBufferAndCreateRenderTargetView();

	void CreateAndBindDefaultShader();
	void CreateBlurUpAndDownShaders();
	void CreateImmediateVertexBuffer(); // for vertex PCU
	void CreateImmediateVertexPCUTBNBuffer();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// settings for emissive texture
	// do all the down sampling, up sampling, and compositing to render an emissive bloom effect.
	void		CreateBlurDown_BlurUp_CompositeShaders();
	Shader*		m_blurDownShader = nullptr;
	Shader*		m_blurUpShader = nullptr;
	Shader*		m_compositeShader = nullptr;

	void		RenderEmissive();
	Texture*	CreateRenderTexture(IntVec2 const& dimensions, const char* name);
	void		BindTextureToSpecificSlot(const Texture* texture, unsigned int slot = 0);

	void		CreateEmissveAndBluredEmissveTexture();
	void		CreateFullScreenQuadVertexbuffer(); // Create a full screen quad vertex buffer at start up. This will be used for up and down sampling

	void		CreateBlurConstantBuffer();
	void		SetBlurConstantBuffer(BlurConstants blurConstant);

	ConstantBuffer* m_blurCBO = nullptr;

	void		CalculateTimesToBlurDown();
	void		CreateBlurDownAndUpTextures();
	int			m_blurDownTimes = 0;

	std::vector<Texture*> m_blurUpTextures;
	std::vector<Texture*> m_blurDownTextures;
	Texture*	m_emissiveRenderTexture = nullptr;
	Texture*	m_emissiveBlurredRenderTexture = nullptr;

	BlurConstants		GetDefaultBlurDownConstants();
	BlurConstants		GetDefaultBlurUpConstants();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// texture and image functions
	Texture*	CreateOrGetTextureFromFile(char const* imageFilePath);
	Texture*	CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData); // only called when the texture is not loaded before
	Texture*	GetTextureForFileName(char const* imageFilePath);
	void		BindTexture(Texture const* texture);

	// Bind diffuse and 
	void		BindDiffuseSpecularNormalTextures(Texture const* texture_d, Texture const* texture_s, Texture const* texture_n);
	void		CreateDefaultTexture();
	Image*		CreateImageFromFile(char const* imageFilePath);
	Texture*	CreateTextureFromImage(Image const& image);

	BitmapFont* CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension);
	BitmapFont* GetBitMapFontForFileName(char const* bitmapFontFilePathWithNoExtension);

	void		DetectDXGIMemoryLeak();

	// change rendering mode and states
	void		SetBlendMode(BlendMode blendMode);
	void		SetDepthMode(DepthMode depthMode);
	void		SetRasterizerMode(RasterizerMode rasterizerState);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	ConstantBuffer* CreateConstantBuffer(const size_t size);
	void			BindConstantBuffer(int slot, ConstantBuffer* cbo);
	void			CopyCPUToGPU(void const* data, size_t size, ConstantBuffer* cbo);

	// constant buffer
	ConstantBuffer* m_cameraCBO = nullptr;
	ConstantBuffer* m_modelCBO = nullptr;

	ConstantBuffer* m_lightingCBO = nullptr;
	ConstantBuffer* m_PhongLightingCBO = nullptr;

	VertexBuffer*   m_immediateVBO = nullptr; // backward compatibility for Libra and Starship
	VertexBuffer*   m_immediateVertex_PCUTBN_BO = nullptr; // buffer for vertex_PCUTBN
	// IndexBuffer* m_immediateIBO = nullptr; // because the index buffer is created and managed by different class like map

	// camera constant buffer related functions
	void			CreateCameraConstantBuffer();

	// Diffuse lighting constant setting - for Doomenstein
	void			CreateLightingConstantBuffer();
	void			SetLightingConstants(Vec3 lightDirection, float sunIntensity, float ambientIntensity);
	void			SetLightingConstants(LightingConstants newSetting);

	// Phong lighting constant setting
	void			CreatePhongLightingConstantBuffer();
	void			SetPhongLightingConstants(PhongLightingConstants const& lightingConstants = PhongLightingConstants());

	Vec2			GetRenderWindowDimensions() const;
	AABB2			GetCameraViewportForD3D11(Camera const camera) const; // TL is (0.f, 0.f), BR is (2000.f, 1000.f)

	// model constant buffer related functions
	void		  CreateModelConstantBuffer();
	void		  SetModelConstants(const Mat44& modelMatrix = Mat44(), const Rgba8& modelColor = Rgba8::WHITE);
	void		  SetModelConstantsTesting(const Mat44& modelMatrix = Mat44(), const Rgba8& modelColor = Rgba8::WHITE);

	// Dynamic Buffers
	// vertex buffer functions
	VertexBuffer* CreateVertexBuffer(const size_t size, size_t stride = sizeof(Vertex_PCU), bool isLinePrimitive = false, ID3D11Device* device = nullptr);
	void		  CopyCPUToGPU(void const* data, size_t size, VertexBuffer*& vbo);
	void		  BindVertexBuffer(VertexBuffer* vbo, int vertexOffset = 0);
	void		  DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset = 0);

	// index buffer functions
	IndexBuffer*  CreateIndexBuffer(const size_t size, size_t stride = sizeof(int), ID3D11Device* device = nullptr);
	void		  CopyCPUToGPU(void const* data, size_t size, IndexBuffer*& ibo);
	void		  BindIndexBuffer(IndexBuffer* ibo, int indexOffset);
	void		  DrawVertexAndIndexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, int indexCount, int indexOffset = 0, int vertexOffset = 0);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// dx11 shader creation functions
	Shader*		  GetLoadedShader(char const* shaderName);
	Shader*		  CreateOrGetShader(char const* shaderPath, VertexType vertexType = VertexType::Vertex_PCU); // see if we need to create a new shader or just reloaded the one
	Shader*		  CreateShader(char const* shaderPath, char const* shaderSource, VertexType vertexType = VertexType::Vertex_PCU);
	Shader*		  CreateShader(char const* shaderPath, VertexType vertexType = VertexType::Vertex_PCU);
	bool		  CompileShaderToByteCode(std::vector<unsigned char>& outByteCode, char const* name,
											char const* shaderSource, char const* entryPoint, char const* target);
	void		  BindShader(Shader* shader);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// openXR support
	// std::vector<std::string> GetInstanceExtensions() const;
	// void InitializeDeviceForOpenXR(XrInstance instance, XrSystemId systemId);

	RenderConfig m_config; // use this to get the windows pointer

	Texture*	CreateTextureFromFile(char const* imageFilePath); // todo:??? why it is a texture* not texture
	BitmapFont* CreateBitmapFont(char const* bitmapFontFilePathWithNoExtension, IntVec2 rowAndColumns = IntVec2(16, 16));

private:
	// should not let any else class modify the the loaded assets of textures and fonts
	std::vector<Texture*>		m_loadedTextures;
	Texture const*				m_currentTexture = nullptr;
	std::vector<BitmapFont*>	m_loadedFonts;


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// blend mode 
	BlendMode		  m_desiredBlendMode = BlendMode::ALPHA;
	ID3D11BlendState* m_currentBlendState = nullptr;
	ID3D11BlendState* m_blendStates[(int)(BlendMode::COUNT)] = {};

	void			  SetStatesIfChanged();
	void			  CreateAllBlendStates();

	// sampler state
	ID3D11SamplerState* m_currentSamplerState = nullptr;
	SamplerMode			m_desiredSamplerMode = SamplerMode::POINT_CLAMP;
	ID3D11SamplerState* m_samplerStates[(int)(SamplerMode::COUNT)] = {};

	// rasterizer state
	void						CreateAllRasterizerStates();

	RasterizerMode				m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	RasterizerMode				m_desiredRasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	ID3D11RasterizerState*		m_rasterizerState[(int)(RasterizerMode::COUNT)];

	void						CreateSamplerState();
	void						SetSamplerMode(SamplerMode samplerMode);

	// depth mode
	void						CreateStencilTextureAndViewAndAllDepthStencilStates();
	// stencil depth property
	DepthMode					m_depthMode = DepthMode::ENABLED;
	DepthMode					m_desiredDepthMode = DepthMode::ENABLED;
	ID3D11DepthStencilState*	m_depthStencilStates[(int)(DepthMode::COUNT)] = {};
	ID3D11DepthStencilView*		m_depthStencilView = nullptr;
	ID3D11Texture2D*			m_depthStencilTexture = nullptr;

	// public shaders
	void	CreatePhongShader();
	public:
	Shader* m_PhongShader = nullptr;

	// public textures
	Texture const* m_defaultWhiteTexture = nullptr;
	Texture const* m_defaultBlackTexture = nullptr;

	protected:
		void* m_rc = nullptr; // Gfx API rendering context: "HGLRC" in Windows/OpenGL

		std::vector<Shader*> m_loadedShaders;// cache pattern
		Shader* m_currentShader = nullptr;
		Shader* m_defaultShader = nullptr;

		ID3D11RenderTargetView* m_renderTargetView = nullptr;
		ID3D11Device* m_device = nullptr;
		ID3D11DeviceContext* m_deviceContext = nullptr;

		IDXGISwapChain* m_windowsSwapChain = nullptr; // windows swapchain


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// OpenXR xrRenderer
#ifdef VR_MODE
public:
	void OpenXRShutDown();
	std::vector<std::string> GetInstanceExtensions() const { return { XR_KHR_D3D11_ENABLE_EXTENSION_NAME }; }

	void InitializeDevice(XrInstance instance, XrSystemId systemId);
	Microsoft::WRL::ComPtr<IDXGIAdapter1> GetAdapter(LUID adapterId);
	void InitializeD3D11DeviceForAdapter(IDXGIAdapter1* adapter, const std::vector<D3D_FEATURE_LEVEL>& featureLevels,
		ID3D11Device** device, ID3D11DeviceContext** deviceContext);

	void XRBeginFrame(XrSession session);
	XrFrameState m_frameState{ XR_TYPE_FRAME_STATE };
	std::vector<XrCompositionLayerBaseHeader*> m_layers;

	ID3D11Texture2D* GetXRSwapchainTexture(XrSwapchain handle);
	void EndFrameFromXRtoWindows();

	void RenderXRSwapchainImageToWindow(const XrSwapchainImageBaseHeader* swapchainImage, Camera* camera);
	Texture* m_XRCopiedTexture = nullptr;

	void CreateSwapchains(XrSession XRSession, XrInstance XRInstance, XrSystemId XRSystemId, const std::shared_ptr<const Options> programOptions);
	int64_t SelectColorSwapchainFormat(const std::vector<int64_t>& runtimeFormats) const ;
	uint32_t GetSupportedSwapchainSampleCount(const XrViewConfigurationView&)  { return 1; }
	std::vector<XrSwapchainImageBaseHeader*> AllocateSwapchainImageStructs(
		uint32_t capacity, const XrSwapchainCreateInfo& /*swapchainCreateInfo*/);

	ComPtr<ID3D11DepthStencilView> GetDepthStencilView(ID3D11Texture2D* colorTexture);
	// Map color buffer to associated depth buffer. This map is populated on demand.
	std::map<ID3D11Texture2D*, ComPtr<ID3D11DepthStencilView>> m_colorToDepthMap;

	void SetClearScreenColor(const Rgba8& clearColor);
	Vec4 m_clearScreenColor;

	void BeginOpenXRCamera(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageBaseHeader* swapchainImage,
		int64_t swapchainFormat, uint32_t eyeIndex);

	const XrBaseInStructure* GetGraphicsBinding() const;
	XrGraphicsBindingD3D11KHR m_graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	std::list<std::vector<XrSwapchainImageD3D11KHR>> m_swapchainImageBuffers;

	// swapchain
	std::vector<Swapchain> m_XRSwapchains;
	std::vector<XrView> m_views;
	std::vector<XrViewConfigurationView> m_configViews;
	std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
	// int64_t m_colorSwapchainFormat{ -1 };
	int64_t m_colorSwapchainFormat{ DXGI_FORMAT_R8G8B8A8_UNORM };

	Vec3 m_headOffsetPos = Vec3::ZERO;
	// Quat m_headOffsetQuat = Quat();
	EulerAngles m_headOffsetOrientation;

	Mat44 m_VRCameraMatrix[2] = { Mat44(), Mat44() };
#endif
};