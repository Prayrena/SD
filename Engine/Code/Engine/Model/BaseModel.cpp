#include "Engine/Model/BaseModel.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern RandomNumberGenerator* g_rng;

BaseModel::BaseModel(Vec3 const& startPos /*= Vec3()*/)
	: m_position(startPos)
{

}

BaseModel::~BaseModel()
{
}

void BaseModel::Startup()
{
	m_position = Vec3(-2.f, 0.f, 0.f);
}

void BaseModel::DebugRender() const
{

}

bool BaseModel::IsOffScreen() const
{
	return false;
}

void BaseModel::InitializeLocalVerts()
{

}

Vec2 BaseModel::GetForwardNormal() const
{
	return Vec2(0.f, 0.f);
}

// transform coordinate from local to world matrix
// [translate][rotate]
Mat44 BaseModel::GetModelMatrix() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat = m_orientation.GetAsMatrix_XFwd_YLeft_ZUp();
	// orientationMat.Append(transformMat);
	// return orientationMat;
	transformMat.Append(orientationMat);
	return transformMat;
}

Mat44 BaseModel::GetModelMatrixWithQuat() const
{	
	Mat44 transformMat;
	transformMat.SetTranslation3D(m_position);
	Mat44 orientationMat(m_quatOrientation);
	transformMat.Append(orientationMat);
	return transformMat;
}

