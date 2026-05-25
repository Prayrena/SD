#pragma once

class Clock;

// Timer class that can be attached to any clock in a hierarchy and correctly handles duration 
// regardless of update frequency
//
class Timer
{
public:
	Timer();
	// create a clock with a period and the specified clock
	// if the clock is null, use the system clock
	explicit Timer(float period, const Clock* clock = nullptr);
	Timer(const Timer& other);
	Timer& operator=(const Timer& other);
	~Timer() = default;

	// set the start time to the clock's current total time
	void Start();

	// set the start time back to zero
	void Stop();

	// set the timer stop and start
	void Restart();

	// set the timer stop and rewind
	void Rewind();
	// stop rewind and go forward
	void RestartAndGoForward();
	bool IsRewinding();

	// return zero if stopped, otherwise returns the time elapsed between the clock's current time 
	// and this timer's start time
	float GetElapsedTime() const;

	// return the elapsed time as a percentage of our period. This can be greater than 1
	float GetElapsedFraction() const;
	
	// returns true if this timer's start time is zero
	bool IsStopped() const;

	// return true if our elapsed time is greater than our period and we are not stopped
	bool HasPeroidElapsed() const;

	// return true if the timer is rewinding and reach to when it starts
	bool HasTimerRewindToStart();
	
	// if a period has elapsed and we are not stopped, decrements a period by adding a period to the start time and returns true
	// Generally called within a loop until it returns false so the caller can process each elapsed period
	bool DecrementPeroidIfElapsed();

	float GetDeltaSecondsFromClock() const;

//----------------------------------------------------------------------------------------------------------------------------------------------------
	// The clock to use for getting the current time
	Clock const* m_clock = nullptr;

	// Clock time at which the timer was started.
	// This is incremented by one period each time we decrement a period, however, so this is not an absolute start time
	// It is the start time of all periods that have not yet been decremented
	float m_startTime = 0.f;

	// record when the timer start to rewind
	float m_rewindStartTime = 0.f;

	// The time interval it takes for a period to elapse
	float m_period = 0.f;

	bool m_isRewinding = false;
};