#include "Game/Model.hpp"
#include "Engine/Model/ObjLoader.hpp"
#include "Engine/Model/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <iostream>

extern Renderer* g_theRenderer;
extern Clock* g_theGameClock;
extern App*		g_theApp;

Model::Model(Game* game)
	:Entity(game)
{

}

Model::~Model()
{
	if (m_debugVertexBuffer)
	{
		delete m_debugVertexBuffer;
	}

	if (m_gpuMesh)
	{
		delete m_gpuMesh;
	}

	if (m_material)
	{
		delete m_material;
	}
}

void Model::Update()
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();

	// rotating around z axis
	m_angularVelocity.m_yawDegrees = (m_angularVelocity.m_yawDegrees * deltaSeconds);

	m_orientation += m_angularVelocity;
}

void Model::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);

	if (m_material)
	{
		g_theRenderer->BindDiffuseSpecularNormalTextures(m_material->m_diffuseTexture, m_material->m_specGlossTexture, m_material->m_normalTexture);
		g_theRenderer->BindShader(m_material->m_shader);
		g_theRenderer->SetPhongLightingConstants(*m_game->m_phongLighinting);
	}
	else
	{
		g_theRenderer->BindDiffuseSpecularNormalTextures(g_theRenderer->m_defaultWhiteTexture, g_theRenderer->m_defaultBlackTexture, g_theRenderer->m_defaultWhiteTexture);
		g_theRenderer->BindShader(g_theRenderer->m_PhongShader);
		PhongLightingConstants lightingConstant = *m_game->m_phongLighinting;
		lightingConstant.lighingDebug.UseNormalMap = false;
		g_theRenderer->SetPhongLightingConstants(lightingConstant);
	}

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	m_gpuMesh->Render(g_theRenderer);

	if (m_game->m_debugMode && !m_debugVertexes.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetModelConstants(GetModelMatrix());

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(m_debugVertexBuffer, (int)m_debugVertexes.size());
	}
}

