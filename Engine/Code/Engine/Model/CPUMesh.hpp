#pragma once

#include "Engine/Math/Mat44.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include <vector>
#include <string>

class CPUMesh
{
public:
	CPUMesh();
	CPUMesh(std::string const& objFileName, Mat44 const& transform = Mat44());
	virtual ~CPUMesh();

	void Load(std::string const& objFileName, Mat44 const& transform);

	std::vector<Vertex_PCUTBN> m_vertexes;
	std::vector<unsigned int> m_indexes;

	bool m_loadingResult = false;
	bool m_hasNormals = false;
	bool m_hasUVs = false;
};