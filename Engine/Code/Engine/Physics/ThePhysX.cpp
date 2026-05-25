#include "Engine/Physics/ThePhysX.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Math/PhysXMathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

extern Renderer* g_theRenderer;

using namespace physx;
using namespace std;

ThePhysX::ThePhysX(PhysXSystemConfig config)
		: m_PhysXConfig(config)
{
}

void ThePhysX::Startup()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
	  
	// set up pvd for debugging
	m_pvd = PxCreatePvd(*m_foundation);
	// Creates a transport mechanism for sending data from the PhysX SDK to the PVD tool over a network socket.
	// The transport uses a TCP / IP socket to communicate with PVD running on the local or remote machine
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	m_pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	 
	// Create Physics instance with PVD
	// PhysX uses PxTolerancesScale to :
	// Normalize values across different scales(e.g., small vs.large worlds).
	// Improve numerical stability for simulations.
	// Set defaults for parameters like skin width, contact offset, and other tolerances.
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), true, m_pvd);

	// create scene
	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.filterShader = [](PxFilterObjectAttributes attributes0, PxFilterData filterData0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) -> PxFilterFlags
		{
			(void)attributes0;
			// (void)filterData0;
			(void)attributes1;
			// (void)filterData1;
			(void)constantBlock;
			(void)constantBlockSize;
			  
			if (filterData0.word0 == filterData1.word0)	// if the two actors belong to the same group, suppress the collision contact
			{
				return PxFilterFlag::eSUPPRESS;
			}

			pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_CONTACT_POINTS; // | PxPairFlag::eNOTIFY_TOUCH_LOST; // | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
			return PxFilterFlag::eDEFAULT;
		};
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS; 
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM; 
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD; 

	sceneDesc.solverBatchSize = 128;  // Increase task parallelization

	// creates a CPU dispatcher that is responsible for managing and distributing tasks across multiple CPU threads for PhysX simulations
	int coresNum = std::thread::hardware_concurrency(); // get the numbers of threads that could run at the same time
	m_dispatcher = PxDefaultCpuDispatcherCreate(coresNum - 1);
	sceneDesc.cpuDispatcher = m_dispatcher;

	m_scene = m_physics->createScene(sceneDesc);
	 
	// retrieves the PVD scene client from the physics scene(gScene).
	// The PVD scene client allows communication between the application and the PhysX Visual Debugger for this specific scene
	// PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient();
	// if (pvdClient)
	// {
	// 	pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);	// joints or other mechanisms restricting movement between actors, such as hinges or sliders
	// 	pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);		// collisions between objects, such as contact points, normals, and penetration depths
	// 	pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);	// operations like raycasting, overlap tests, and sweep tests
	// }

	CreatePhysXPlaneAndGroundMat();

	SetVisualizationParameterByConfig();

	m_scene->setFlag(PxSceneFlag::eENABLE_ACTIVE_ACTORS, true);


#ifdef SHIPPING
	m_PhysXConfig.m_debugRender = false;
#endif
}

void ThePhysX::SetVisualizationParameterByConfig()
{
	m_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, m_PhysXConfig.m_globalScale); // Enable visualization globally

	if (m_PhysXConfig.m_showBodyAxes.first)
	{
		m_scene->setVisualizationParameter(PxVisualizationParameter::eWORLD_AXES, m_PhysXConfig.m_showBodyAxes.second);
		m_scene->setVisualizationParameter(PxVisualizationParameter::eBODY_AXES, m_PhysXConfig.m_showBodyAxes.second);
		m_scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, m_PhysXConfig.m_showBodyAxes.second);
	}

	if (m_PhysXConfig.m_showCollisionShapes.first)
	{
		m_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, m_PhysXConfig.m_showCollisionShapes.second);
	}
	// 
	// if (m_PhysXConfig->m_showActorAxes.first)
	// {
	// 	m_scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, m_PhysXConfig->m_showActorAxes.second);
	// }

	if (m_PhysXConfig.m_showJointLocalFrame.first)
	{
		m_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, m_PhysXConfig.m_showJointLocalFrame.second);
	}

	if (m_PhysXConfig.m_showJointLimits.first)
	{
		m_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, m_PhysXConfig.m_showJointLimits.second);
	}
	
	if (m_PhysXConfig.m_showContactPoint.first)
	{
		m_scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, m_PhysXConfig.m_showContactPoint.second);
	}
}

physx::PxMaterial* ThePhysX::GetPhysXMaterialByStinrg(std::string const& materialName) const
{
	auto it = m_materials.find(materialName);
	return it->second;
}

