#include "Engine/Animation/Skeleton.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Model/Material.hpp"
#include "Engine/Animation/Actor.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/Animation/ProceduralAnimation/IKTwoBonesSolver.hpp"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "ThirdParty/Physx/include/PxPhysicsAPI.h"

#ifdef VR_MODE
#include "Game/Game.hpp"
extern Game* g_theGame;
#endif

using namespace physx;

extern Renderer* g_theRenderer;
extern Material* g_materials;

Joint::Joint(std::string const& name, Vec3 bonePos /*= Vec3()*/, Quat localRotation /*= Quat()*/,
	Joint* parent /*= nullptr*/, Skeleton* skeleton /*= nullptr*/)
	: m_name(name)
	, m_boneEndPos(bonePos)
	, m_parent(parent)
{
	m_localRotation = localRotation.GetNormalized();

	if (parent)
	{
		parent->m_children.push_back(this);

		if (!skeleton) // if no skeleton is specified, take its parent info
		{
			m_skeleton = m_parent->m_skeleton;
		}
		else
		{
			m_skeleton = skeleton;
		}
	}
	else // this is the root joint
	{
		m_skeleton = skeleton;
	}
}

Joint::Joint(Joint* j)
	: m_localRotation(j->m_localRotation)
	, m_boneEndPos(j->m_boneEndPos)
	, m_parent(j->m_parent)
	, m_skeleton(j->m_skeleton)
{
	
}

Joint::~Joint()
{
	if (m_debugConstraintVertexBuffer)
	{
		delete m_debugConstraintVertexBuffer;
		m_debugConstraintVertexBuffer = nullptr;
	}	
	
	if (m_debugBoneVertexBuffer)
	{
		delete m_debugBoneVertexBuffer;
		m_debugBoneVertexBuffer = nullptr;
	}

	if (m_debugJointVertexBuffer)
	{
		delete m_debugJointVertexBuffer;
		m_debugJointVertexBuffer = nullptr;
	}
}

void Joint::Startup()
{
	AddVertesForDebug();
	CreateVertexBufferAndCopyToGPU();

	if (!m_children.empty())
	{
		for (Joint* joint : m_children)
		{
			joint->Startup();
		}
	}
}

void Joint::AddVertesForDebug()
{

	// we locate the joint at the origin of the local space
	if (m_boneEndPos != Vec3())
	{
		AddVertsForCone3D(m_debugBoneVerts, Vec3(), m_boneEndPos,
			m_debugBoneRadius, m_debugBoneColor, AABB2::ZERO_TO_ONE, 4);
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// add verts for the sphere - joint
	AddVertsForSphere3D(m_debugJointVerts, Vec3(), m_debugJointRadius, m_debugJointColor, AABB2::ZERO_TO_ONE, 8, 4);
	// add verts for the Basis - orientation
	AddVertsForArrow3D(m_debugJointVerts, Vec3(), Vec3(m_debugArrowLength, 0.f, 0.f), m_debugArrowRadius, Rgba8::RED, Rgba8::RED);
	AddVertsForArrow3D(m_debugJointVerts, Vec3(), Vec3(0.f, m_debugArrowLength, 0.f), m_debugArrowRadius, Rgba8::GREEN, Rgba8::GREEN);
	AddVertsForArrow3D(m_debugJointVerts, Vec3(), Vec3(0.f, 0.f, m_debugArrowLength), m_debugArrowRadius, Rgba8::BLUE, Rgba8::BLUE);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (DoesJointHaveConstraint())
	{
		AddDebugVertsForJointConstraint(m_debugConstraintVerts, Vec3(), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f),
			m_minLocalRotation, m_maxLocalRotation);
	}
}

