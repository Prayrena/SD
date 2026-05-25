#pragma once

struct ID3D11Buffer;

class ConstantBuffer
{
	friend class Renderer;
	friend class XRRenderer;

protected:// only the renderer class has the right to create and manage this class
	ConstantBuffer(size_t size);
	ConstantBuffer(const ConstantBuffer& copy) = delete;
	virtual ~ConstantBuffer();

	ID3D11Buffer* m_buffer = nullptr;
	size_t		  m_size = 0;
};