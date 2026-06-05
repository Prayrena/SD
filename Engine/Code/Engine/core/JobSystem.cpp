#include "Engine/core/JobSystem.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"
#include <algorithm>

using namespace std;

namespace
{
std::size_t GetJobTypeIndex(JobType jobType)
{
	// Array-backed queues require a real job category; NUM_TYPES is only the enum count sentinel.
	GUARANTEE_OR_DIE(IsValidJobType(jobType), "Job type must be a valid queue category");
	return static_cast<std::size_t>(jobType);
}
}

//------------------------------------------------------------------------------------------------
// std::atomic
// std::mutex
// std::lock( mutex, mutex, ... )			- blocking deadlock-safe lock of N mutexes
// std::lock_guard scopedLock( mutex )		- scoped lock wrapper of a single mutex (ctor=lock, dtor=unlock)
// std::unique_lock( mutex )				- scoped lock wrapper of a single mutex (ctor=lock, dtor=unlock), can use with CVs
// std::scoped_lock( mutex, mutex, ... )	- scoped lock wrapper of N mutexes (ctor=lock all, dtor=unlock all; deadlock-safe)
// std::condition_variable					- used to notify (wake) blocking (suspended) threads waiting on this CV
// std::condition_variable_any				- like std::condition_variable but works with other locks besides unique_lock
// std::shared_lock( mutex )				- used by multiple readers to co-lock a shared mutex (single-writer uses unique_lock)
// std::recursive_mutex						- can be re-acquired multiple times by the same thread; .unlock() pops .lock() until none
// std::shared_mutex						- useful for multiple readers via .lock_shared() and single writer via .lock()


// EXAMPLE: unique_lock
/*
std::mutex mutexA;
std::mutex mutexB;
{
	std::unique_lock<std::mutex> lockMutexA( mutexA, std::defer_lock );
	std::unique_lock<std::mutex> lockMutexB( mutexB, std::defer_lock );
	std::lock( mutexA, mutexB ); // locks both unique_locks without deadlock
	// Both unique_lock objects automatically call .unlock() on their mutexes when they go out of scope and die
}
*/

mutex					g_newJobAvailableConditionVariableMutex;
condition_variable		g_newJobAvailableConditionVariable;

JobWorkerThread::JobWorkerThread(int id, WorkerProfile workerProfile, JobSystem* jobSystem)
	: m_Id(id)
	, m_workerProfile(workerProfile)
	, m_jobSystem(jobSystem)
{	
	m_thread = new std::thread(&JobWorkerThread::ThreadMain, this);
}

JobWorkerThread::~JobWorkerThread()
{
	m_thread->join(); 
}

void JobWorkerThread::ThreadMain()
{
	// Spins continuously, looking for unclaimed Job work to claim 
	// repeating this until signaled to stop looping and exit
	while (!m_jobSystem->m_isShuttingDown)
	{
		Job* jobToExecute = m_jobSystem->ClaimJob(this);
		if (jobToExecute)
		{
			jobToExecute->Execute();
			m_jobSystem->ReceiveCompletedJob(jobToExecute);
		}
		else
		{
			// old method
			// there is also std::chrono::hours, minutes, seconds, milliseconds, microseconds, nanoseconds to use
			// std::this_thread::sleep_for((std::chrono::microseconds(1))); // sleeping for 1ms if none job is found

			unique_lock newJobUniqueLock(g_newJobAvailableConditionVariableMutex);
			g_newJobAvailableConditionVariable.wait(newJobUniqueLock, [this]()
			{
				return m_jobSystem->m_isShuttingDown || m_jobSystem->HasQueuedJobForWorker(m_workerProfile);
			});
		}
	}
}

bool JobWorkerThread::CanExecute(JobType jobType) const
{
	// Derive permissions from the profile so a worker cannot carry a profile and a conflicting mask.
	return (GetAllowedJobTypes(m_workerProfile) & GetJobTypeBit(jobType)) != 0u;
}

