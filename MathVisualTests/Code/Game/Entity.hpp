#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
//#include "Engine/Renderer/Renderer.hpp"// this allows all children of Entity to use the function in Renderer
#include "Engine/core/VertexUtils.hpp"
//#include "Engine/core/EngineCommon.hpp"
#include "Game/GameCommon.hpp"
//#include "Engine/Math/RandomNumberGenerator.hpp"
//#include "Game/App.hpp"// circular loop
// Because the Enitity uses the Game class function, as well as a pointer pointing to it
// this hpp file must declare the Game class in the beginning

class GetNearestPointsMode;// allow all entity uses Game class's function
//class Renderer; // allow all entity to uses the renderer to do rendering stuff
class App;



// if either children need to call g_theRenderer's function, the children's cpp will call it
// because if one hpp includes the other hpp file, it might cause circular inclusion problem if the other hpp also includes yourself hpp
// extern Renderer* g_theRenderer; // we are using the g_renderer to do all the rendering
extern App* g_theApp;


class Entity
{
public:
	// owner define the ownership so the children could know which parent they should talk to
	// as well as they could talk to their parents 
	// however, g_theGame could directly control all the Entities's children
	Entity( Vec2 const& startPos );
	virtual ~Entity();

	virtual void Update( float deltaSeconds) = 0;// 0 means it's pure virtual func that children's func must be rewritten
	virtual void TakeDamage();

	virtual void Render() const = 0;
	virtual void DebugRender() const = 0;

	virtual bool IsOffScreen() const;
	virtual void InitializeLocalVerts() = 0;

	Vec2 GetForwardNormal() const;
	//Rgba8 updateDebrisColor(string entityCategory);

	Vec2	m_position;
	Vec2	m_velocity;

	//
	float	m_orientationDegrees	= 0.f;
	float	m_linearVelocity		= 0.f;
	float   m_angularVelocity		= 0.f;// spin rate, in degrees/second, + is in counter-clockwise

	float	m_physicsRadius			= 5.f;// radius for determine the collision
	float	m_cosmeticRadius		= 10.f;// radius for what it looks like

	float	m_uniformScale			= 1.5f;

	int		m_health				= 1;
	bool	m_isDead				= false;
	bool	m_isGarbage				= false;// need to be delete or not


private:

};

