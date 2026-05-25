#pragma once
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/Math/Mat44.hpp"
#include <string>
#include <vector>
#include <map>

typedef std::map<std::string, Rgba8> materialColorMap;

struct Vertex
{
	unsigned int m_vertexPosIndex = 0;
	unsigned int m_vertexTextureIndex = 0;
	unsigned int m_vertexNormalIndex = 0;
};

class Face
{
public:
	Rgba8 m_color = Rgba8::WHITE;
	std::vector<Vertex> vertexArray;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
// this structure is used for storing vertex and check if the face is using duplicated vertexes
struct Key
{
	Vertex first;
	Vertex_PCUTBN Second;
};

struct KeyHash
{
	std::size_t operator()(const Vertex& v) const
	{
		return ((std::hash<unsigned int>()(v.m_vertexPosIndex) ^
			(std::hash<unsigned int>()(v.m_vertexTextureIndex) << 1)) >> 1) ^
			(std::hash<unsigned int>()(v.m_vertexNormalIndex) << 1);
	}
};

struct KeyEqual
{
	bool operator()(const Vertex& lhs, const Vertex& rhs) const
	{
		return (lhs.m_vertexPosIndex == rhs.m_vertexPosIndex && 
		lhs.m_vertexTextureIndex == rhs.m_vertexTextureIndex && 
		lhs.m_vertexNormalIndex == rhs.m_vertexNormalIndex);
	}
};
//----------------------------------------------------------------------------------------------------------------------------------------------------

class ObjLoader
{
public:
	static bool GetMTLFileName(const std::vector<Strings>& tokensOfAllLines, std::string& mtlFName);
	static bool ReadMTLColorInfo(std::string const& filePath, std::map<std::string, materialColorMap*>& mtlInfo);
	static std::string NormalizeLineEndings(std::string const& content);
	static bool Load(std::string const& fileName,
		std::vector<Vertex_PCUTBN>& outVertexes, std::vector<unsigned int>& outIndexes,
		bool& outHasNormals, bool& outHasUVs, Mat44 const& transform = Mat44());
	static Vertex_PCUTBN AddVertexToVertexList(std::vector<Vertex_PCUTBN>& outVertexes, Vertex const& vertexInfo, bool outHasUVs, bool outHasNormals, 
			std::vector<Vec3> const& posList, std::vector<Vec3> const& normalsList, std::vector<Vec2> const& UVsList, Rgba8 vertexColor = Rgba8::WHITE, Mat44 const& transform = Mat44());
};