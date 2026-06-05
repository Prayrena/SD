#pragma once

#include "Engine/core/JobSystem.hpp"
#include "Game/GameMode.hpp"
#include <vector>

class JobSystemAndThreadsMode : public GameMode
{
public:
	JobSystemAndThreadsMode();
	~JobSystemAndThreadsMode();

	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;
	void Shutdown() override;
	void UpdateModeInfo() override;

private:
	class TestJob : public Job
	{
	public:
		explicit TestJob(int sleepDurationMilliseconds);
		void Execute() override;

	private:
		int m_sleepDurationMilliseconds = 0;
	};

	void CreateAndQueueJob();
	void CreateAndQueueJobs(int numJobs);
	void RetrieveOneCompletedJob();
	void RetrieveAllCompletedJobs();
	void AddVertsForVisibleJobs(std::vector<Vertex_PCU>& verts) const;
	Rgba8 GetColorForJobStatus(JobStatus status) const;

private:
	static constexpr int NUM_JOB_COLUMNS = 40;
	static constexpr int NUM_JOB_ROWS = 20;
	static constexpr int MAX_VISIBLE_JOBS = NUM_JOB_COLUMNS * NUM_JOB_ROWS;

	JobSystem* m_jobSystem = nullptr;
	std::vector<TestJob*> m_jobs;
};
