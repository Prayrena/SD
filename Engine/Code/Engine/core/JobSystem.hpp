#pragma once
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

struct JobSystemConfig
{
	int numberOfWorkers = -1; // if it equals -1, create 1 worker thread per core
};

class Job
{
	friend class JobWorkerThread;

public:

	Job() {}
	virtual ~Job();

	virtual void Execute() = 0;
	std::atomic <JobStatus> m_jobStatus = JobStatus::NUM_STATE; // todo: or this should be in the derived job class?
};

class JobSystem
{
public:
	JobSystem(JobSystemConfig const& config);
	~JobSystem();

	void Startup();
	void ShutDown();

	void CreateWorkers(int numWorkers);
	void DestroyAllWorkers(); // todo: do I need to let the job system destroy all thread workers?

	void QueueJobs(Job* jobToQueue);
	Job* ClaimJob();
	Job* ClaimJob(JobWorkerThread* worker); // specific workers will claim specific jobs
	void ReceiveCompletedJob(Job* job);
	Job* RetrieveCompletedJobs(Job* requestedJob); // Retrieve any completed job, or a specific job

	int GetNumQueuedJobs() const;

	JobSystemConfig m_config;

	std::atomic <bool> m_isShuttingDown = false;

	std::deque <Job*>		m_queuedJobs;
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
	JobWorkerThread(int id, JobSystem* jobSystem);
	~JobWorkerThread();

	void ThreadMain();

	int m_Id = 0;
	JobSystem* m_jobSystem = nullptr;
	std::thread* m_thread = nullptr;
};