void Joint::CreateVertexBufferAndCopyToGPU()
{
	size_t vertexSize = sizeof(Vertex_PCU); // Vertex_PCU_I
	size_t vertexArrayDataSize = (m_debugBoneVerts.size()) * vertexSize;
	if (!m_debugBoneVerts.empty())
	{
		m_debugBoneVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugBoneVerts.size()), vertexSize);
		g_theRenderer->CopyCPUToGPU(m_debugBoneVerts.data(), vertexArrayDataSize, m_debugBoneVertexBuffer);
	}
	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	vertexArrayDataSize = (m_debugJointVerts.size()) * vertexSize;
	m_debugJointVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugJointVerts.size()), vertexSize);
	g_theRenderer->CopyCPUToGPU(m_debugJointVerts.data(), vertexArrayDataSize, m_debugJointVertexBuffer);

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (DoesJointHaveConstraint())
	{
		vertexArrayDataSize = (m_debugConstraintVerts.size()) * vertexSize;
		m_debugConstraintVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(m_debugConstraintVerts.size()), vertexSize);
		g_theRenderer->CopyCPUToGPU(m_debugConstraintVerts.data(), vertexArrayDataSize, m_debugConstraintVertexBuffer);
	}
}

float Joint::GetBoneLength() const
{
	return m_boneEndPos.GetLength();
}

Vec3 Joint::GetBoneLocalPosition()
{
	return m_boneEndPos;
}

void Joint::SetLocalBonePos(Vec3 p)
{
	m_boneEndPos = p;
}

Mat44 Joint::GetWorldTransformMat() const
{
	if (!m_skeleton->m_renderByRigidActor)
	{
		// only the actor has world pos and rotation
		// joints, skeleton just have local/relative transform information
		// calculate world transform info
		if (m_parent)
		{
			Mat44 parentMat = m_parent->GetWorldTransformMat();
			Mat44 localMat = GetLocalTransformMat();

			// If there's a parent joint, calculate the world position and rotation relative to the parent
			return parentMat.MatMultiply(localMat);
		}
		else // root joint, get skeleton's world transform mat do similar stuff, this stops the recursive loop
		{
			Mat44 actorMat = m_skeleton->m_actor->GetModelMatrix();
			Mat44 localMat = GetLocalTransformMat();

			return actorMat.MatMultiply(localMat);
		}
	}
	else
	{
		if (!m_rigidActor)
		{
			if (m_parent)
			{
				Mat44 parentMat = m_parent->GetWorldTransformMat();
				Mat44 localMat = GetLocalTransformMat();

				// If there's a parent joint, calculate the world position and rotation relative to the parent
				return parentMat.MatMultiply(localMat);
			}
			else // root joint, get skeleton's world transform mat do similar stuff, this stops the recursive loop
			{
				Mat44 actorMat = m_skeleton->m_actor->GetModelMatrix();
				Mat44 localMat = GetLocalTransformMat();

				return actorMat.MatMultiply(localMat);
			}
		}


		// update position and orientation
		// Mat44 quatMat = GetPhysXToGameSpaceMat().SimilarityTransformation(Mat44(Quat(capsuleCollision)));
		// Vec3 actorZUp = quatMat.GetKBasis3D();
		// Vec3 pos = GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(capsulePos)) + (m_capsuleHalfHeight + m_capsuleRadius) * (actorZUp * 1.f);
		// 
		// Mat44 transformMat(pos);
		// Mat44 modelMat = transformMat.MatMultiply(Mat44(TransformQuatFromPhysXToGame(Quat(capsuleCollision))));
		
		// todo: this method is wrong, find out why
		// PxTransform capsuleGlobalPose = m_rigidActor->getGlobalPose();

		// Extract the position and orientation
		// PxVec3 capsulePos = capsuleGlobalPose.p;
		// Quat capsuleCollision = TransformQuatFromPhysXToGame(Quat(capsuleGlobalPose.q));
		// 
		// Vec3 rotatedOffset = capsuleCollision.RotateVector(m_relativeDispFromRigidActor);
		// 
		// Mat44 transformMat(GetPhysXToGameSpaceMat().TransformPosition3D(Vec3(capsulePos)) + rotatedOffset);
		// Quat JointOrientation = (capsuleCollision * m_relativeRotationToRigidActor).GetNormalized();
		// Mat44 orientationMat(JointOrientation);
		// Mat44 modelMat = transformMat.MatMultiply(orientationMat);

		Mat44 actorModelMatrix = GetPhysXToGameSpaceMat().SimilarityTransformation(Mat44(m_rigidActor->getGlobalPose()));
		Mat44 modelMat = actorModelMatrix.MatMultiply(m_relativeModelMatrixToRigidActor);

		return modelMat;
	}
}

