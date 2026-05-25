#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Model/Material.hpp"
#include "Game/Prop.hpp"

extern Renderer* g_theRenderer;
extern Game* g_theGame;
extern Clock* g_theColorChangingClock;
extern Clock* g_theGameClock;

Prop::Prop()
	:Entity(g_theGame, Vec3())
{

}

Prop::~Prop()
{
	if (m_vertexBuffer)
	{
		delete m_vertexBuffer;
	}

	if (m_debugVertexBuffer)
	{
		delete m_debugVertexBuffer;
	}

	if (m_indexBuffer)
	{
		delete m_indexBuffer;
	}
}

void Prop::CreateVertexAndIndexBuffer()
{
	// create vertex buffer and index buffer
	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(size_t(m_PCUTBNVertexes.size()), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(size_t(m_indexes.size()));

	size_t vertexSize = sizeof(Vertex_PCUTBN);
	size_t vertexArrayDataSize = (m_PCUTBNVertexes.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_PCUTBNVertexes.data(), vertexArrayDataSize, m_vertexBuffer);

	size_t indexSize = sizeof(int);
	size_t numIndexes = (int)m_indexes.size();
	size_t indexArrayDataSize = numIndexes * indexSize;
	g_theRenderer->CopyCPUToGPU(m_indexes.data(), indexArrayDataSize, m_indexBuffer);

	CreateDebugVertexBuffer();
}

void Prop::CreateDebugTangentBasisVectors()
{
	for (int i = 0; i < (int)m_PCUTBNVertexes.size(); ++i)
	{
		Vertex_PCU originPt_R = GetPCUFromPCUTBN(m_PCUTBNVertexes[i]);
		originPt_R.m_color = Rgba8::RED;		 
		Vertex_PCU originPt_G = GetPCUFromPCUTBN(m_PCUTBNVertexes[i]);
		originPt_G.m_color = Rgba8::GREEN;		 
		Vertex_PCU originPt_B = GetPCUFromPCUTBN(m_PCUTBNVertexes[i]);
		originPt_B.m_color = Rgba8::BLUE;

		Vertex_PCU normalEndPt(originPt_R.m_position + m_PCUTBNVertexes[i].m_normal * 0.1f, Rgba8::BLUE);
		Vertex_PCU tangentEndPt(originPt_R.m_position + m_PCUTBNVertexes[i].m_tangent * 0.1f, Rgba8::RED);
		Vertex_PCU bitangentEndPt(originPt_R.m_position + m_PCUTBNVertexes[i].m_bitangent * 0.1f, Rgba8::GREEN);

		m_debugVertexes.push_back(originPt_B);
		m_debugVertexes.push_back(normalEndPt);

		m_debugVertexes.push_back(originPt_R);
		m_debugVertexes.push_back(tangentEndPt);

		m_debugVertexes.push_back(originPt_G);
		m_debugVertexes.push_back(bitangentEndPt);
	}
}

void Prop::CreateDebugVertexBuffer()
{
	m_debugVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugVertexes.size()), sizeof(Vertex_PCU), true);

	size_t vertexSize = sizeof(Vertex_PCU);
	size_t vertexArrayDataSize = (m_debugVertexes.size()) * vertexSize;
	g_theRenderer->CopyCPUToGPU(m_debugVertexes.data(), vertexArrayDataSize, m_debugVertexBuffer);	
}

void Prop::Update()
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();

	// rotating around z axis
	m_angularVelocity.m_yawDegrees = (m_angularVelocity.m_yawDegrees * deltaSeconds);

	m_orientation += m_angularVelocity;
}

void Prop::Render() const
{
	if (m_name == "Sun")
	{
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);

		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);
		// transparent
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)m_unlitVertexes.size(), m_unlitVertexes.data());
		// opaque
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
		g_theRenderer->DrawVertexArray((int)m_unlitVertexes.size(), m_unlitVertexes.data());
	}
	else
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);

		if (m_material)
		{
			g_theRenderer->BindDiffuseSpecularNormalTextures(m_material->m_diffuseTexture, m_material->m_specGlossTexture, m_material->m_normalTexture);
			g_theRenderer->BindShader(g_theRenderer->CreateOrGetShader("Data/Shaders/Phong"));
			g_theRenderer->SetPhongLightingConstants(*g_theGame->m_phongLighinting);
			g_theRenderer->DrawVertexAndIndexBuffer(m_vertexBuffer, m_indexBuffer, int(m_indexes.size()));
		}
		else
		{
			g_theRenderer->BindTexture(m_unlitTexture);
			g_theRenderer->BindShader(nullptr);
			g_theRenderer->DrawVertexArray((int)m_unlitVertexes.size(), m_unlitVertexes.data());
		}
	}

	if (g_theGame->m_debugMode && !m_debugVertexes.empty())
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

void Prop::CreateCube()
{
	float halfSize = m_cubeSize * 0.5f;
	AddVertsForAABB3D(m_PCUTBNVertexes, m_indexes, AABB3(Vec3(-halfSize, -halfSize, -halfSize), Vec3(halfSize, halfSize, halfSize)), Rgba8::WHITE, AABB2::ZERO_TO_ONE);

	CalculateTangentSpaceBasisVectors(m_PCUTBNVertexes, m_indexes);

	CreateDebugTangentBasisVectors();
}

void Prop::CreateSphere()
{
	AddVertsForSphere3D(m_PCUTBNVertexes, m_indexes, Vec3(), m_sphereRadius);

	CalculateTangentSpaceBasisVectors(m_PCUTBNVertexes, m_indexes);

	CreateDebugTangentBasisVectors();
}