JobSystem::JobSystem(JobSystemConfig const& config)
	: m_config(config)
{

}

JobSystem::~JobSystem()
{

}

void JobSystem::Startup()
{
	// Reset this so the same JobSystem object can be started after construction or restart.
	m_isShuttingDown = false;
	int numCores = std::thread::hardware_concurrency(); // get the numbers of threads that could run at the same time
	int numDiskIOWorkers = m_config.createDedicatedDiskIOWorker ? 1 : 0;
	int numGenericWorkers = 1;
	if (m_config.numberOfWorkers == -1)
	{
		// Keep one core available for the main thread; one of the remaining workers is reserved for disk I/O.
		int totalWorkerCount = (std::max)(1, numCores - 1);
		numGenericWorkers = (std::max)(1, totalWorkerCount - numDiskIOWorkers);
	}
	else
	{
		// A JobSystem with zero workers would silently leave queued jobs forever.
		numGenericWorkers = (std::max)(1, m_config.numberOfWorkers - numDiskIOWorkers);
	}

	CreateWorkers(numGenericWorkers, numDiskIOWorkers);
}

void JobSystem::ShutDown()
{
	m_isShuttingDown = true;
	g_newJobAvailableConditionVariable.notify_all();	// we have to wake up all workers to shut all workers down

	DestroyAllWorkers();
}

void JobSystem::CreateWorkers(int numGenericWorkers, int numDiskIOWorkers)
{
	for (int workerIndex = 0; workerIndex < numGenericWorkers; ++workerIndex)
	{
		JobWorkerThread* worker = new JobWorkerThread(workerIndex, WorkerProfile::GENERAL, this);
		m_workers.push_back(worker);
	}

	for (int workerIndex = 0; workerIndex < numDiskIOWorkers; ++workerIndex)
	{
		// Disk I/O jobs can block unpredictably, so they get a worker that never claims CPU generation work.
		int workerId = numGenericWorkers + workerIndex;
		JobWorkerThread* worker = new JobWorkerThread(workerId, WorkerProfile::DISK_IO, this);
		m_workers.push_back(worker);
	}
}

void JobSystem::DestroyAllWorkers()
{
	for (int i = 0; i < (int)m_workers.size(); ++i)
	{
		delete m_workers[i];
		m_workers[i] = nullptr;
	}
	// Clear stale worker slots so a later startup cannot see deleted workers.
	m_workers.clear();
}

Job* JobSystem::ClaimJob()
{ 
	// Default claims remain generic so older callers do not accidentally run disk I/O work.
	std::lock_guard<std::mutex> queuedJobsLock(m_queuedJobsMutex);
	std::deque<Job*>& queuedJobs = m_queuedJobsByType[GetJobTypeIndex(JobType::GENERIC)];
	if (!queuedJobs.empty())
	{
		Job* claimedJob = queuedJobs.front();
		queuedJobs.pop_front();
		// Status changes here so game code never needs to mark jobs as claimed.
		claimedJob->m_jobStatus = JobStatus::CLAIMED;
		return claimedJob;
	}

	return nullptr;
}

Job* JobSystem::ClaimJob(JobWorkerThread* worker)
{
	if (!worker)
	{
		return ClaimJob();
	}

	std::lock_guard<std::mutex> queuedJobsLock(m_queuedJobsMutex);
	// Queues are indexed by JobType so new categories do not require another dedicated claim branch.
	for (std::size_t jobTypeIndex = 0; jobTypeIndex < m_queuedJobsByType.size(); ++jobTypeIndex)
	{
		JobType jobType = static_cast<JobType>(jobTypeIndex);
		if (!worker->CanExecute(jobType))
		{
			continue;
		}

		std::deque<Job*>& queuedJobs = m_queuedJobsByType[jobTypeIndex];
		if (!queuedJobs.empty())
		{
			Job* claimedJob = queuedJobs.front();
			queuedJobs.pop_front();
			// Status changes here so game code never needs to mark jobs as claimed.
			claimedJob->m_jobStatus = JobStatus::CLAIMED;
			return claimedJob;
		}
	}

	return nullptr;
}

