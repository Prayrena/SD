#pragma once

#include "Engine/Animation/Skeleton.hpp"
#include "Engine/Math/Mat44.hpp"

struct TwoJointsIKSolver
{
	TwoJointsIKSolver(Joint* root, Joint* mid, Joint* end);
	TwoJointsIKSolver(TwoJointsIKSolver* copiedSolver);
	~TwoJointsIKSolver() = default;

	// take three joints to calculate
	Joint* m_root; // shoulder
	Joint* m_mid;  // elbow, use x direction and bone length to predict the position of it
	Joint* m_end;  // hand

	void SolveIK(); // updates transforms in-place
	void SolveIK_oldVersion(); // updates transforms in-place
	void SolveIKWithWeight(float weight); // the weight is relative to new pose comparing to origin

	std::vector<Joint*> m_joints;
	void SolveFK();

	void SetSolverTarget(Vec3 const& pos);
	void SetPoleVector(Vec3 const& pos);

	bool m_clampedlastFrame = false;

	Vec3 m_target = Vec3();
	Vec3 m_targetLastFrame = Vec3();

	bool m_usePoleVector = false;

	Vec3 m_poleVecPos = Vec3();
	Vec3 m_poleVectorLastFrame = Vec3();

	std::vector<Vertex_PCU> m_debugVerts;
	bool m_debugMode = false;
	unsigned int m_debugCounter = 0;

	bool m_step1_reached = false;
	bool m_step2_reached = false;
	bool m_step3_reached = false;

	bool	CheckIfInDebugModeAndShouldQuit();
	void	UpdateDebugCounterInDebugMode();
};