#include "Engine/Renderer/Texture.hpp"
#include "Engine/core/EngineCommon.hpp"
#include <d3d11.h>


Texture::Texture()
{

}

Texture::Texture(std::string textureName, IntVec2 dimensions, ID3D11Texture2D* d3dTexture, ID3D11ShaderResourceView* d3dShaderResourceView, ID3D11RenderTargetView* d3dRenderTargetView)
	: m_name(textureName)
	, m_dimensions(dimensions)
	, m_texture(d3dTexture)
	, m_shaderResourceView(d3dShaderResourceView)
	, m_renderTargetView(d3dRenderTargetView)
{

}

Texture::~Texture()
{
	DX_SAFE_RELEASE(m_texture);
	DX_SAFE_RELEASE(m_shaderResourceView);
	DX_SAFE_RELEASE(m_renderTargetView);
}