Mat44 Joint::GetDebugConstraintModelMatrix() const
{
	if (m_parent)
	{
		Mat44 parentMat = m_parent->GetWorldTransformMat();
		Mat44 localMat(m_boneEndPos);

		// If there's a parent joint, calculate the world position and rotation relative to the parent
		return parentMat.MatMultiply(localMat);
	}
	else
	{
		return Mat44();
	}
}

Vec3 Joint::GetWorldPosition() const
{
	return GetWorldTransformMat().GetTranslation3D();
}

Quat Joint::GetWorldQuat() const
{
	// position information are not related, this is corrected
	if (m_parent)
	{
		return (m_parent->GetWorldQuat() * m_localRotation).GetNormalized(); 
	}
	else
	{
		// for the root joint's world quat
		// do not forget to add actor's world rotation
		Quat actorOrientation = ConvertEulerAnglesToQuat(m_skeleton->m_actor->m_orientation);
		return (actorOrientation * m_localRotation).GetNormalized();								
	}

	// return GetWorldTransformMat().GetQuaternion(); // may have two answers for negated quats, so we are not using this method
}

void Joint::SetJointLocalOrientationByWorldOrientation(Quat const& worldQuat)
{
	if (m_parent)
	{
		m_localRotation = m_parent->GetWorldQuat().GetInversed() * worldQuat;
	}
	else   // this is root joint
	{
		m_localRotation = ConvertEulerAnglesToQuat(m_skeleton->m_actor->m_orientation).GetInversed() * worldQuat;
	}
}

Quat Joint::GetLocalQuat() const
{
	return m_localRotation;
}

// if player moves, move the actor instead of skeleton or joints
// notice, the bone of this joint means the bone that the parent joint use to link this joint(the orange bone in unreal, not the green one)
Mat44 Joint::GetLocalTransformMat() const
{
	Mat44 transformationMat;
	if (m_parent)
	{
		Mat44 translationMat = Mat44::CreateTranslation3D(m_boneEndPos);			
		Mat44 rotationMat = Mat44::CreateRotationMatrixFromQuat(m_localRotation.GetNormalized());

		// rotate the joint first, then use the bone end pos to translate the joint
		transformationMat = translationMat.MatMultiply(rotationMat);
	}
	else // for the root
	{
		Mat44 translationMat = Mat44::CreateTranslation3D(m_skeleton->m_rootMotionPos);
		Mat44 rotationMat = Mat44::CreateRotationMatrixFromQuat(m_skeleton->m_rootJoint->m_localRotation.GetNormalized());
		transformationMat = translationMat.MatMultiply(rotationMat);
	}

	return transformationMat;
}

Mat44 Joint::GetModelMatrixForObjModel() const
{
	Mat44 transformMat;
	transformMat.SetTranslation3D(GetWorldPosition());
	Mat44 orientationMat(GetWorldQuat());
	transformMat.Append(orientationMat);
	return transformMat;
}

bool Joint::DoesJointHaveConstraint() const
{
	bool noConstraint = (m_minLocalRotation == EulerAngles() && m_maxLocalRotation == EulerAngles());
	return !noConstraint;
}

Quat Joint::ContrainJointLocalRotation()
{
	if (DoesJointHaveConstraint())
	{
		m_localRotation = m_localRotation.ClampQuaternion(m_minLocalRotation, m_maxLocalRotation);
		return m_localRotation;
	}
	else
	{
		return m_localRotation;
	}
}

