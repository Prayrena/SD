#include "Engine/VFX/EffectSystem.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/core/DevConsole.hpp"

#include <algorithm>

extern EffectSystem* g_theEffectSystem;
extern Renderer* g_theRenderer;
extern Clock*	g_theGameClock;
extern RandomNumberGenerator* g_rng;
extern DevConsole* g_theDevConsole;

Emitter::Emitter(SpriteAnimDefinition const& particleAnim, Vec3 pos, Vec3 direction, float spreadAngleDegrees /*= 0.f*/, 
				int numParticles /*= 1*/, float emitterLifeTime /*= 1.f*/)
		: m_particleAnim(&particleAnim)
		, m_position(pos)
		, m_direction(direction)
		, m_spread(spreadAngleDegrees)
		, m_particleCount(numParticles)
{
	m_lifeTimer = new Timer(emitterLifeTime, g_theGameClock);
	m_lifeTimer->Start();
}

Emitter::Emitter(std::string text, Vec3 pos, Vec3 direction, int numParticles /*= 1*/, float emitterLifeTime /*= 1.f*/)
	: m_text(text)
	, m_position(pos)
	, m_direction(direction)
	, m_particleCount(numParticles)
{
	m_lifeTimer = new Timer(emitterLifeTime, g_theGameClock);
	m_lifeTimer->Start();
}

Emitter::~Emitter()
{
	if (m_lifeTimer)
	{
		delete m_lifeTimer;
	}

	if (m_particleAnim)
	{
		delete m_particleAnim;
	}
}

