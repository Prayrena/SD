#pragma once
#include <vector>

// Hierarchical clock that inherits time scale
// Parent clocks pass scaled delta seconds down to child clocks to be used as their base delta seconds
// Child clocks in return scale that time and pass that down to their children.
// There is only one system clock at the root of the hierarchy
//
class Clock
{
public:
	// default clock constructor uses the system clock as the parent of the new clock
	// but if the 
	Clock();

	// constructor to specify a parent clock for the new clock
	// if the parent is null, then the clock we are constructing is the system clock
	explicit Clock(Clock& parent);

	// Deconstructor, unparents ourself and our children's to avoid crashes 
	// but does not otherwise try to fix up the clock hierarchy, which is the responsibility of the user of this class
	~Clock();
	Clock(Clock const& copy) = delete;

	// Reset all book keeping variables' values back to zero 
	// then set the last updated time to be current system time
	void Reset();

	bool IsPaused() const;
	void Pause();
	void Unpause();
	void TogglePause();

	// unpause for frame then pause again in next frame
	void StepSingleFrame();

	// set and get the value by which this clock scales delta seconds
	void  SetTimeScale(float timeScale);
	float GetTimeScale() const;

	float GetDeltaSeconds() const;
	float GetTotalSeconds() const;
	size_t GetFrameCount() const;

	bool  HasTotalTimeElapsedPeriod(float period) const;

	int GetFrameRatePerSecond() const;
//----------------------------------------------------------------------------------------------------------------------------------------------------
	// return a reference to a static system clock instance
	// if we don't return Clock&, it is going to return a copy of the instance that the ptr is pointing to
	static Clock& GetSystemClock();

	// called in Begin frame to tick the system clock, which will in return advance the system clock
	// which will in return advance all of its children, thus updating the entire hierarchy
	static void TickSystemClock();

protected:
	// calculate the current delta seconds and clamp it to the max delta time
	// sets the last updated time, then calls advance all of its children
	// thus updating the entire hierarchy
	// !!! ONLY PARENT WILL TICK, which call all its children Advance
	void Tick();

	// calculates delta seconds based on pausing and time scale
	// updates all remaining book keeping variables
	// calls advance on all children clocks and pass down our delta seconds
	// and handles pausing after frames for stepping single frames
	void Advance(float deltaSeconds);

	// Add a child clock as one of our children
	// Does not handle cases where the child clock already has a parent
	void AddChild(Clock* childClock);

	// removes a child clock from our children if it is a child
	// otherwise does nothing
	void RemoveChild(Clock* childClock);

protected:
	// parent clock. Will be nullptr for the root clock
	Clock* m_parent = nullptr;

	// all children of this clock
	std::vector<Clock*> m_children;

	// book keeping variables
	float	m_lastUpdateTimeInSeconds = 0.f;
	float	m_totalSeconds = 0.f;
	float	m_deltaSeconds = 0.f;
	size_t  m_frameCount = 0;

	// time scale for this clock
	float	m_timeScale = 1.f;

	// pauses the clock completely
	bool	m_isPaused = false;

	// for single stepping frames affect isPaused
	bool	m_stepSingleFrame = false;

	// max delta time. Useful for preventing large time steps when stepping in a debugger
	// why the max deltaSeconds is not 1/60
	float	m_maxDeltaSeconds = 0.1f;
};