void JobSystem::ReceiveCompletedJob(Job* job)
{
	m_completedJobsMutex.lock();
	m_completedJobs.push_back(job);
	// Mark completed while holding the completed-list lock so retrieval cannot race the status update.
	job->m_jobStatus = JobStatus::COMPLETED;
	m_completedJobsMutex.unlock();
}

Job* JobSystem::RetrieveCompletedJobs(Job* requestedJob)
{
	if (requestedJob) // request a specific job
	{
		m_completedJobsMutex.lock();
		std::deque<Job*>::iterator iter;
		for (iter = m_completedJobs.begin(); iter != m_completedJobs.end(); ++iter)
		{
			if (*iter == requestedJob)
			{
				Job* retrievedjob = *iter;
				// Retrieval transfers ownership back to the caller, so remove it from the completed list.
				m_completedJobs.erase(iter);
				retrievedjob->m_jobStatus = JobStatus::RETRIEVED;
				m_completedJobsMutex.unlock();
				return retrievedjob;
			}
		}
		m_completedJobsMutex.unlock(); 
		return nullptr; // did not find the specific job
	}
	else
	{
		// not requested specific job
		m_completedJobsMutex.lock();
		if (!m_completedJobs.empty()) // return the front one when the deque is not empty
		{
			Job* retrievedjob = m_completedJobs.front();
			m_completedJobs.pop_front();
			retrievedjob->m_jobStatus = JobStatus::RETRIEVED;
			m_completedJobsMutex.unlock();
			return retrievedjob;
		}
		else
		{
			// return null when the completed job deque is empty
			m_completedJobsMutex.unlock();
			return nullptr;
		}
	}
}

int JobSystem::GetNumQueuedJobs() const
{
	return GetNumQueuedJobs(JobType::GENERIC);
}

int JobSystem::GetNumQueuedJobs(JobType jobType) const
{
	std::lock_guard<std::mutex> queuedJobsLock(m_queuedJobsMutex);
	return (int)m_queuedJobsByType[GetJobTypeIndex(jobType)].size();
}

int JobSystem::GetNumCompletedJobs() const
{
	m_completedJobsMutex.lock();
	int numJobs = (int)m_completedJobs.size();
	m_completedJobsMutex.unlock();
	return numJobs;
}

bool JobSystem::HasQueuedJobForWorker(WorkerProfile workerProfile) const
{
	std::lock_guard<std::mutex> queuedJobsLock(m_queuedJobsMutex);
	// Check only queues allowed by the profile so sleeping workers wake for work they can actually claim.
	for (std::size_t jobTypeIndex = 0; jobTypeIndex < m_queuedJobsByType.size(); ++jobTypeIndex)
	{
		JobType jobType = static_cast<JobType>(jobTypeIndex);
		bool canExecuteJobType = (GetAllowedJobTypes(workerProfile) & GetJobTypeBit(jobType)) != 0u;
		if (canExecuteJobType && !m_queuedJobsByType[jobTypeIndex].empty())
		{
			return true;
		}
	}

	return false;
}

void JobSystem::QueueJobs(Job* jobToQueue)
{
	// Queueing is the engine-owned transition out of CREATED.
	jobToQueue->m_jobStatus = JobStatus::QUEUED;

	{
		std::lock_guard<std::mutex> queuedJobsLock(m_queuedJobsMutex);
		// Index by type so future categories stay isolated without adding parallel queue members.
		m_queuedJobsByType[GetJobTypeIndex(jobToQueue->GetJobType())].push_back(jobToQueue);
	}

	// wake up all thread, because we may have different workers
	g_newJobAvailableConditionVariable.notify_all();
}

Job::~Job()
{

}

JobType Job::GetJobType() const
{
	return JobType::GENERIC;
}