void Emitter::Update()
{
	// when this emitter has not ended, check if its particles not achieve the capacity
	if (!(m_lifeTimer->HasPeroidElapsed()) && g_theEffectSystem->GetPartciclesCountForEmitter(this) < m_particleCount)
	{
		while (g_theEffectSystem->GetPartciclesCountForEmitter(this) < m_particleCount)
		{
			// using either way, we are copy the instance from stack/heap memory, because the std::vector<Particle>	m_particles is consistent on the memory
			
			// Particle newParticle = Particle(*this);
			// g_theEffectSystem->m_particles.push_back(newParticle);	// this is pushing a copied Particle instance
			Particle* newParticle = new Particle(*this);
			g_theEffectSystem->m_particles.push_back(*newParticle);		// this is pushing a copied Particle instance

			// get the particle we just pushed
			Particle* particlePtr = &g_theEffectSystem->m_particles.back();
			particlePtr->Startup();		// this avoids deconstructor issue
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
float Emitter::GetSizeScale(Particle* p) const
{
	return m_sizeScale_startEnd.EvaluateAtParametric(p->m_lifeTimer->GetElapsedFraction());
}

float Emitter::GetMovingSpeedScale(Particle* p) const
{
	return m_movingSpeedScale_startEnd.EvaluateAtParametric(p->m_lifeTimer->GetElapsedFraction());
}

float Emitter::GetRotationSpeedScale(Particle* p) const
{
	return m_rotationSpeedScale_startEnd.EvaluateAtParametric(p->m_lifeTimer->GetElapsedFraction());
}

float Emitter::GetAlpha(Particle* p) const
{
	return m_alpha_startEnd.EvaluateAtParametric(p->m_lifeTimer->GetElapsedFraction());
}

Rgba8 Emitter::GetColor(Particle* p) const
{
	float fraction = p->m_lifeTimer->GetElapsedFraction();
	return InterpolateRGB(m_colorRange_start, m_colorRange_end, fraction);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Emitter* Emitter::SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange range)
{
	m_sizeScale_startEnd = range;
	return this;
}

Emitter* Emitter::SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange range)
{
	m_movingSpeedScale_startEnd = range;
	return this;
}

Emitter* Emitter::SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange range)
{
	m_rotationSpeedScale_startEnd = range;
	return this;
}

Emitter* Emitter::SetParticle_Alpha_FromStartToEnd(unsortedFloatRange range)
{
	m_alpha_startEnd = range;
	return this;
}

Emitter* Emitter::SetParticle_Color_FromStartToEnd(Rgba8 start, Rgba8 end)
{
	m_colorRange_start = start;
	m_colorRange_end = end;
	return this;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Emitter* Emitter::SetParticle_Size_StartRange(FloatRange range)
{
	m_size_RandomRange = range;
	return this;
}

Emitter* Emitter::SetParticle_MovingSpeed_StartRange(FloatRange range)
{
	m_movingSpeed_RandomRange = range;
	return this;
}

Emitter* Emitter::SetParticle_Rotation_StartRange(FloatRange range)
{
	m_rotationRange = range;
	return this;
}


Emitter* Emitter::SetParticle_RotationSpeed_StartRange(FloatRange range)
{
	m_rotationSpeed_RandomRange = range;
	return this;
}

Emitter* Emitter::SetParticle_LifeTime_Range(FloatRange range)
{
	m_lifeTimeRange = range;
	return this;
}

Emitter* Emitter::SetParticle_Offset_StartRange(FloatRange range)
{
	m_offsetRange = range;
	return this;
}

Vec3 Emitter::GetRandomDirectionInSpreadCone() const
{
	FloatRange yawRange(-m_spread, m_spread);
	FloatRange pitchRange(-m_spread, m_spread);

	float yawDeflection = g_rng->RollRandomFloatInFloatRange(yawRange);
	float pitchDeflection = g_rng->RollRandomFloatInFloatRange(pitchRange);

	EulerAngles orientation = m_direction.GetOrientation();
	orientation.m_yawDegrees += yawDeflection;
	orientation.m_pitchDegrees += pitchDeflection;

	return Vec3::GetDirectionForYawPitch(orientation.m_yawDegrees, orientation.m_pitchDegrees);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
bool Emitter::HasLifeEnded() const
{
	bool dead = m_lifeTimer->HasPeroidElapsed();
	return dead;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Particle::Particle(Emitter& emitter)
		: m_emitter(&emitter)
{
}

Particle::Particle(const Particle& other)
	: m_particleVertexes(other.m_particleVertexes)
	, m_position(other.m_position)
	, m_direction(other.m_direction)
	, m_rotation(other.m_rotation)
	, m_originalMovingSpeed(other.m_originalMovingSpeed)
	, m_rotationSpeed(other.m_rotationSpeed)
	, m_color(other.m_color)
	, m_modelMatrix(other.m_modelMatrix)
	, m_emitter(other.m_emitter)
	, m_originalSize(other.m_originalSize)
{
	if (other.m_lifeTimer)
	{
		m_lifeTimer = new Timer(*other.m_lifeTimer); // Deep copy, otherwise we just copy a particle pointer, will make a double deletion on the life timer
	}
	else
	{
		m_lifeTimer = nullptr;
	}

	m_emitter = other.m_emitter;
}

Particle::Particle(Particle&& other) noexcept
	: m_particleVertexes(other.m_particleVertexes)
	, m_position(other.m_position)
	, m_direction(other.m_direction)
	, m_rotation(other.m_rotation)
	, m_originalMovingSpeed(other.m_originalMovingSpeed)
	, m_rotationSpeed(other.m_rotationSpeed)
	, m_color(other.m_color)
	, m_modelMatrix(other.m_modelMatrix)
	, m_emitter(other.m_emitter)
	, m_originalSize(other.m_originalSize)
{
	// Clean up current resources
	if (m_lifeTimer)
	{
		delete m_lifeTimer;
	}

	// Transfer ownership
	m_lifeTimer = other.m_lifeTimer;
	m_emitter = other.m_emitter;

	other.m_lifeTimer = nullptr; // Nullify the source pointer
}

Particle& Particle::operator=(Particle&& other) noexcept
{
	// Clean up current resources
	if (m_lifeTimer)
	{
		delete m_lifeTimer;
	}

	// Transfer ownership
	m_lifeTimer = other.m_lifeTimer;
	m_emitter = other.m_emitter;

	other.m_lifeTimer = nullptr; // Nullify the source pointer

	m_particleVertexes = (other.m_particleVertexes);
	m_position = (other.m_position);
	m_direction = (other.m_direction);
	m_rotation = (other.m_rotation);
	m_originalMovingSpeed = (other.m_originalMovingSpeed);
	m_rotationSpeed = (other.m_rotationSpeed);
	m_color = (other.m_color);
	m_modelMatrix = (other.m_modelMatrix);
	m_emitter = (other.m_emitter);
	m_originalSize = (other.m_originalSize);

	return *this;
}

Particle::~Particle()
{
	if (m_lifeTimer)
	{
		delete m_lifeTimer;
		m_lifeTimer = nullptr;
	}

	// if (m_particleVertexBuffer)
	// {
	// 	delete m_particleVertexBuffer;
	// 	m_particleVertexBuffer = nullptr;
	// }
	// if (m_particleIndexBuffer)
	// {
	// 	delete m_particleIndexBuffer;
	// 	m_particleIndexBuffer = nullptr;
	// }
}

void Particle::Startup()
{
	if (m_emitter->m_offsetRange.GetRangeLength() != 0.f)
	{
		float offset = g_rng->RollRandomFloatInFloatRange(m_emitter->m_offsetRange);
		m_position = m_emitter->m_position + Vec3(offset, offset, offset);
	}
	else
	{
		m_position = m_emitter->m_position;
	}

	m_originalMovingSpeed = g_rng->RollRandomFloatInFloatRange(m_emitter->m_movingSpeed_RandomRange);
	m_direction = m_emitter->GetRandomDirectionInSpreadCone();

	float lifeTime = g_rng->RollRandomFloatInFloatRange(m_emitter->m_lifeTimeRange);
	m_lifeTimer = new Timer(lifeTime, g_theGameClock);
	m_lifeTimer->Start();

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// get a random size in the range and generate vertex and index buffer
	m_originalSize = g_rng->RollRandomFloatInFloatRange(m_emitter->m_size_RandomRange);

	if (m_emitter->m_text == "undefined")
	{
		float halfSize = m_originalSize * 0.5f;
		// quad normal facing +x direction
		Vec3 BL = Vec3(0.f, -halfSize, -halfSize);
		Vec3 BR = Vec3(0.f, halfSize, -halfSize);
		Vec3 TR = Vec3(0.f, halfSize, halfSize);
		Vec3 TL = Vec3(0.f, -halfSize, halfSize);

		// random rotation
		AABB2 uvBounds = m_emitter->m_particleAnim->GetSpriteDefAtTime(0.f).GetUVs();
		AddVertsForQuad3D(m_particleVertexes, BL, BR, TR, TL, m_color, uvBounds);
	}

	if (m_emitter->m_rotationRange.GetRangeLength() != 0.f)
	{
		float angleDegrees = g_rng->RollRandomFloatInFloatRange(m_emitter->m_rotationRange);
		m_rotation = angleDegrees;
		m_rotationSpeed = g_rng->RollRandomFloatInFloatRange(m_emitter->m_rotationSpeed_RandomRange);
	}
}

void Particle::Update()
{
	// alpha
	float alpha = m_emitter->GetAlpha(this);
	unsigned char alphaByte = DenormalizeByte(alpha);
	m_color = m_emitter->GetColor(this);
	m_color.a = alphaByte;

	float deltaSeconds = m_lifeTimer->GetDeltaSecondsFromClock();

	// position and scale
	float scale = m_emitter->GetSizeScale(this);
	float movingSpeedScale = m_emitter->GetMovingSpeedScale(this);
	float currentSpeed = m_originalMovingSpeed * movingSpeedScale;
	m_position += deltaSeconds * currentSpeed * m_direction;
	// we use matrix to move and scale
	m_modelMatrix = GetBillboardMatrix(m_billboardType, g_theEffectSystem->m_trackingCamera->GetModelMatrix(), m_position, Vec3(1.f, scale, scale));
	// EulerAngles orientation = m_modelMatrix.GetIBasis3D().GetOrientation();
	// orientation.m_rollDegrees = m_rotation;
	EulerAngles orientation(0.f, 0.f, m_rotation);
	m_modelMatrix.Append(orientation.GetAsMatrix_XFwd_YLeft_ZUp());

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// rotation - we rotate the verts
	float rotationSpeedScale = m_emitter->GetRotationSpeedScale(this);
	float currentRotationSpeed = m_rotationSpeed * rotationSpeedScale;
	m_rotation += deltaSeconds * currentRotationSpeed;
	if (rotationSpeedScale != 0.f)
	{
		// if (m_particleVertexBuffer)
		// {
		// 	delete m_particleVertexBuffer;
		// 	m_particleVertexBuffer = nullptr;
		// }
		// if (m_particleIndexBuffer)
		// {
		// 	delete m_particleIndexBuffer;
		// 	m_particleIndexBuffer = nullptr;
		// }

		// TransformVertexArrayXY3D((int)m_particleVertexes.size(), m_particleVertexes.data(), 1.f, m_rotation);

		// if (m_emitter->m_sizeScale_startEnd.GetRangeLength() != 0.f)
		// {
		// 	float scale = m_emitter->GetSizeScale(this);
		// 	TransformVertexArrayXY3D((int)m_particleIndexArray.size(), m_particleVertexes.data(), m_rotation, Vec2(), scale);
		// }
		// else
		// {
		// 	
		// }

		// m_particleVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_particleVertexes.size()), sizeof(Vertex_PCUTBN));
		// m_particleIndexBuffer = g_theRenderer->CreateIndexBuffer((size_t)(m_particleIndexArray.size()));
		// 
		// size_t vertexSize = sizeof(Vertex_PCUTBN);
		// size_t vertexArrayDataSize = (m_particleVertexes.size()) * vertexSize;
		// g_theRenderer->CopyCPUToGPU(m_particleVertexes.data(), vertexArrayDataSize, m_particleVertexBuffer);
		// 
		// size_t indexSize = sizeof(int);
		// size_t indexArrayDataSize = m_particleIndexArray.size() * indexSize;
		// g_theRenderer->CopyCPUToGPU(m_particleIndexArray.data(), indexArrayDataSize, m_particleIndexBuffer);
	}


	// add verts
	m_particleVertexes.clear();
	m_particleVertexes.reserve(6);

	float halfSize = m_originalSize * 0.5f;
	Vec3 BL = Vec3(0.f, -halfSize, -halfSize);
	Vec3 BR = Vec3(0.f, halfSize, -halfSize);
	Vec3 TR = Vec3(0.f, halfSize, halfSize);
	Vec3 TL = Vec3(0.f, -halfSize, halfSize);

	if (m_emitter->m_text == "undefined")
	{
		AABB2 uvBounds = m_emitter->m_particleAnim->GetSpriteDefAtTime(m_lifeTimer->GetElapsedTime()).GetUVs();
		AddVertsForQuad3D(m_particleVertexes, BL, BR, TR, TL, m_color, uvBounds);
	}
	else
	{
		g_theDevConsole->m_config.m_font->AddVertsForText3DAtOriginXForward(m_particleVertexes, m_originalSize, m_emitter->m_text, m_color, 0.5f, Vec2(0.5f, 0.5f));
	}
}

void Particle::Render() const
{
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	if (m_emitter->m_text == "undefined")
	{
		g_theRenderer->BindTexture(&(m_emitter->m_particleAnim->GetSpriteDefAtTime(m_lifeTimer->GetElapsedTime()).GetTexture()));
	}
	else
	{
		g_theRenderer->BindTexture(&g_theDevConsole->m_config.m_font->GetTexture());
	}
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	g_theRenderer->SetDepthMode(DepthMode::READ_ONLY_LESS_EQUAL);
	g_theRenderer->BindShader(m_emitter->m_shader);
	g_theRenderer->SetModelConstants(m_modelMatrix, m_color);
	// g_theRenderer->DrawVertexAndIndexBuffer(m_particleVertexBuffer, m_particleIndexBuffer, (int)m_particleIndexArray.size());
	g_theRenderer->DrawVertexArray((int)m_particleVertexes.size(), m_particleVertexes.data());
}

bool Particle::HasLifeEnded() const
{
	bool dead = m_lifeTimer->HasPeroidElapsed();
	return dead;
}

void EffectSystem::Render() const
{
	for (int i = 0; i < (int)m_particles.size(); ++i)
	{
		m_particles[i].Render();
	}
}

void EffectSystem::Update()
{
	UpdateEmitters();
	UpdateParticles();
	SortParticlesRenderingOrder();
}

void EffectSystem::UpdateEmitters()
{
	erase_if(m_emitters, [](Emitter* emitter) 
	{
		return emitter->HasLifeEnded(); // Replace with your condition
	});

	for (auto emitter : m_emitters)
	{
		emitter->Update();
	}
}

void EffectSystem::UpdateParticles()
{
	if (!m_particles.empty())
	{
		erase_if(m_particles, [](const Particle& particle) 
		{
			return particle.HasLifeEnded();
		});
	}

	for (int i = 0; i < (int)m_particles.size(); ++i)
	{
		m_particles[i].Update();
	}
}

int EffectSystem::GetPartciclesCountForEmitter(Emitter* emitter) const
{
	unsigned int num = 0;
	for (int i = 0; i < (int)m_particles.size(); ++i)
	{
		if (m_particles[i].m_emitter == emitter)
		{
			++num;
		}
	}

	return num;
}

EffectSystem* EffectSystem::AddEmitter(Emitter*& emitter)
{
	m_emitters.push_back(emitter);
	return this;
}

void EffectSystem::Shutdown()
{
	for (int i = 0; i < (int)m_emitters.size(); ++i)
	{
		if (m_emitters[i])
		{
			delete m_emitters[i];
			m_emitters[i] = nullptr;
		}
	}
}

void EffectSystem::UpdateTrackingCamera(Camera& camera)
{
	m_trackingCamera = &camera;
}

void EffectSystem::SortParticlesRenderingOrder()
{
	Mat44 cameraTransformMat = m_trackingCamera->GetModelMatrix();
	Vec3  cameraPos = cameraTransformMat.GetTranslation3D();
	Vec3  cameraForward = cameraTransformMat.GetIBasis3D();

	std::sort
	(
		m_particles.begin(),
		m_particles.end(),
		[&](Particle& A, Particle& B)
		{
			// Method 1: use dist to sort the rendering order
			// float dist_A = (A.GetPosition - cameraPos).GetLengthSquared();
			// float dist_B = (B.GetPosition - cameraPos).GetLengthSquared();

			// Method 2: use the dist projected on the camera forward direction
			float dist_A = DotProduct3D((A.m_position - cameraPos), cameraForward);
			float dist_B = DotProduct3D((B.m_position - cameraPos), cameraForward);

			return dist_A < dist_B;
		}
	);
}

bool EffectSystem::Command_GenerateDamageEmitter(EventArgs& args)
{
	int damage = args.GetValue("damageAmount", 0);
	float duration = args.GetValue("showDuration", 1.f);
	Vec3 position = args.GetValue("damagePosition", Vec3());
	g_theEffectSystem->GenerateDamageNumberEmitter(position, damage, duration);
	return true;
}

void EffectSystem::GenerateDamageNumberEmitter(Vec3 const& position, int amount, float duration)
{
	Emitter* explosionEmitter = new Emitter(Stringf("%i", amount), position, Vec3(0.f, 0.f, 1.f), 1, 0.1f);
	explosionEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 1.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 1.f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(0.f, 0.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::RED, Rgba8::RED)->
		SetParticle_Size_StartRange(FloatRange(0.6f, 0.6f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.2f, 0.2f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 0.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(0.f, 0.f))->
		SetParticle_LifeTime_Range(FloatRange(duration, duration))->
		SetParticle_Offset_StartRange(FloatRange(0.f, 0.f));

	g_theEffectSystem->AddEmitter(explosionEmitter);
}

void EffectSystem::Startup()
{
	SubscribeEventCallbackFunction("GenerateDamageEmitter", EffectSystem::Command_GenerateDamageEmitter);
}
