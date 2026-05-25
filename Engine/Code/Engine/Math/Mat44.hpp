#pragma once
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Quat.hpp"

#include "ThirdParty/OpenXR/src/common/xr_linear.h"
#include <DirectXMath.h>

namespace physx 
{
	template <typename T> class PxTransformT; // Forward declaration of template class
	using PxTransform = PxTransformT<float>;   // Forward declare PxTransform as a typedef
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
// A 4x4 homogeneous 2D/3D transformation matrix, stored basis-major
// 
// we don't provide operate* overload, but prefer to use method names, which is "append"
// which is more neutral(e.g. multiply a new matrix "on the right in column-notation(on the left in row-notation)
 struct Mat44
 {
	enum
	{
		Ix, Iy, Iz, Iw,
		Jx, Jy, Jz, Jw,
		Kx, Ky, Kz, Kw,
		Tx, Ty, Tz, Tw
	};// index nicknames, [0] thru [15]
	float m_values[16]; //stored in "basis major" order (Ix, Iy, Iz, Iw, Jx ...) translation in [12, 13, 14]

    Mat44(); // Default constructor is identity matrix, not all zeros
	~Mat44() = default;
	explicit Mat44(Vec3 vec3);
	explicit Mat44(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D);
	explicit Mat44(Vec3 const& ibasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D = Vec3());
	explicit Mat44(Vec3 const& ibasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D, float UniformScale);
	explicit Mat44(Vec4 const& ibasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D);
	explicit Mat44(float const* sixteenValuesBasisMajor);
	explicit Mat44(DirectX::XMMATRIX const& directXMat);
	explicit Mat44(physx::PxTransform const& physXTransform);

	explicit Mat44(XrMatrix4x4f XrMat);
	explicit Mat44(XrPosef XrPose);
	explicit Mat44(XrVector3f XrVec3);
	explicit Mat44(XrQuaternionf quat);
	explicit Mat44(Quat quat);
	XrMatrix4x4f GetXrMatByMat();

	EulerAngles GetEulerAngles() const;
	Quat		GetQuaternion();	// two results are satisfied, they are Conjugated
	static Quat TransformQuatBetweenSpaces(Quat q, Mat44 transformMat);
	void		AppendRotation(Quat const& q);

	static Mat44 const CreateTranslation2D(Vec2 const& translationXY);
	static Mat44 const CreateTranslation3D(Vec3 const& translationXYZ);
	static Mat44 const CreateUniformScale2D(float uniformScaleXY );
	static Mat44 const CreateUniformScale3D(float uniformScaleXYZ);
	static Mat44 const CreateNonUniformScale2D(Vec2 nonUniformScaleXY );
	static Mat44 const CreateNonUniformScale3D(Vec3 nonUniformScaleXYZ);
	static Mat44 const CreateZRotationDegrees(float rotationDegreesAboutZ);
	static Mat44 const CreateYRotationDegrees(float rotationDegreesAboutY);
	static Mat44 const CreateXRotationDegrees(float rotationDegreesAboutX);
	static Mat44 const CreateRotationMatrixFromQuat(Quat const& q);

	Vec2 const TransformVectorQuantity2D(Vec2 const& vectorQuantityXY) const;   // assumes z = 0, w = 0
	Vec3 const TransformVectorQuantity3D(Vec3 const& vectorQuantityXYZ)const;   // assumes w = 0
	Vec2 const TransformPosition2D(Vec2 const& positionXY) const;			    // assumes z = 0, w = 1
	Vec3 const TransformPosition3D(Vec3 const& position3D) const;				// assumes w = 1
	Vec4 const TransformHomogeneous3D(Vec4 const& homogeneousPoint3D) const;	// w is provided

	Quat  TransformQuaternion(Quat const& q);
	Mat44 SimilarityTransformation(Mat44 const& rotationMat);

	float*			GetAsFloatArray();		// non-const (mutable) version
	float const*	GetAsFloatArray() const;// const version, used only when Mat44 is const
	Vec2  const		GetIBasis2D() const;
	Vec2  const		GetJBasis2D() const;
	Vec2  const		GetTranslation2D() const;
	Vec3  const		GetIBasis3D() const;
	Vec3  const		GetJBasis3D() const;
	Vec3  const		GetKBasis3D() const;
	Vec3  const		GetTranslation3D() const;
	Vec4  const		GetIBasis4D() const;
	Vec4  const		GetJBasis4D() const;
	Vec4  const		GetKBasis4D() const;
	Vec4  const		GetTranslation4D() const;
	Mat44 const 	GetOrthonormalInverse();// return a inverse matrix of a matrix, which is a rotation orthonormal matrix append translation orthonormal matrix

	void	SetTranslation2D(Vec2 const& translationXY);			// sets translationZ = 0, translationW = 1
	void	SetTranslation3D(Vec3 const& translationXYZ);			// sets translationW = 1
	void	SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D);	// sets z = 0; w = 0 for i & j; does not modify k or t
	void	SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY);	// sets z = 0, w = 0 for i, j, t; does not modify k
	void	SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D);			// sets w = 0 for i, j, k
	void	SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ); //sets w = 0 for i, j, k, w = 1 for t
	void	SetIJKT4D(Vec4 const& iBasis3D, Vec4 const& jBasis3D, Vec4 const& kBasis3D, Vec4 const& translation4D);	
	void	Transpose();// swap columns with rows, for rotation matrices, the transpose is equivalent to the inverse
	// when a matrix is not orthogonal or normalized
	// this function will turn the matrix to be orthogonal and normalized by fixing the IJK basis
	// Forward is canonical, up is secondary and left is tertiary
	void	Orthonormalize_IFwd_JLeft_KUp();

	// we are post multiplication
	Mat44	MatMultiply(Mat44 const& addingTransformInfo);	
	
	void	Append(Mat44 const& appendThis);						 // multiply on right in column notation / on left in row notation, 
	void	AppendZRotation(float degreesRotationAboutZ);			 // same as appending (* = in column notation) a z-rotation matrix
	void	AppendYRotation(float degreesRotationAboutY);			 // same as appending (* = in column notation) a y-rotation matrix
	void	AppendXRotation(float degreesRotationAboutX);			 // same as appending (* = in column notation) a x-rotation matrix
	void	AppendTranslation2D(Vec2 const& translationXY);			 // same as appending (* = in column notation) a translation matrix
	void	AppendTranslation3D(Vec3 const& translationXYZ);		 // same as appending (* = in column notation) a translation matrix
	void	AppendScaleUniform2D(float uniformScaleXY);				 // K and T bases should remain unaffected
	void	AppendScaleUniform3D(float uniformScaleXYZ);			 // translation should remain unaffected
	void	AppendScaleNonUniform2D(Vec2 const& nonUniformScaleXY);  // K and T bases should remain unaffected
	void	AppendScaleNonUniform3D(Vec3 const& nonUniformScaleXYZ); // translation should remain unaffected

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// input the render space info and get the projection transform mat44
	static Mat44 const CreateOrthoProjection(float left, float bottom, float Znear, float right, float top, float Zfar);
	static Mat44 const CreateOrthoProjection(Vec3 LBN, Vec3 RTF); // for inputs in render space coordinates 

	// fovYDegrees means the Y direction in dx11 coordinates
	// aspectRatio means the X / Y
	static Mat44 const CreatePerspectiveProjection(float fovYDegrees, float aspectRatio, float zNear, float zFar);

	// operator definition
	void operator = (Mat44 const& copyFrom); //Vec3 = Vec3
 };