void ThePhysX::CreatePhysXPlaneAndGroundMat()
{
	Vec3 gameUpwards(Vec3(0.f, 0.f, 1.f));
	Vec3 physXUpwards = GetGameToPhysXSpaceMat().TransformVectorQuantity3D(gameUpwards);
	PxVec3 upwardsNormal = GetPxVec3(physXUpwards);

	// Define the plane equation: y = 0 (XZ plane)
	PxPlane groundPlane(upwardsNormal, 0.f);

	// Create the material for the plane
	PxMaterial* material = CreatePhysXMaterial("ground", 0.2f, 0.2f, 0.f);
	Actor* planeActor = new Actor(Vec3());

	// Create the plane actor
	PxRigidStatic* rigidPlane = PxCreatePlane(*m_physics, groundPlane, *material);

	// Add the plane to the scene
	m_scene->addActor(*rigidPlane);

	planeActor->m_rigidActor = rigidPlane;
}

void ThePhysX::ShutDown()
{
	// release all physX stuff
	PX_RELEASE(m_scene);
	PX_RELEASE(m_dispatcher);
	PX_RELEASE(m_physics);
	if (m_pvd)
	{
		PxPvdTransport* transport = m_pvd->getTransport();
		PX_RELEASE(m_pvd);
		PX_RELEASE(transport);
	}
	PX_RELEASE(m_foundation);
}

void ThePhysX::Update()
{
	// 1. Physics Simulation Consistency
	// The physics simulation always progresses in fixed intervals, maintaining stability and accuracy
	// 2. Handling Variable Frame Rates
	// The loop allows for multiple physics updates in a single frame if the deltaSeconds exceeds the timestep.
	// This prevents the physics from "falling behind" in case of lower FPS

	// if we get higher FPS, the physics-base movement might left behind or everything stay the same?
	// if we use another thread to simulate the physX, or do it this way, will it have a better result? or same result?
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	if (m_isUsingFixedPhysicsTimestep)
	{
		m_physicsTimeOwed += deltaSeconds;
		while (m_physicsTimeOwed >= m_physicsFixedTimestep)
		{
			m_physicsTimeOwed -= m_physicsFixedTimestep;
			m_scene->simulate(m_physicsFixedTimestep);
			m_scene->fetchResults(true);
		}
	}
	else
	{
		m_scene->simulate(deltaSeconds);
		m_scene->fetchResults(true);
	}
}

