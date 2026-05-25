#pragma once
#include "Engine/Math/Quat.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Model/ObjModel.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include <string>
#include <vector>
#include <map>

class Skeleton; // tell the joint class that our skeleton class is coded later
struct TwoJointsIKSolver;
class Actor;

namespace physx
{
	class PxD6Joint;
	class PxRigidDynamic;
}

class Joint
{
public:
	Joint(std::string const& name, Vec3 bonePos = Vec3(), Quat localRotation = Quat(),
		Joint* parent = nullptr, Skeleton* skeleton = nullptr);
	Joint(Joint* j);
	~Joint();

	void Startup();
	void AddVertesForDebug();
	void CreateVertexBufferAndCopyToGPU();

	std::vector<Vertex_PCU> m_debugJointVerts;
	VertexBuffer* m_debugJointVertexBuffer = nullptr; 
	
	std::vector<Vertex_PCU> m_debugConstraintVerts;
	VertexBuffer* m_debugConstraintVertexBuffer = nullptr;
	
	std::vector<Vertex_PCU> m_debugBoneVerts;
	VertexBuffer* m_debugBoneVertexBuffer = nullptr;


	std::string			m_name;         // Name of the joint
	Joint* m_parent = nullptr;			// Pointer to the parent joint (nullptr if root)
	Skeleton* m_skeleton = nullptr;     // Pointer to the skeleton this joint belongs to
	std::vector<Joint*> m_children;		// Child joints

	// bool				m_IKMode = true; // false represent in FK mode, true means IK mode

	// Transformation data
	float	GetBoneLength() const;
	Vec3	GetBoneLocalPosition();
	void	SetLocalBonePos(Vec3 p);

	Vec3	m_boneEndPos = Vec3();				// The start of the bone is always at Vec3(), this is the bone end, it also works as the translation for the joint in parent's local space
	Quat	m_localRotation = Quat();			// Local rotation relative to the parent

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	Mat44	GetWorldTransformMat() const; // calculate the latest (translation + rotation) information whenever you need and extract it
	Mat44	GetDebugConstraintModelMatrix() const;
	Vec3	GetWorldPosition() const;
	Quat	GetWorldQuat() const;
	void	SetJointLocalOrientationByWorldOrientation(Quat const& worldQuat);
	Quat	GetLocalQuat() const;
	Mat44	GetLocalTransformMat() const;

	Mat44	GetModelMatrixForObjModel() const;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// max local rotation
	bool		DoesJointHaveConstraint() const;
	Quat		ContrainJointLocalRotation();
	void		SetLocalRotationConstraint(EulerAngles const& min, EulerAngles const& max);
	EulerAngles	m_minLocalRotation;
	EulerAngles	m_maxLocalRotation;

	// debug settings
	void		ChangeDebugMode(bool newStatus);
	bool		m_debugMode = false;
	float		m_debugBoneRadius = 0.025f;
	Rgba8		m_debugBoneColor = Rgba8::BRIGHT_ORANGE;

	float		m_debugJointRadius = 0.05f;
	Rgba8		m_debugJointColor = Rgba8::WHITE;

	float		m_debugArrowRadius = 0.01f;
	float		m_debugArrowLength = 0.1f;

	physx::PxD6Joint* 			m_D6Joint = nullptr;
	physx::PxRigidDynamic*		m_rigidActor = nullptr;
	Quat m_relativeRotationToRigidActor = Quat();
	Vec3 m_relativeDispFromRigidActor  = Vec3();
	float m_capsuleHalfHeight = 0.f;
	float m_capsuleRadius = 0.f;

	Mat44 m_relativeModelMatrixToRigidActor;

	void						BindJointWithObjModel(ObjModel* model);
	std::vector<ObjModel*>		m_objModels;

	void RenderJointModel() const;
	void DebugRenderJoint() const;
};

class AnimationCurve
{
public:
	AnimationCurve(std::vector<float> keys, std::vector<Vec3> pos);
	~AnimationCurve() {}

	std::vector<float> m_keys; // record the key frame time
	std::vector<Vec3> m_pos; // record the key frame joint 3D positions
};

class Chain
{
public:
	Chain() = default;
	Chain(Chain* c);
	~Chain();
	explicit Chain(TwoJointsIKSolver* solver);

	bool m_isCopied = false;

	float GetTotalBoneLength();
	Vec3 GetDispFromRootToEnd();
	float GetDistanceFromRootToEnd();

	Joint* GetEndJoint();
	Joint* GetRootJoint();
	Joint* GetMidJoint();
	std::vector<Joint*> m_joints;
	TwoJointsIKSolver* m_solver = nullptr;

	void InterpolateWithCopiedChain(Chain* copiedChain, float weight);

	// work together to control the movement of the end effector
	AnimationCurve* m_endEffectorCurve = nullptr;
	Timer* m_timer = nullptr;
};

class Skeleton
{
public:

	Skeleton(std::string const& skeletonName, Actor* actor);
	~Skeleton(); // clean up all its allocated joints

	void Startup();

	// Recursive deletion of joints
	void DeleteThisJointAndItsChildren(Joint* joint);

	// Add a joint to the skeleton
	void   AddRootJoint(Joint* rootJoint);
	Joint* AddJoint(const std::string& name, Vec3 boneEndPos = Vec3(), Quat relativeQuat = Quat(),
		Joint* parent = nullptr);

	// Update the entire skeleton's joints
	void Update();

	void DebugAllJoints(bool newStatus);

	void DebugRenderAllJoints() const;

	void Render() const;

	void BreakAllD6Joints();

	// record the root motion's offset and rotation
	// we can use these info to enable the root motion for the actor
	// for my thesis, these values stay at the origin of the actor
	Vec3	m_rootMotionPos = Vec3();

	Joint* GetJointByName(std::string name);
	Joint* m_rootJoint = nullptr;					// Root joint of the skeleton
	std::map<std::string, Joint*> m_allJoints;		// record all the joints, cache misses?

	std::string m_name;

	Actor* m_actor = nullptr; // the actor that the skeleton is bind to

	bool m_drawSkeletonDebug = false;
	bool m_renderByRigidActor = false;
};