//-----------------------------------------------------------------------------------------------
// UnitTests_MP2A2.hpp
//
#include "Game/GameCommon.hpp"
#include "Game/UnitTests_MP2A1.hpp" // Uses any or all #defines and dependencies from prior tests

//-----------------------------------------------------------------------------------------------
void RunTests_MP2A2();


//-----------------------------------------------------------------------------------------------
// YOU MAY COMMENT THESE OUT TEMPORARILY to disable certain test sets while you work.
// For every assignment submission, all test sets must be enabled.
//
#define ENABLE_TestSet_MP2A2_AABB3_Basics
#define ENABLE_TestSet_MP2A2_DotProduct4D
#define ENABLE_TestSet_MP2A2_OrthoProjection
#define ENABLE_TestSet_MP2A2_PerspectiveProjection
#define ENABLE_TestSet_MP2A2_TransposeInvert
#define ENABLE_TestSet_MP2A2_OrthoNormalize


//-----------------------------------------------------------------------------------------------
// YOU MAY CHANGE any of these #includes to match your engine filenames (e.g. Vector2D.hpp, etc.)
//
#if defined(ENABLE_TestSet_MP2A2_AABB3_Basics)
#include "Engine/Math/AABB3.hpp"
#endif


//-----------------------------------------------------------------------------------------------
// YOU MAY CHANGE the "Your Name" column of these #defines to match your own classes / functions
//
//		MathUnitTests name							Your name
//		~~~~~~~~~~~~~~~~~~~~~~						~~~~~~~~~~~~~~~~~~~~~~
#define AABB3Class									AABB3
#define AABB3_Mins									m_mins		// e.g. "mins" if your AABB3 is used as "myBox.mins"
#define AABB3_Maxs									m_maxs		// e.g. "maxs" if your AABB3 is used as "myBox.maxs"

#define Function_DotProduct4D						DotProduct4D

#define Mat44_CreateOrthoProjection					CreateOrthoProjection
#define Mat44_CreatePerspectiveProjection			CreatePerspectiveProjection
#define Mat44_Transpose								Transpose
#define Mat44_GetOrthonormalInverse					GetOrthonormalInverse
#define Mat44_Orthonormalize_IFwd_JLeft_KUp			Orthonormalize_IFwd_JLeft_KUp


