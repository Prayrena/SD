#include "Engine/Animation/Actor.hpp"

class HealthBar
{
public:
	HealthBar(Actor* trackingActor, Vec2 const& size, Vec2 const& frameOffset = Vec2(0.05f, 0.05f), Vec3 dispToActor = Vec3::ZERO,
				Rgba8 backgroundColor = Rgba8::WHITE, Rgba8 healthColor = Rgba8::WHITE, 
				Texture* texture = nullptr, BillboardType m_billboardType = BillboardType::FULL_CAMERA_FACING);
	~HealthBar();

	void Update(Mat44 const& trackingMat);
	void Render() const;

	Actor*			m_trackingActor = nullptr;
	Vec3			m_dispToActor = Vec3::ZERO;

	Vec2			m_size = Vec2::ZERO;
	Vec2			m_frameOffset = Vec2::ZERO;

	Mat44			m_modelMatrix = Mat44();
	BillboardType	m_billboardType = BillboardType::FULL_CAMERA_FACING;

	Rgba8			m_BGColor = Rgba8::WHITE;
	Rgba8			m_healthColor = Rgba8::RED;

	Texture*		m_BGTexture = nullptr;
	std::vector<Vertex_PCU>		m_healthBarVertexes;
};