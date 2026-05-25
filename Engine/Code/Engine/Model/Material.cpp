#include "Engine/Model/Material.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"

extern Renderer* g_theRenderer;

Material::Material()
{
	
}

Material::~Material()
{
	// released in g_theRenderer shutdown
}

bool Material::Load(std::string const& xmlFileName)
{
	XmlDocument matXml;
	char const* objXmlFilePath = xmlFileName.c_str();
	XmlResult result = matXml.LoadFile(objXmlFilePath);

	if (!result == tinyxml2::XML_SUCCESS) // can not open the XML
	{
		return false;
	}

	XmlElement* matXMLElement = matXml.RootElement();

	if (strcmp(matXMLElement->Name(), "Material") == 0)
	{
		m_name = ParseXmlAttribute(*matXMLElement, "name", "Material name not defined"); // Model name="Cube_Textured"

		// get the vertex type
		m_vertexTypeName = ParseXmlAttribute(*matXMLElement, "vertexType", "Vertex not defined"); // vertexType="Vertex_PCUTBN"
		if (m_vertexTypeName != "Vertex not defined")
		{
			if (m_vertexTypeName == "Vertex_PCUTBN")
			{
				m_vertexType = VertexType::Vertex_PCUTBN;
			}
			else if (m_vertexTypeName == "Vertex_PCU")
			{
				m_vertexType = VertexType::Vertex_PCU;
			}
			else
			{
				ERROR_AND_DIE("The vertex type in the texture xml is not defined");
			}
		}

		// get the shader file path
		m_shaderName = ParseXmlAttribute(*matXMLElement, "shader", "Shader not defined"); // "Data/Shaders/Phong"
		if (m_shaderName != "Shader not defined")
		{
			m_shader = g_theRenderer->CreateOrGetShader(m_shaderName.c_str(), m_vertexType);
		}

		m_diffuseTextureName = ParseXmlAttribute(*matXMLElement, "diffuseTexture", "Texture not defined"); // "diffuseTexture="Data/Images/Grass_Diffuse.png"
		if (m_shaderName != "Texture not defined")
		{
			m_diffuseTexture = g_theRenderer->CreateOrGetTextureFromFile(m_diffuseTextureName.c_str());
		}		
		
		m_normalTextureName = ParseXmlAttribute(*matXMLElement, "normalTexture", "Texture not defined"); 
		if (m_shaderName != "Texture not defined")
		{
			m_normalTexture = g_theRenderer->CreateOrGetTextureFromFile(m_normalTextureName.c_str());
		}		
		
		m_specGlossTextureName = ParseXmlAttribute(*matXMLElement, "specGlossEmitTexture", "Texture not defined");
		if (m_shaderName != "Texture not defined")
		{
			m_specGlossTexture = g_theRenderer->CreateOrGetTextureFromFile(m_specGlossTextureName.c_str());
		}

		return true;
	}
	else
	{
		return false;
	}
}
