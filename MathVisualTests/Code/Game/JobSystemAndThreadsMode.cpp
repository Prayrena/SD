#include "Game/JobSystemAndThreadsMode.hpp"

#include "Engine/Input/InputSystem.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include <chrono>
#include <thread>

extern InputSystem* g_theInput;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;

JobSystemAndThreadsMode::JobSystemAndThreadsMode()
	: GameMode()
{
}

JobSystemAndThreadsMode::~JobSystemAndThreadsMode()
{
}

JobSystemAndThreadsMode::TestJob::TestJob(int sleepDurationMilliseconds)
	: Job()
	, m_sleepDurationMilliseconds(sleepDurationMilliseconds)
{
}

void JobSystemAndThreadsMode::TestJob::Execute()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDurationMilliseconds));
}

void JobSystemAndThreadsMode::Startup()
{
	g_theInput->SetCursorMode(false, false);
	m_worldCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(WORLD_SIZE_X, WORLD_SIZE_Y));
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_ORTHO_X, SCREEN_CAMERA_ORTHO_Y));

	JobSystemConfig config;
	m_jobSystem = new JobSystem(config);
	m_jobSystem->Startup();

	UpdateModeInfo();
}

void JobSystemAndThreadsMode::Update(float deltaSeconds)
{
	(void)deltaSeconds;

	if (g_theInput->WasKeyJustPressed('J'))
	{
		CreateAndQueueJob();
	}

	if (g_theInput->WasKeyJustPressed('N'))
	{
		CreateAndQueueJobs(100);
	}

	if (g_theInput->WasKeyJustPressed('R'))
	{
		RetrieveOneCompletedJob();
	}

	if (g_theInput->WasKeyJustPressed('A'))
	{
		RetrieveAllCompletedJobs();
	}

	UpdateModeInfo();
}

void JobSystemAndThreadsMode::Render() const
{
	if (!m_jobs.empty())
	{
		std::vector<Vertex_PCU> jobVerts;
		AddVertsForVisibleJobs(jobVerts);

		g_theRenderer->BeginCamera(m_worldCamera);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)jobVerts.size(), jobVerts.data());
		g_theRenderer->EndCamera(m_worldCamera);
	}

	g_theRenderer->BeginCamera(m_screenCamera);
	RenderScreenMessage();
	g_theRenderer->EndCamera(m_screenCamera);
}

void JobSystemAndThreadsMode::Shutdown()
{
	if (m_jobSystem)
	{
		m_jobSystem->ShutDown();
		delete m_jobSystem;
		m_jobSystem = nullptr;
	}

	for (TestJob* job : m_jobs)
	{
		delete job;
	}
	m_jobs.clear();
}

void JobSystemAndThreadsMode::UpdateModeInfo()
{
	m_modeName = "JobSystemAndThreads";
	m_controlInstruction = "J: queue 1 job | N: queue 100 jobs | R: retrieve 1 completed | A: retrieve all completed";
	m_testString = "Red: queued | Yellow: running | Green: completed | Blue: retrieved";
}

void JobSystemAndThreadsMode::CreateAndQueueJob()
{
	int sleepDurationMilliseconds = g_rng->RollRandomIntInRange(50, 3000);
	TestJob* job = new TestJob(sleepDurationMilliseconds);
	m_jobs.push_back(job);
	m_jobSystem->QueueJobs(job);
}

void JobSystemAndThreadsMode::CreateAndQueueJobs(int numJobs)
{
	for (int jobIndex = 0; jobIndex < numJobs; ++jobIndex)
	{
		CreateAndQueueJob();
	}
}

void JobSystemAndThreadsMode::RetrieveOneCompletedJob()
{
	m_jobSystem->RetrieveCompletedJobs(nullptr);
}

void JobSystemAndThreadsMode::RetrieveAllCompletedJobs()
{
	while (m_jobSystem->RetrieveCompletedJobs(nullptr) != nullptr)
	{
	}
}

void JobSystemAndThreadsMode::AddVertsForVisibleJobs(std::vector<Vertex_PCU>& verts) const
{
	int numJobs = (int)m_jobs.size();
	int visibleStartIndex = 0;
	while (numJobs - visibleStartIndex > MAX_VISIBLE_JOBS)
	{
		visibleStartIndex += NUM_JOB_COLUMNS;
	}

	float squareSize = 3.f;
	float spacingX = (WORLD_SIZE_X - squareSize * (float)NUM_JOB_COLUMNS) / (float)(NUM_JOB_COLUMNS + 1);
	float spacingY = (WORLD_SIZE_Y - squareSize * (float)NUM_JOB_ROWS) / (float)(NUM_JOB_ROWS + 1);

	for (int jobIndex = visibleStartIndex; jobIndex < numJobs; ++jobIndex)
	{
		int visibleJobIndex = jobIndex - visibleStartIndex;
		int column = visibleJobIndex % NUM_JOB_COLUMNS;
		int rowFromBottom = visibleJobIndex / NUM_JOB_COLUMNS;

		AABB2 bounds;
		bounds.m_mins.x = spacingX + (float)column * (squareSize + spacingX);
		bounds.m_mins.y = spacingY + (float)rowFromBottom * (squareSize + spacingY);
		bounds.m_maxs = bounds.m_mins + Vec2(squareSize, squareSize);

		JobStatus status = m_jobs[jobIndex]->m_jobStatus.load();
		AddVertsForAABB2D(verts, bounds, GetColorForJobStatus(status));
	}
}

Rgba8 JobSystemAndThreadsMode::GetColorForJobStatus(JobStatus status) const
{
	switch (status)
	{
	case JobStatus::CREATED:		return Rgba8::LIGHT_ORANGE;
	case JobStatus::QUEUED:		return Rgba8::RED;
	case JobStatus::CLAIMED:		return Rgba8::YELLOW;
	case JobStatus::COMPLETED:	return Rgba8::GREEN;
	case JobStatus::RETRIEVED:	return Rgba8::BLUE;
	default:					return Rgba8::WHITE;
	}
}
