#include "Engine/Model/ObjLoader.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/Time.hpp"
#include <cstdlib>
#include <unordered_map>
#include <iostream>


bool ObjLoader::GetMTLFileName(const std::vector<Strings>& tokensOfAllLines, std::string& mtlFName)
{
	mtlFName.clear();

	for (auto tokensOfEachLine : tokensOfAllLines) // mtllib Tank4.mtl
	{
		for (int tokenIndex = 0; tokenIndex < (int)(tokensOfEachLine.size()); ++tokenIndex) // mtllib Tank4.mtl
		{
			if (!strcmp(tokensOfEachLine[tokenIndex].c_str(), "mtllib") && (tokenIndex + 1) < (int)(tokensOfEachLine.size())) // if the token is mtllib, means there is the mtl file for the next token
			{
				std::string& token = tokensOfEachLine[tokenIndex + 1];
				for (int charIndex = 0; charIndex < (int)(token.size()); ++charIndex) // Tank4
				{
					if (token[charIndex] != '.')
					{
						mtlFName.push_back(token[charIndex]);
					}
					else
					{
						return true;
					}
				}
			}
			else
			{
				continue;
			}
		}
	}

	return false;
}

bool ObjLoader::ReadMTLColorInfo(std::string const& filePath, std::map<std::string, materialColorMap*>& mtlInfo)
{
	float debugMsgFontSize = 10.f;

	// Read the file in as a string
	std::string fileContent;
	int fileSize = FileReadToString(fileContent, filePath);
	if (fileSize <= 0)
	{
		std::string errorMsg = "Error: MTL File is empty";
		DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		return false;
	}

	// cut into each lines
	Strings lines;
	std::string normalizedFileContent = NormalizeLineEndings(fileContent);
	unsigned int numLines = SplitStringOnDelimiter(lines, normalizedFileContent, "\n");
	if (numLines <= 1)
	{
		std::string errorMsg = "Error: MTL file lack of info";
		DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		return false;
	}

	// each line has its token, which are a bunch of string
	std::vector<Strings> tokensOfAllLines; 
	GetTokensOfAllLines(lines, tokensOfAllLines);

	std::string currentMatName;   // Name of the current material
	materialColorMap* currentMatMap = nullptr;  // Temp structure to hold material properties
	for (int lineIndex = 0; lineIndex < (int)(tokensOfAllLines.size()); ++lineIndex)
	{
		Strings& tokensOfEachLine = tokensOfAllLines[lineIndex];
		if (tokensOfEachLine.empty())
		{
			continue;
		}

		const std::string& firstToken = tokensOfEachLine[0];
		if (firstToken == "newmtl" && tokensOfEachLine.size() >= 2)
		{
			for (int tokenIndex = 0; tokenIndex < (int)tokensOfEachLine.size(); ++tokenIndex)
			{
				std::string& token = tokensOfEachLine[tokenIndex];

				if (strcmp(token.c_str(), "newmtl") == 0) // update material map name
				{
					currentMatName = tokensOfEachLine[tokenIndex + 1];
					currentMatMap = new materialColorMap; // a new material map when reading newmtl
					currentMatMap->clear(); // clear material map info
					mtlInfo[currentMatName] = currentMatMap; // insert a new map, is this only making a copy???
				}
			}
		}
		else if (firstToken == "Kd" && (int)tokensOfEachLine.size() >= 4)
		{
			// vertex color
			Rgba8 vertexColor;
			if ((int)tokensOfEachLine.size() == 4)
			{
				Rgba8 color(
					DenormalizeByte(std::stof(tokensOfEachLine[1])),
					DenormalizeByte(std::stof(tokensOfEachLine[2])),
					DenormalizeByte(std::stof(tokensOfEachLine[3])),
					255 // Alpha default to 255
				);
				vertexColor = color;
			}
			else
			{
				Rgba8 color(
					DenormalizeByte(std::stof(tokensOfEachLine[1])),
					DenormalizeByte(std::stof(tokensOfEachLine[2])),
					DenormalizeByte(std::stof(tokensOfEachLine[3])),
					DenormalizeByte(std::stof(tokensOfEachLine[4]))
				);
				vertexColor = color;
			}
			(*currentMatMap)["Kd"] = vertexColor; // this will not change the mtlInfo ???
		}
		else
		{
			continue;
		}
	}

	if (mtlInfo.empty())
	{
		return false;
	}
	else
	{
		return true;
	}
}

