#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>

enum class JobStatus
{
	CREATED,
	QUEUED,
	CLAIMED,
	COMPLETED,
	RETRIEVED,

	NUM_STATE
};

enum class JobType
{
	GENERIC,
	DISK_IO,

	NUM_TYPES
};

enum class WorkerProfile
{
	GENERAL,
	DISK_IO,

	NUM_PROFILES
};

using JobTypeMask = unsigned int;

constexpr bool IsValidJobType(JobType jobType)
{
	return static_cast<unsigned int>(jobType) < static_cast<unsigned int>(JobType::NUM_TYPES);
}

constexpr JobTypeMask GetJobTypeBit(JobType jobType)
{
	return IsValidJobType(jobType) ? 1u << static_cast<unsigned int>(jobType) : 0u;
}

// Worker profiles are the single source of truth for which jobs each worker may claim.
// Keep this mapping explicit so adding a job category does not hide worker permissions in queue code.
constexpr JobTypeMask GetAllowedJobTypes(WorkerProfile workerProfile)
{
	switch (workerProfile)
	{
	case WorkerProfile::GENERAL:
		return GetJobTypeBit(JobType::GENERIC);
	case WorkerProfile::DISK_IO:
		return GetJobTypeBit(JobType::DISK_IO);
	default:
		return 0u;
	}
}

struct JobSystemConfig
{
	int numberOfWorkers = -1; // if it equals -1, create one fewer total worker than the CPU core count
	bool createDedicatedDiskIOWorker = true;
};

class Job
{
	friend class JobWorkerThread;

public:

	Job() {}
	virtual ~Job();

	virtual void Execute() = 0;
	virtual JobType GetJobType() const;
	// JobSystem owns generic status transitions; game code should only observe this.
	std::atomic <JobStatus> m_jobStatus = JobStatus::CREATED;
};

class JobSystem
{
public:
	JobSystem(JobSystemConfig const& config);
	~JobSystem();

	void Startup();
	void ShutDown();

	void CreateWorkers(int numGenericWorkers, int numDiskIOWorkers);
	void DestroyAllWorkers(); // todo: do I need to let the job system destroy all thread workers?

	void QueueJobs(Job* jobToQueue);
	Job* ClaimJob();
	Job* ClaimJob(JobWorkerThread* worker); // specific workers will claim specific jobs
	void ReceiveCompletedJob(Job* job);
	Job* RetrieveCompletedJobs(Job* requestedJob); // Retrieve any completed job, or a specific job

	// Legacy generic-job count used by current game code to throttle CPU work.
	int GetNumQueuedJobs() const;
	int GetNumQueuedJobs(JobType jobType) const;
	// Exposes completed count without requiring game code to read the protected list directly.
	int GetNumCompletedJobs() const;
	bool HasQueuedJobForWorker(WorkerProfile workerProfile) const;

	JobSystemConfig m_config;

	std::atomic <bool> m_isShuttingDown = false;

	// Each JobType receives its own queue without requiring a new member variable and branch per type.
	std::array<std::deque<Job*>, static_cast<std::size_t>(JobType::NUM_TYPES)> m_queuedJobsByType;
	mutable std::mutex		m_queuedJobsMutex;
	
	std::deque <Job*>		m_claimedJobs;
	mutable std::mutex		m_claimedJobsMutex;

	std::deque <Job*>		m_completedJobs;
	mutable std::mutex		m_completedJobsMutex;

	std::vector<JobWorkerThread*> m_workers;
};

class JobWorkerThread
{
	friend class JobSystem;

public:
	JobWorkerThread(int id, WorkerProfile workerProfile, JobSystem* jobSystem);
	~JobWorkerThread();

	void ThreadMain();
	bool CanExecute(JobType jobType) const;

	int m_Id = 0;
	// Store only the readable profile; allowed job types are derived to avoid contradictory state.
	WorkerProfile m_workerProfile = WorkerProfile::GENERAL;
	JobSystem* m_jobSystem = nullptr;
	std::thread* m_thread = nullptr;
};
