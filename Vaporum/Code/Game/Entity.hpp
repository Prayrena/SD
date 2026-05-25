#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"

class Game;// allow all entity uses Game class's function

class Entity
{
public:
	// owner define the ownership so the children could know which parent they should talk to
	// as well as they could talk to their parents 
	// however, g_theGame could directly control all the Entities's children
	Entity(Game* owner, Vec3 const& startPos = Vec3());
	virtual ~Entity();

	virtual void Startup();
	virtual void Update() = 0;// 0 means it's pure virtual func that children's func must be rewritten

	virtual void Render() const = 0;
	virtual void DebugRender() const;

	virtual bool IsOffScreen() const;
	virtual void InitializeLocalVerts();

	Vec2 GetForwardNormal() const;
	Mat44 GetModelMatrix() const;

	Game*		m_game					= nullptr;// if we just declare the m_game but not using its function, we just
											  // declare the class Game instead of include <Game.hpp>
	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Rgba8		m_color = Rgba8::WHITE;

	bool		m_renderDebug = false;
};