void Joint::SetLocalRotationConstraint(EulerAngles const& min, EulerAngles const& max)
{
	m_minLocalRotation = min;
	m_maxLocalRotation = max;
}

void Joint::ChangeDebugMode(bool newStatus)
{
	m_debugMode = newStatus;
	if (!m_children.empty())
	{
		for (Joint* joint : m_children)
		{
			joint->ChangeDebugMode(newStatus);
		}
	}
}

void Joint::BindJointWithObjModel(ObjModel* model)
{
	m_objModels.push_back(model);
}
 
void Joint::RenderJointModel() const
{
	// render the model
	Mat44 modelMat = GetWorldTransformMat();

	g_theRenderer->SetModelConstants(modelMat);
	if (g_theRenderer->m_config.m_drawPlanarShadow)
	{
		g_theRenderer->m_modelMatForPlanarShadow = modelMat;
	}


	if (!m_objModels.empty())
	{
		for (auto model : m_objModels)
		{
			if (model)
			{
				g_theRenderer->SetModelConstants(modelMat);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);
				g_theRenderer->SetDepthMode(DepthMode::ENABLED);

				model->Render();
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	if (!m_children.empty())
	{
		for (Joint* child : m_children)
		{
			child->RenderJointModel();
		}
	}
}

void Joint::DebugRenderJoint() const
{
#ifndef SHIPPING
#endif
	if (m_skeleton->m_drawSkeletonDebug)
	{
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);

		// draw bone
		if (m_boneEndPos != Vec3() && m_parent)
		{
			g_theRenderer->SetModelConstants(m_parent->GetWorldTransformMat());
			g_theRenderer->DrawVertexBuffer(m_debugBoneVertexBuffer, (int)m_debugBoneVerts.size());
		}

		// draw joint
		g_theRenderer->SetModelConstants(GetWorldTransformMat());
		g_theRenderer->DrawVertexBuffer(m_debugJointVertexBuffer, (int)m_debugJointVerts.size());

		// draw constraint - not using for now
		// if (DoesJointHaveConstraint())
		// {
		// 	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
		// 	g_theRenderer->SetModelConstants(GetDebugConstraintModelMatrix());
		// 	g_theRenderer->DrawVertexBuffer(m_debugConstraintVertexBuffer, (int)m_debugConstraintVerts.size());
		// 	// g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		// 	// g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
		// 	// g_theRenderer->DrawVertexBuffer(m_debugConstraintVertexBuffer, (int)m_debugConstraintVerts.size());
		// }
	}

	if (!m_children.empty())
	{
		for (Joint* child : m_children)
		{
			child->DebugRenderJoint();
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
Skeleton::Skeleton(std::string const& skeletonName, Actor* actor)
	: m_name(skeletonName)
	, m_actor(actor)

{

}

Skeleton::~Skeleton()
{
	DeleteThisJointAndItsChildren(m_rootJoint);
}

void Skeleton::Startup()
{
	if (m_rootJoint)
	{
		m_rootJoint->Startup();
	}
}

void Skeleton::DeleteThisJointAndItsChildren(Joint* joint)
{
	if (joint)
	{
		if (!joint->m_children.empty())
		{
			for (Joint* child : joint->m_children)
			{
				DeleteThisJointAndItsChildren(child);
			}
		}
		delete joint;
	}
}

void Skeleton::AddRootJoint(Joint* rootJoint)
{
	m_rootJoint = rootJoint;
}

Joint* Skeleton::AddJoint(const std::string& name, Vec3 boneEndPos /*= Vec3*/, Quat relativeQuat /*= Quat()*/, Joint* parent /*= nullptr*/)
{
	// add the joint to the map for later search convenience
	Joint* newJoint = new Joint(name, boneEndPos, relativeQuat, parent, this);
	m_allJoints[name] = newJoint;

	// if the joint's parent is not specified, it will be connected to the root
	if (!parent)
	{
		if (!m_rootJoint) // if there is no root joint, the first added joint will be the root joint
		{
			m_rootJoint = newJoint;
		}
		else
		{
			m_rootJoint->m_children.push_back(newJoint);
		}
	}

	return newJoint;
}

void Skeleton::Update()
{
	// // if the joint have a model, let's update its model - world transform info
	// if (m_rootJoint)
	// {
	// 	m_rootJoint->UpdateObjModelTransformInfo();
	// }
}

void Skeleton::DebugAllJoints(bool newStatus)
{
	if (m_rootJoint)
	{
		m_rootJoint->ChangeDebugMode(newStatus);
	}
}

void Skeleton::DebugRenderAllJoints() const
{
	if (m_rootJoint)
	{
		m_rootJoint->DebugRenderJoint();
	}
}

void Skeleton::Render() const
{
	if (m_rootJoint)
	{
		m_rootJoint->RenderJointModel();
	}

	if (m_rootJoint)
	{
		m_rootJoint->DebugRenderJoint();
	}
}

void Skeleton::BreakAllD6Joints()
{
	for (const auto& [key, joint] : m_allJoints)
	{
		if (joint && joint->m_D6Joint)
		{
			joint->m_D6Joint->release();
		}
	}
}

Joint* Skeleton::GetJointByName(std::string name)
{
	auto it = m_allJoints.find(name);
	if (it != m_allJoints.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

Chain::Chain(TwoJointsIKSolver* solver)
	: m_solver(solver)
{
	m_joints.push_back(solver->m_root);
	m_joints.push_back(solver->m_mid);
	m_joints.push_back(solver->m_end);
}

Chain::Chain(Chain* c)
{
	m_solver = new TwoJointsIKSolver(c->m_solver);

	m_joints.push_back(m_solver->m_root);
	m_joints.push_back(m_solver->m_mid);
	m_joints.push_back(m_solver->m_end);

	m_isCopied = true;
}

Chain::~Chain()
{
	if (m_timer)
	{
		delete m_timer;
	}

	// for (auto timer : m_timerForAnimStates)
	// {
	// 	if (timer)
	// 	{
	// 		delete timer;
	// 	}
	// }
	// m_timerForAnimStates.clear();	// avoid dangling pointers

	// delete copied joints for copied chain
	if (m_isCopied)
	{
		for (auto joint: m_joints)
		{
			delete joint;
		}
		m_joints.clear();
	}
}

float Chain::GetTotalBoneLength()
{
	float totalLength = 0.f;
	int numJoints = (int)m_joints.size();
	for (int jointIndex = numJoints - 1; jointIndex > 0; --jointIndex)
	{
		float boneLen = GetDistance3D(Vec3::ZERO, m_joints[jointIndex]->m_boneEndPos);
		totalLength += boneLen;
	}
	return totalLength;
}

Vec3 Chain::GetDispFromRootToEnd()
{
	if (m_joints.empty())
	{
		ERROR_AND_DIE("The chain is empty, cannot get the distance from root to end effector");
	}
	Joint* endEffector = m_joints.back();
	Joint* root = *m_joints.begin();
	Vec3 disp = endEffector->GetWorldPosition() - root->GetWorldPosition();
	return disp;
}

float Chain::GetDistanceFromRootToEnd()
{
	Vec3 disp = GetDispFromRootToEnd();
	return disp.GetLength();
}

Joint* Chain::GetEndJoint()
{
	return m_joints.back();
}

Joint* Chain::GetRootJoint()
{
	return m_joints.front();
}

Joint* Chain::GetMidJoint()
{
	return m_joints[1];
}

void Chain::InterpolateWithCopiedChain(Chain* copiedChain, float weight)
{
	for (int i = 0; i < (int)copiedChain->m_joints.size(); ++i)
	{
		Quat::Nlerp(m_joints[i]->m_localRotation, copiedChain->m_joints[i]->m_localRotation, weight);
	}
}