// Different End-of-Line Conventions:
// On Windows, lines typically end with \r\n(carriage return +newline).
// On Unix / Linux / macOS, lines typically end with \n(newline only).
// If you are reading a file that was created or edited on Unix / Linux, the line endings would just be \n.
std::string ObjLoader::NormalizeLineEndings(std::string const& content)
{
	std::string normalizedContent = content;
	size_t pos = 0;

	// Replace all occurrences of \n with \r\n
	while ((pos = content.find("\r\n", pos)) != std::string::npos) 
	{
		normalizedContent.replace(pos, 1, "\n");
		pos += 1;
	}

	return normalizedContent;
}

bool ObjLoader::Load(std::string const& fileName, 
	std::vector<Vertex_PCUTBN>& outVertexes, std::vector<unsigned int>& outIndexes, 
	bool& outHasNormals, bool& outHasUVs, Mat44 const& transform /*= Mat44()*/)
{
	double timeAtStart = GetCurrentTimeSeconds();

	float debugMsgFontSize = 10.f;

	// Read the file in as a string
	std::string fileContent;
	int fileSize = FileReadToString(fileContent, fileName);
	if (fileSize <= 0) 
	{
		std::string errorMsg = "Error: File is empty";
		DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		return false;
	}

	// cut into each lines
	Strings lines;
	std::string normalizedFileContent = NormalizeLineEndings(fileContent);
	unsigned int numLines = SplitStringOnDelimiter(lines, normalizedFileContent, "\n");
	if (numLines <= 3)
	{
		std::string errorMsg = "Error: OBJ file lack of info";
		DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));

		return false;
	}

	std::vector<Strings> tokensOfAllLines; // each line has its token, which are a bunch of string
	GetTokensOfAllLines(lines, tokensOfAllLines);

	// check if the obj file has matched with any MTL file
	std::string mtlFName;
	std::map<std::string, materialColorMap*> mtlInfo;
	bool MTLisValid = true;
	if (!GetMTLFileName(tokensOfAllLines, mtlFName)) // no mtl file info inside the obj file
	{
		MTLisValid = false;
		std::string errorMsg = "Model have no related MTL file";
		DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
	}
	else // obj file appoint a MTL file
	{
		// split obj file path and make mtl file path
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];

		// Split the file path into components
		_splitpath_s(fileName.c_str(), drive, dir, fname, ext);

		const char* mtlExt = ".mtl";
		const char* mtlFname = mtlFName.c_str();

		char mtlFilePath[_MAX_PATH];
		_makepath_s(
			mtlFilePath,
			sizeof(mtlFilePath),
			drive,
			dir,
			mtlFname,
			mtlExt
		);

		if (!ReadMTLColorInfo(mtlFilePath, mtlInfo)) // read the mtl file info
		{
			MTLisValid = false;
			std::string errorMsg = "Cannot acquire material info from the MTL file";
			DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// parsing obj info
	// check if there is any normals in the content
	outHasNormals = false;
	for (auto tokensOfEachLine : tokensOfAllLines) // Strings
	{
		for (auto token : tokensOfEachLine) // std::string
		{
			if (!strcmp(token.c_str(), "vn"))
			{
				outHasNormals = true;
				break;
			}
		}
	}

	// check if there is any UV info in the content
	outHasUVs = false;
	for (auto tokensOfEachLine : tokensOfAllLines) // Strings
	{
		for (auto token : tokensOfEachLine) // std::string
		{
			if (!strcmp(token.c_str(), "vt"))
			{
				outHasUVs = true;
				break;
			}
		}
	}

	// vectors for store info to construct Vertex_PCUTBN
	std::vector<Vec3> posList;
	std::vector<Vec3> normalsList;
	std::vector<Vec2> UVsList;
	std::map<std::string, Rgba8> currentMat;
	std::vector<Face> facesList;
	
	for (auto tokensOfEachLine : tokensOfAllLines) // Strings
	{
		if (tokensOfEachLine.empty())
		{
			continue;
		}

		// position
		if (strcmp(tokensOfEachLine[0].c_str(), "v") == 0) 
		{
			//Vec3 pos(
			//	float(atof(tokensOfEachLine[1].c_str())),
			//	float(atof(tokensOfEachLine[2].c_str())),
			//	float(atof(tokensOfEachLine[3].c_str()))
			//	);
			Vec3 pos;
			pos.x = strtof(tokensOfEachLine[1].c_str(), nullptr);
			pos.y = strtof(tokensOfEachLine[2].c_str(), nullptr);
			pos.z = strtof(tokensOfEachLine[3].c_str(), nullptr);
			posList.push_back(pos);
		}

		// UVs
		else if (strcmp(tokensOfEachLine[0].c_str(), "vt") == 0) 
		{
			//Vec2 UV(
			//	float(atof(tokensOfEachLine[1].c_str())),
			//	float(atof(tokensOfEachLine[2].c_str()))
			//);
			Vec2 UV;
			UV.x = strtof(tokensOfEachLine[1].c_str(), nullptr);
			UV.y = strtof(tokensOfEachLine[2].c_str(), nullptr);
			UVsList.push_back(UV);
		}

		// Normals
		else if (strcmp(tokensOfEachLine[0].c_str(), "vn") == 0)
		{
			//Vec3 normal(
			//	float(atof(tokensOfEachLine[1].c_str())),
			//	float(atof(tokensOfEachLine[2].c_str())),
			//	float(atof(tokensOfEachLine[3].c_str()))
			//);
			Vec3 normal;
			normal.x = strtof(tokensOfEachLine[1].c_str(), nullptr);
			normal.y = strtof(tokensOfEachLine[2].c_str(), nullptr);
			normal.z = strtof(tokensOfEachLine[3].c_str(), nullptr);
			normal.GetNormalized();
			normalsList.push_back(normal);
		}
		 
		// Materials
		else if (strcmp(tokensOfEachLine[0].c_str(), "usemtl") == 0)
		{
			std::string matName = tokensOfEachLine[1];
			if (mtlInfo.find(matName) != mtlInfo.end())
			{
				currentMat = *mtlInfo[matName]; // update the material for the following faces to use
			}
		}

		// Faces
		else if (strcmp(tokensOfEachLine[0].c_str(), "f") == 0)
		{
			Face newFace;
			//  for the face documentation in obj, there are four cases
			//  f v1 v2 v3 ... (Vertex Indices Only)
			// 	Case1: refers to face definitions that only have vertex positions.Each v references a position in the posList.
			// 
			// 	f v1/vt1 v2/vt2 v3/vt3 ... (Vertex and Texture Coordinates)
			// 	Case2: In this case, each vertex has a corresponding texture coordinate.Texture indices(vt) point to entries in the UVsList.
			// 
			// 	f v1//vn1 v2//vn2 v3//vn3 ... (Vertex and Normals, No Textures)
			// 	Case3: This format defines faces with vertex and normal indices, but no texture coordinates.
			// 
			// 	f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ... (Vertex, Texture, and Normals)
			// 	Case4: This is the most detailed case, where each vertex has a position, texture coordinate, and normal.
			for (int tokenIndex = 1; tokenIndex < tokensOfEachLine.size(); ++tokenIndex)
			{
				Vertex vertex;
				std::string& token = tokensOfEachLine[tokenIndex];
				Strings indices = SplitStringOnDelimiter(token, '/');

				// Vertex position index (always present)
				// vertex.m_vertexPosIndex = atoi(indices[0].c_str()) - 1;
				vertex.m_vertexPosIndex = strtol(indices[0].c_str(), nullptr, 10) - 1;

				// not case 1: f v1 v2 v3 ...
				if (indices.size() > 1 && !indices.empty())
				{
					// case 4: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
					if (indices.size() == 3)
					{
						// vertex.m_vertexTextureIndex = atoi(indices[1].c_str()) - 1;
						// vertex.m_vertexNormalIndex = atoi(indices[2].c_str()) - 1; 
						
						vertex.m_vertexTextureIndex = strtol(indices[1].c_str(), nullptr, 10) - 1;
						vertex.m_vertexNormalIndex = strtol(indices[2].c_str(), nullptr, 10) - 1;
					}
					else if (indices.size() == 2) // case 2 or 3
					{
						size_t firstSlash = token.find('/');
						size_t secondSlash = token.find('/', firstSlash + 1);

						// todo:???????????? case 2: "f v1/vt1 v2/vt2 v3/vt3" or "v1/vt1/ v2/vt2/ v3/vt3/"
						if (firstSlash != std::string::npos && (secondSlash == std::string::npos || secondSlash == (token.size() - 1)))
						{
							// vertex.m_vertexTextureIndex = atoi(indices[1].c_str()) - 1;
							vertex.m_vertexTextureIndex = strtol(indices[1].c_str(), nullptr, 10) - 1;
						}
						// case 3: f v1//vn1 v2//vn2 v3//vn3 ... 
						else if (secondSlash == (firstSlash + 1))
						{
							// vertex.m_vertexNormalIndex = atoi(indices[1].c_str()) - 1;
							vertex.m_vertexNormalIndex = strtol(indices[1].c_str(), nullptr, 10) - 1;
						}
						else
						{
							std::string errorMsg = "Undocumented face format, please check the obj file";
							DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
							continue;
						}
					}
					else
					{
						std::string errorMsg = "Undocumented face format, please check the obj file";
						DebugAddMessage(errorMsg, 5.f, debugMsgFontSize, Rgba8::WHITE, Rgba8(255, 255, 255, 100));
						continue;
					}
				}
				newFace.m_color = currentMat["Kd"];
				newFace.vertexArray.push_back(vertex);
			}
			facesList.push_back(newFace);
		}		
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// constructing vertices and indices
	std::unordered_map<Vertex, int, KeyHash, KeyEqual>VertexMap = {};

	// reserve the size based on face nums
	int totalVertices = 0;
	for (const Face& face : facesList)
	{
		totalVertices += ((int)face.vertexArray.size() - 2) * 3;
	}
	outVertexes.reserve(totalVertices);

	for (auto const& face : facesList)
	{
		if (!face.vertexArray.empty())
		{
			// Faces are lists of vertexes in counter-clockwise winding order
			// They can specify any convex polygon, not just triangles
			for (int triIndex = 1; triIndex < ((int)face.vertexArray.size() - 1); ++triIndex)
			{
				Vertex const& v_0 = face.vertexArray[0];				// First vertex (fan point)
				Vertex const& v_1 = face.vertexArray[triIndex];			// Current vertex
				Vertex const& v_2 = face.vertexArray[triIndex + 1];		// Next vertex

				unsigned int i_0 = unsigned int(outVertexes.size());
				auto iter_0 = VertexMap.find(v_0);
				if (iter_0 == VertexMap.end()) // the vertex have not add before
				{
					outVertexes.push_back(AddVertexToVertexList(outVertexes, face.vertexArray[0], outHasUVs, outHasNormals, posList, normalsList, UVsList, face.m_color, transform));	
					VertexMap[v_0] = i_0;
				}
				else // the vertex have been added before
				{
					i_0 = iter_0->second;
				}
				outIndexes.push_back(i_0);

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				unsigned int i_1 = unsigned int(outVertexes.size());
				auto iter_1 = VertexMap.find(v_1);
				if (iter_1 == VertexMap.end()) // the vertex have not add before
				{
					outVertexes.push_back(AddVertexToVertexList(outVertexes, face.vertexArray[triIndex], outHasUVs, outHasNormals, posList, normalsList, UVsList, face.m_color, transform));
					VertexMap[v_1] = i_1;
				}
				else // the vertex have been added before
				{
					i_1 = iter_1->second;
				}
				outIndexes.push_back(i_1);

				//----------------------------------------------------------------------------------------------------------------------------------------------------
				unsigned int i_2 = unsigned int(outVertexes.size());
				auto iter_2 = VertexMap.find(v_2);
				if (iter_2 == VertexMap.end()) // the vertex have not add before
				{
					outVertexes.push_back(AddVertexToVertexList(outVertexes, face.vertexArray[triIndex + 1], outHasUVs, outHasNormals, posList, normalsList, UVsList, face.m_color, transform));
					VertexMap[v_2] = i_2;
				}
				else // the vertex have been added before
				{
					i_2 = iter_2->second;
				}
				outIndexes.push_back(i_2);

				
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// debug print to console
	double timeAtEnd = GetCurrentTimeSeconds();
	double timeElapsed = timeAtEnd - timeAtStart;

	DebuggerPrintf("*******************************************************\n");
	std::string UMsg = Stringf("Load .obj file: %s \n", fileName.c_str());
	DebuggerPrintf(UMsg.c_str());	
	 
	UMsg = Stringf("                                    positions: %i", posList.size());
	DebuggerPrintf(UMsg.c_str());	
	
	UMsg = Stringf(" UVs: ", UVsList.size());
	DebuggerPrintf(UMsg.c_str());	
	
	UMsg = Stringf(" Normals: ", normalsList.size());
	DebuggerPrintf(UMsg.c_str());
	
	UMsg = Stringf(" faces: %i\n", facesList.size());
	DebuggerPrintf(UMsg.c_str());

	UMsg = Stringf("                                    vertexes: %i", posList.size());
	DebuggerPrintf(UMsg.c_str());

	UMsg = Stringf(" triangles: %i", (outIndexes.size() / 3));
	DebuggerPrintf(UMsg.c_str());

	UMsg = Stringf(" time: %.8f\n", timeElapsed);
	DebuggerPrintf(UMsg.c_str());

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// normal process
	double timeAtStartNormal = GetCurrentTimeSeconds();

	if (!outHasNormals)
	{
		if (outHasUVs)
		{
			CalculateTangentSpaceBasisVectors(outVertexes, outIndexes, true, true);
		}
		else
		{
			CalculateTangentSpaceBasisVectors(outVertexes, outIndexes, true, false);
		}
	}
	else
	{
		if (outHasUVs)
		{
			CalculateTangentSpaceBasisVectors(outVertexes, outIndexes, false, true);
		}
	}

	double timeAtEnd_Normal = GetCurrentTimeSeconds();
	double timeElapsed_Normal = timeAtEnd_Normal - timeAtStartNormal;

	std::string CPUMsg = Stringf("Calculated tangent basis			%.8f\n", timeElapsed_Normal);
	DebuggerPrintf(CPUMsg.c_str());

	return true;
}

Vertex_PCUTBN ObjLoader::AddVertexToVertexList(std::vector<Vertex_PCUTBN>& outVertexes, Vertex const& vertexInfo, bool outHasUVs, bool outHasNormals, 
											std::vector<Vec3> const& posList, std::vector<Vec3> const& normalsList, std::vector<Vec2> const& UVsList, 
											Rgba8 vertexColor /*= Rgba8::WHITE*/, Mat44 const& transform /*= Mat44()*/)
{
	Vertex_PCUTBN newVertex;
	newVertex.m_position = transform.TransformPosition3D(posList[vertexInfo.m_vertexPosIndex]);
	newVertex.m_color = vertexColor;

	// newVertex.m_uvTexCoords = (outHasUVs) ? UVsList[vertexInfo.m_vertexTextureIndex] : Vec2();
	if (outHasUVs)
	{
		newVertex.m_uvTexCoords = UVsList[vertexInfo.m_vertexTextureIndex];
	}
	if (outHasNormals)
	{
		newVertex.m_normal = transform.TransformPosition3D(normalsList[vertexInfo.m_vertexNormalIndex]).GetNormalized();
	}
	else
	{
		newVertex.m_normal = normalsList[vertexInfo.m_vertexNormalIndex];
	}

	outVertexes.push_back(newVertex);
	return newVertex;
}

