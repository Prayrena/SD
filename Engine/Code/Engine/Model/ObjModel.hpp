#pragma once

#include "Engine/Model/BaseModel.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Model/CPUMesh.hpp"
#include "Engine/Model/GPUMesh.hpp"
#include <string>

class Material;

class ObjModel : public BaseModel
{
public:
	ObjModel(Vec3 const& pos = Vec3::ZERO);
	virtual ~ObjModel();

	virtual void Update() override;
	virtual void Render() const;

	bool LoadXml(std::string const& fileName);
	bool LoadObj(std::string const& fileName, Mat44 const& transform = Mat44());

	Material* GetMaterial() const;

protected:
	std::string m_objectFileName;

	CPUMesh*	m_cpuMesh = nullptr;
	GPUMesh*	m_gpuMesh = nullptr;
	Material*	m_material = nullptr;

	void CreateDebugTangentBasisVectors();
	void CreateDebugVertexBuffer();

	std::vector<Vertex_PCU> m_debugVertexes;
	VertexBuffer* m_debugVertexBuffer = nullptr;
};