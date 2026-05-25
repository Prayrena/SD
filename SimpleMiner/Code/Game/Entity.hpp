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
	Entity(Game* owner, Vec3 const& startPos);
	virtual ~Entity();

	virtual void Startup();
	virtual void Update() = 0;// 0 means it's pure virtual func that children's func must be rewritten
	virtual void TakeDamage();

	virtual void Render() const = 0;
	virtual void DebugRender() const;

	virtual bool IsOffScreen() const;
	virtual void InitializeLocalVerts();

	Vec2 GetForwardNormal() const;
	Mat44 GetModelMatrix() const;

	//Rgba8 updateDebrisColor(string entityCategory);

	Game*	m_game					= nullptr;// if we just delcare the m_game but not using its function, we just
											  //declare the class Game instead of include <Game.hpp>
	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Rgba8	m_color = Rgba8::WHITE;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	float	m_orientationDegrees	= 0.f;
	float	m_linearVelocity		= 0.f;
	float   m_angularVelocity_float		= 0.f;// spin rate, in degrees/second, + is in counter-clockwise

	float	m_physicsRadius			= 5.f;// radius for determine the collision
	float	m_cosmeticRadius		= 10.f;// radius for what it looks like

	float	m_uniformScale			= 1.5f;

	// debris settings
	Rgba8   m_debrisColor			= Rgba8(255, 0, 255, 255);
	float   debris_outer_radius		= 0.f;
	float   debris_inner_radius		= 0.f;

	int		m_health				= 1;
	bool	m_isDead				= false;
	bool	m_isGarbage				= false;// need to be delete or not


private:

};

