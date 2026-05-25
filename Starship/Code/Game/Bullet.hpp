#pragma once
#include "Game/Entity.hpp"
#include "Engine/Audio/AudioSystem.hpp"

// initialize the variable before the compile of the cpp file
constexpr int		BULLET_NUM_TRI = 2; // the triangles that this entity uses
constexpr int		BULLET_NUM_VERTS = BULLET_NUM_TRI * 3; // the vertexs that this entity contains


class Bullet : public Entity
{
// if the children class do not have its own or change anything, it does not need to specify here
public:
	Bullet(Game* owner, Vec2 const& startPos);
	virtual ~Bullet();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override
	virtual void TakeDamage();
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void InitializeLocalVerts() override;

	// sound settings
	SoundPlaybackID m_bulletSound;

	// debris settings
	Rgba8   m_debrisColor = Rgba8(BULLET_DEBRIS_COLOR_R, BULLET_DEBRIS_COLOR_G, BULLET_DEBRIS_COLOR_B, DEBRIS_COLOR_A);// each 
	int		debris_min_num = BULLET_DEBRIS_MIN_NUM;
	int		debris_max_num = BULLET_DEBRIS_MAX_NUM;

private:
	void RenderBullet() const;

	Vertex_PCU m_localVerts[BULLET_NUM_VERTS];//C++ could not generate dynamic array, size must be known at compile time
};