bool Model::LoadXml(std::string const& fileName)
{
	XmlDocument objXml;
	char const* objXmlFilePath = fileName.c_str();
	XmlResult result = objXml.LoadFile(objXmlFilePath);

	if (!result == tinyxml2::XML_SUCCESS) // can not open the XML
	{
		return false;
	}

	XmlElement* objElement = objXml.RootElement();
	if (!objElement && strcmp( objElement->Name(), "Model") == 0 ) // the XML file does not have a root element
	{
		return false;
	}

	// those info needed to be read
	std::string objFileName;
	std::string materialXMLFilePath;
	Mat44* modelTransform = nullptr;

	// Get current xml file path info
	// split obj file path and make mtl file path
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	// Split the file path into components
	_splitpath_s(fileName.c_str(), drive, dir, fname, ext);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// obj file path
	std::string modelName = ParseXmlAttribute(*objElement, "name", "Model name not found"); // Model name="Cube_Textured"

	// char materialXMLFilePath[_MAX_PATH];

	std::string objFilePath = ParseXmlAttribute(*objElement, "path", "OBJ file path not found"); // path="Data/Models/Cube_Textured.obj"
	if (objFilePath != "OBJ file path not found")
	{
		//Strings filePathTokens = SplitStringOnDelimiter(objFilePath, '/');
		//std::string objFname;
		//for (int charIndex = 0; charIndex < filePathTokens[2].size(); ++charIndex)
		//{
		//	char singleChar = filePathTokens[2][charIndex];
		//	while (singleChar != '.')
		//	{
		//		objFname.push_back(singleChar);
		//	}
		//	continue;
		//}
		//
		//const char* objExt = ".obj";
		//const char* objFnameChar = objFname.c_str();
		//	
		//_makepath_s(
		//	materialXMLFilePath,
		//	sizeof(materialXMLFilePath),
		//	drive,
		//	dir,
		//	objFnameChar,
		//	objExt);
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// material file path
	materialXMLFilePath = ParseXmlAttribute(*objElement, "material", "Material file path not found"); // path="Data/Models/Cube_Textured.obj"
	if (materialXMLFilePath != "Material file path not found")
	{
		m_material = new Material;
		if (!m_material->Load(materialXMLFilePath))
		{
			ERROR_AND_DIE(Stringf("The %s file is missing", materialXMLFilePath.c_str()));
		}
	}

	// read transform info
	objElement = objElement->FirstChildElement();
	if ( strcmp(objElement->Name(), "Transform")  == 0 )
	{
		Vec3 xValue = ParseXmlAttribute(*objElement, "x", Vec3());
		Vec3 yValue = ParseXmlAttribute(*objElement, "y", Vec3());
		Vec3 zValue = ParseXmlAttribute(*objElement, "z", Vec3());
		Vec3 tValue = ParseXmlAttribute(*objElement, "t", Vec3());

		float scale = ParseXmlAttribute(*objElement, "scale", 1.f);

		modelTransform = new Mat44(xValue, yValue, zValue, tValue, scale);
	}

	// load OBJ
	if (modelTransform)
	{
		LoadObj(objFilePath, *modelTransform);

		return true;
	}
	else
	{
		LoadObj(objFilePath);

		return true;
	}
}

bool Model::LoadObj(std::string const& fileName, Mat44 const& transform /*= Mat44()*/)
{
	// update the model actor with the transform info pass in
	// Vec3 direction = transform.TransformPosition3D(Vec3(1.f, 0.f, 0.f)).GetNormalized();
	// m_orientation = direction.GetOrientation();

	double timeAtStart = GetCurrentTimeSeconds();

	m_cpuMesh = new CPUMesh(fileName, transform);

	double timeAtEnd_CPU = GetCurrentTimeSeconds();
	double timeElapsed_CPU = timeAtEnd_CPU - timeAtStart;

	std::string CPUMsg = Stringf("Created CPU mesh					%.8f\n", timeElapsed_CPU);
	DebuggerPrintf(CPUMsg.c_str());


	if (m_cpuMesh->m_loadingResult)
	{
		m_gpuMesh = new GPUMesh(m_cpuMesh, g_theRenderer);

		double timeAtEnd_GPU = GetCurrentTimeSeconds();
		double timeElapsed_GPU = timeAtEnd_GPU - timeAtEnd_CPU;
		std::string GPUMsg = Stringf("Created GPU mesh					%.8f\n", timeElapsed_GPU);
		DebuggerPrintf(GPUMsg.c_str());

		CreateDebugTangentBasisVectors();
		CreateDebugVertexBuffer();

		double timeAtEnd_Debug = GetCurrentTimeSeconds();
		double timeElapsed_Debug = timeAtEnd_Debug - timeElapsed_GPU;

		std::string debugMsg = Stringf("Created debug normals				%.8f\n",  timeElapsed_Debug);
		DebuggerPrintf(debugMsg.c_str());
		DebuggerPrintf("*******************************************************\n");

		return true;
	}
	else
	{
		return false;
	}
}

void Model::CreateDebugTangentBasisVectors()
{
	for (int i = 0; i < (int)m_cpuMesh->m_vertexes.size(); ++i)
	{
		Vertex_PCU originPt_R = GetPCUFromPCUTBN(m_cpuMesh->m_vertexes[i]);
		originPt_R.m_color = Rgba8::RED;		
		Vertex_PCU originPt_G = GetPCUFromPCUTBN(m_cpuMesh->m_vertexes[i]);
		originPt_G.m_color = Rgba8::GREEN;		
		Vertex_PCU originPt_B = GetPCUFromPCUTBN(m_cpuMesh->m_vertexes[i]);
		originPt_B.m_color = Rgba8::BLUE;

		Vertex_PCU normalEndPt(originPt_R.m_position + m_cpuMesh->m_vertexes[i].m_normal * 0.1f, Rgba8::BLUE);
		Vertex_PCU tangentEndPt(originPt_R.m_position + m_cpuMesh->m_vertexes[i].m_tangent * 0.1f, Rgba8::RED);
		Vertex_PCU bitangentEndPt(originPt_R.m_position + m_cpuMesh->m_vertexes[i].m_bitangent * 0.1f, Rgba8::GREEN);

		m_debugVertexes.push_back(originPt_B);
		m_debugVertexes.push_back(normalEndPt);		
		
		m_debugVertexes.push_back(originPt_R);
		m_debugVertexes.push_back(tangentEndPt); 
		
		m_debugVertexes.push_back(originPt_G);
		m_debugVertexes.push_back(bitangentEndPt);
	}
}

void Model::CreateDebugVertexBuffer()
{
	m_debugVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugVertexes.size()), sizeof(Vertex_PCU), true);

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_debugVertexes.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_debugVertexes.data(), vertexArrayDataSize, m_debugVertexBuffer);
}

