#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"

class Game;// allow all entity uses Game class's function

class BaseModel
{
public:
	BaseModel(Vec3 const& startPos = Vec3());
	virtual ~BaseModel();

	virtual void Startup();
	virtual void Update() = 0;// 0 means it's pure virtual func that children's func must be rewritten

	virtual void Render() const = 0;
	virtual void DebugRender() const;

	virtual bool IsOffScreen() const;
	virtual void InitializeLocalVerts();

	Vec2	GetForwardNormal() const;
	Mat44	GetModelMatrix() const;
	Mat44	GetModelMatrixWithQuat() const;

	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity; // Euler angles per second

	Quat		m_quatOrientation = Quat();
	Mat44		m_modelMatrix = Mat44();

	Rgba8		m_color = Rgba8::WHITE;

	bool		m_renderDebug = false;
};

