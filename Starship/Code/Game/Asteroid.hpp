#pragma once
#include "Game/Entity.hpp"// to inheritance, need to include the parent .hpp



// initialize the variable before the compile of the cpp file
constexpr int		ASTEROID_NUM_SIDE = 16; // how many sides does asteroid have
constexpr int		ASTEROID_NUM_TRI = ASTEROID_NUM_SIDE; // the triangles that this entity uses
constexpr int		ASTEROID_NUM_VERT = ASTEROID_NUM_TRI * 3; // the vertexes that this entity contains

//class Renderer;
//extern Renderer* g_theRenderer;

class Asteroid : public Entity
{
public:
	Asteroid(Game* owner, Vec2 const& startPos);
	virtual ~Asteroid();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override
	virtual void TakeDamage();
	virtual void Render() const override;
	virtual void DebugRender() const override;

	virtual bool IsOffScreen() const override;
	virtual void InitializeLocalVerts() override;

private:
	void BounceOffWalls();
	void Respawn();
	void TransformAsteroidAcrossScreen();


	Vertex_PCU	m_localVerts[ASTEROID_NUM_VERT];

};

