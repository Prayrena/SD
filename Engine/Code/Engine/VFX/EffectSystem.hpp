#pragma once
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"

class	Timer;
struct	Particle;
class	Shader;
struct	Vertex_PCUTBN;
class   VertexBuffer;
class   IndexBuffer;

struct Emitter
{
	Emitter(SpriteAnimDefinition const& particleAnim, Vec3 pos, Vec3 direction, float spreadAngleDegrees = 0.f, int numParticles = 1, float emitterLifeTime = 1.f);
	Emitter(std::string text, Vec3 pos, Vec3 direction, int numParticles = 1, float emitterLifeTime = 1.f);
	~Emitter();

	void Update();

	std::string m_text = "undefined";

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float	GetSizeScale(Particle* p) const;
	float	GetMovingSpeedScale(Particle* p) const;
	float	GetRotationSpeedScale(Particle* p) const;
	float	GetAlpha(Particle* p) const;
	Rgba8	GetColor(Particle* p) const;

	Emitter* SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange range);
	Emitter* SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange range);
	Emitter* SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange range);
	Emitter* SetParticle_Alpha_FromStartToEnd(unsortedFloatRange range);
	Emitter* SetParticle_Color_FromStartToEnd(Rgba8 start, Rgba8 end);

	// those range shows the start and end
	unsortedFloatRange m_sizeScale_startEnd;				// percent of its original value
	unsortedFloatRange m_movingSpeedScale_startEnd;			// percent of its original value
	unsortedFloatRange m_rotationSpeedScale_startEnd;		// percent of its original value
	unsortedFloatRange m_alpha_startEnd = unsortedFloatRange(1.f, 0.f);

	Rgba8		m_colorRange_start = Rgba8::WHITE;
	Rgba8		m_colorRange_end = Rgba8::WHITE;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Emitter* SetParticle_Size_StartRange(FloatRange range);
	Emitter* SetParticle_MovingSpeed_StartRange(FloatRange range);
	Emitter* SetParticle_Rotation_StartRange(FloatRange range);
	Emitter* SetParticle_RotationSpeed_StartRange(FloatRange range);
	Emitter* SetParticle_LifeTime_Range(FloatRange range);
	Emitter* SetParticle_Offset_StartRange(FloatRange range);

	// those are the range to roll when a particle is generated
	FloatRange	m_movingSpeed_RandomRange;
	FloatRange	m_rotationSpeed_RandomRange;
	FloatRange	m_size_RandomRange;
	FloatRange	m_lifeTimeRange;
	FloatRange	m_rotationRange;	// Degrees
	FloatRange	m_offsetRange;

	// emitter transform information
	Vec3	GetRandomDirectionInSpreadCone() const;
	float	m_spread = 0.f;			// the spread angle for the start direction
	Vec3	m_direction = Vec3();	// particle generated moving direction

	Vec3	m_position = Vec3();
	int		m_particleCount = 0;

	Timer*	m_lifeTimer;
	bool	HasLifeEnded() const;

	Shader*	m_shader = nullptr;
	SpriteAnimDefinition const*	m_particleAnim = nullptr;
};

struct Particle
{
	Particle(Emitter& emitter);
	~Particle();

	explicit Particle(Particle const& other);		// if we have a move constructor, we must have a explicit copy constructor
	// Particle(Particle const& other) = default; 

	// Move constructor for remove_if
	Particle(Particle&& other) noexcept;
	// Move assignment operator
	Particle& operator=(Particle&& other) noexcept;	// we need to manually trigger the copy constructor of the Timer

	void	Startup();

	void	Update();
	void	Render() const;

	BillboardType	m_billboardType = BillboardType::FULL_CAMERA_OPPOSING;

	Timer*			m_lifeTimer = nullptr;
	bool			HasLifeEnded() const;

	Vec3			m_position = Vec3();
	Vec3			m_direction = Vec3();
	float			m_rotation = 0.f;

	float			m_originalMovingSpeed = 0.f;
	float			m_rotationSpeed = 0.f;
	
	Rgba8			m_color = Rgba8::WHITE;
	Mat44			m_modelMatrix = Mat44();
	Emitter*		m_emitter = nullptr;
	
	float			m_originalSize = 0.f;

	std::vector<Vertex_PCU>		m_particleVertexes;
	// std::vector<Vertex_PCUTBN>		m_particleVertexes;
	// std::vector<unsigned int>		m_particleIndexArray;
	// 
	// VertexBuffer* m_particleVertexBuffer = nullptr;
	// IndexBuffer* m_particleIndexBuffer = nullptr;
};

class EffectSystem		// manage
{
public:

	EffectSystem() = default;
	~EffectSystem() = default;

	void			Startup();

	void			Shutdown();

	void			Update();
	void			UpdateEmitters();
	void			UpdateParticles();
	void			SortParticlesRenderingOrder();

	void			Render() const;

	EffectSystem*			AddEmitter(Emitter *& emitter);

	int						GetPartciclesCountForEmitter(Emitter* emitter) const;
	 
	std::vector<Emitter*>	m_emitters;
	std::vector<Particle>	m_particles;	// the effect system will sort the particles for rendering

	void					UpdateTrackingCamera(Camera& camera);
	Camera*					m_trackingCamera = nullptr;

	static bool				Command_GenerateDamageEmitter(EventArgs& args);
	void					GenerateDamageNumberEmitter(Vec3 const& position, int amount, float duration);
};