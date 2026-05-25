#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Game/GameCommon.hpp"

class GetNearestPointsMode;
class App;

// if either children need to call g_theRenderer's function, the children's cpp will call it
// because if one hpp includes the other hpp file, it might cause circular inclusion problem if the other hpp also includes yourself hpp
// extern Renderer* g_theRenderer; // we are using the g_renderer to do all the rendering
extern App* g_theApp;


class UI
{
public:
	// owner define the ownership so the children could know which parent they should talk to
	// as well as they could talk to their parents 
	// however, g_theGame could directly control all the Entities's children
	UI(Vec2 const& startPos);
	virtual ~UI();

	virtual void Update( float deltaSeconds) = 0;// 0 means it's pure virtual func that children's func must be rewritten

	virtual void Render() const = 0;

	virtual void InitializeLocalVerts() = 0;

	Vec2 GetForwardNormal() const;
	//Rgba8 updateDebrisColor(string entityCategory);
											
	Vec2	m_position;
	Vec2	m_velocity;
	Vec2	m_originalPosition;

	float	m_orientationDegrees	= 0.f;
	float	m_linearVelocity		= 0.f;
	float   m_angularVelocity		= 0.f;// spin rate, in degrees/second, + is in counter-clockwise
	float	m_floatingCycle			= 0.f;

	float	m_uniformScale			= 2.f;
	float   m_shinningFrequency     = 1.f;
	float   m_age					= 0.f;
	Rgba8   m_UIColor;

	bool	m_isEnabled				= true;
	bool	m_isGarbage				= false;// need to be delete or not

private:

};

