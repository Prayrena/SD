#include "Engine/Model/CPUMesh.hpp"
#include "Engine/Model/ObjLoader.hpp"

CPUMesh::CPUMesh(std::string const& objFileName, Mat44 const& transform /*= Mat44()*/)
{
	m_loadingResult = ObjLoader::Load(objFileName, m_vertexes, m_indexes, m_hasNormals, m_hasUVs, transform);
}

CPUMesh::~CPUMesh()
{

}
