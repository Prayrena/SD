#pragma once
#include "Game/Entity.hpp"// to inheritance, need to include the parent .hpp

// initialize the variable before the compile of the cpp file
constexpr int	DEBRIS_NUM_SIDE = 6; // how many sides does asteroid have
constexpr int	DEBRIS_NUM_TRI = DEBRIS_NUM_SIDE; // the triangles that this entity uses
constexpr int	DEBRIS_NUM_VERT = DEBRIS_NUM_TRI * 3; // the vertexes that this entity contains

//class Renderer;
//extern Renderer* g_theRenderer;

class Debris : public Entity
{
public:
	Debris(Vec2 const& startPos);
	virtual ~Debris();

	virtual void Update( float deltaSeconds ) override;	// 'override' states that the virtual function is being override

	virtual void InitializeLocalVerts() override;

	virtual void Render() const override;
	virtual void DebugRender() const override;

	void UpdateDebrisColorAlpha();

	float m_age = DEBRIS_LIFESPAN;

private:

	bool IsOffScreen() const;
	bool DebrisDeadDueToAge();
	Vertex_PCU	m_localVerts[DEBRIS_NUM_VERT];

};

