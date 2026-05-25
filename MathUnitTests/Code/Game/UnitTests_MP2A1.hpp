//-----------------------------------------------------------------------------------------------
// UnitTests_MP2A1.hpp
//
#include "Game/GameCommon.hpp"
#include "Game/UnitTests_MP1A7.hpp" // Uses any or all #defines and dependencies from prior tests


//-----------------------------------------------------------------------------------------------
void RunTests_MP2A1();


//-----------------------------------------------------------------------------------------------
// YOU MAY COMMENT THESE OUT TEMPORARILY to disable certain test sets while you work.
// For every assignment submission, all test sets must be enabled.
//
#define ENABLE_TestSet_MP2A1_ByteDenormalization
#define ENABLE_TestSet_MP2A1_DotAndCross
#define ENABLE_TestSet_MP2A1_EulerAngles


//-----------------------------------------------------------------------------------------------
// YOU MAY CHANGE any of these #includes to match your engine filenames (e.g. Vector2D.hpp, etc.)
//
#if defined(ENABLE_TestSet_MP2A1_EulerAngles)
#include "Engine/Math/EulerAngles.hpp"	// #include for your EulerAngles (Yaw/Pitch/Roll) class
#endif


//-----------------------------------------------------------------------------------------------
// YOU MAY CHANGE the "Your Name" column of these #defines to match your own classes / functions
//
//		MathUnitTests name							Your name
//		~~~~~~~~~~~~~~~~~~~~~~						~~~~~~~~~~~~~~~~~~~~~~
#define EulerAnglesClass							EulerAngles

#define Function_NormalizeByte						NormalizeByte
#define Function_DenormalizeByte					DenormalizeByte

#define Function_DotProduct3D						DotProduct3D
#define Function_CrossProduct2D						CrossProduct2D
#define Function_CrossProduct3D						CrossProduct3D

#define EulerAngles_GetAsVectors_XFwd_YLeft_ZUp		GetAsVectors_IFwd_JLeft_KUp
#define EulerAngles_GetAsMatrix_XFwd_YLeft_ZUp		GetAsMatrix_XFwd_YLeft_ZUp