// this will draw physX SDK's debug information
void ThePhysX::Render() const
{
	if (!m_PhysXConfig.m_debugRender)
	{
		return;
	}

	PxRenderBuffer const& debugRenderBuffer = m_scene->getRenderBuffer();

	// draw lines
	// Iterate over debug lines
	const PxU32 numLines = debugRenderBuffer.getNbLines();
	const PxDebugLine* lines = debugRenderBuffer.getLines();

	// reserve
	// transform matrix

	vector<Vertex_PCU> tempLineVertexes;
	tempLineVertexes.reserve(21000);
	Mat44 transformMat = GetPhysXToGameSpaceMat();
	if (numLines != 0 && lines)
	{
		for (PxU32 i = 0; i < numLines; ++i) {
			const PxDebugLine& line = lines[i];

			// Extract start and end points
			PxVec3 start = line.pos0;
			PxVec3 end = line.pos1;

			// Extract color (ARGB format)
			PxU32 color = line.color0;

			Vec3 startPos = transformMat.TransformPosition3D(Vec3(start));
			Vec3 endPos = transformMat.TransformPosition3D(Vec3(end));

			Rgba8 verticeColor(color);

			Vertex_PCU v1(startPos, verticeColor, Vec2::ZERO);
			Vertex_PCU v2(endPos, verticeColor, Vec2::ZERO);

			tempLineVertexes.push_back(v1);
			tempLineVertexes.push_back(v2);
		}

		VertexBuffer* debugLineVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(tempLineVertexes.size()), sizeof(Vertex_PCU), true);

		size_t vertexSize = sizeof(Vertex_PCU);
		size_t vertexArrayDataSize = (tempLineVertexes.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(tempLineVertexes.data(), vertexArrayDataSize, debugLineVertexBuffer);

		// render
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(debugLineVertexBuffer, (int)tempLineVertexes.size());

		// after rendering, delete the vertexBuffer
		delete debugLineVertexBuffer;
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// draw triangles
	// Get debug triangles
	const PxU32 numTriangles = debugRenderBuffer.getNbTriangles();
	const PxDebugTriangle* triangles = debugRenderBuffer.getTriangles();

	vector<Vertex_PCU> tempTriangleVertexes;
	tempTriangleVertexes.reserve(1000);
	if (numTriangles != 0 && triangles)
	{
		// Iterate through triangles and render them
		for (PxU32 i = 0; i < numTriangles; ++i) 
		{
			const PxDebugTriangle& tri = triangles[i];

			// Extract vertices
			PxVec3 v0 = tri.pos0; // First vertex
			PxVec3 v1 = tri.pos1; // Second vertex
			PxVec3 v2 = tri.pos2; // Third vertex

			// Extract colors (ARGB format)
			PxU32 color = tri.color0; // Color of the triangle

			Vec3 v0Pos = transformMat.TransformPosition3D(Vec3(v0));
			Vec3 v1Pos = transformMat.TransformPosition3D(Vec3(v1));
			Vec3 v2Pos = transformMat.TransformPosition3D(Vec3(v2));

			Rgba8 verticeColor(color);

			Vertex_PCU t0(v0Pos, verticeColor, Vec2::ZERO);
			Vertex_PCU t1(v1Pos, verticeColor, Vec2::ZERO);
			Vertex_PCU t2(v2Pos, verticeColor, Vec2::ZERO);

			tempTriangleVertexes.push_back(t0);
			tempTriangleVertexes.push_back(t1);
			tempTriangleVertexes.push_back(t2);
		}

		VertexBuffer* debugTriangleVertexBuffer = g_theRenderer->CreateVertexBuffer((size_t)(tempTriangleVertexes.size()), sizeof(Vertex_PCU), false);

		size_t vertexSize = sizeof(Vertex_PCU);
		size_t vertexArrayDataSize = (tempTriangleVertexes.size()) * vertexSize;
		g_theRenderer->CopyCPUToGPU(tempTriangleVertexes.data(), vertexArrayDataSize, debugTriangleVertexBuffer);

		// render
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);

		g_theRenderer->SetModelConstants(Mat44());
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(debugTriangleVertexBuffer, (int)tempTriangleVertexes.size());

		// after rendering, delete the vertexBuffer
		delete debugTriangleVertexBuffer;
	}
}

// use physX to create material and add to the system
// for restitution: 0.0f (completely inelastic, no bounce) to 1.0f (perfectly elastic, full bounce)
PxMaterial* ThePhysX::CreatePhysXMaterial(std::string const& name, float staticFriction, float dynamicFriction, float restitution)
{
	PxMaterial* material = m_physics->createMaterial(staticFriction, dynamicFriction, restitution);
	m_materials[name] = material;
	return material;
}


physx::PxRigidDynamic* ThePhysX::ConvertActorFromDynamicToKinematic(physx::PxRigidDynamic* dynamicActor)
{
	if (dynamicActor)
	{
		dynamicActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		return dynamicActor;
	}
	else ERROR_AND_DIE("physX dynamic actor does not exist");
}

// there is no separate "kinematic" type in PhysX. 
// PhysX uses flags and methods to switch a dynamic actor between dynamic and kinematic behavior
physx::PxRigidDynamic* ThePhysX::ConvertActorFromKinematicToDynamic(physx::PxRigidDynamic* kinematicAcotr)
{
	if (kinematicAcotr)
	{
		kinematicAcotr->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
		return kinematicAcotr;
	}
	else ERROR_AND_DIE("physX dynamic actor does not exist");
}

void ThePhysX::MoveKinematicActor(physx::PxRigidDynamic* actor, Vec3 const& targetPos, Quat const& targetOrientation)
{
	// Ensure the actor is kinematic and valid
	if (actor && actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC) 
	{
		// convert the pass in game transform info to physX space transform info
		Vec3 targetPosInPhysX = GetGameToPhysXSpaceMat().TransformPosition3D(targetPos);
		Quat targetQuatInPhysX = GetGameToPhysXSpaceMat().TransformQuaternion(targetOrientation);
		PxTransform pxTransform;
		pxTransform.p = GetPxVec3(targetPos);
		pxTransform.q = GetPxQuat(targetOrientation);

		// Set the kinematic target
		actor->setKinematicTarget(pxTransform);
	}
	else
	{
		ERROR_AND_DIE("Actor is nullptr or it is dynamic actor");
	}
}

//physx::PxFilterFlags ThePhysX::CreateFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
//													PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
//{
//	pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND; // Enable contact notifications
//	return PxFilterFlag::eDEFAULT;
//}
