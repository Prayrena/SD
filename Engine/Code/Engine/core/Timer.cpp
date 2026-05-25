#include "Engine/core/Timer.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

Timer::Timer(float period, const Clock* clock /*= nullptr*/)
	:m_period(period)
{
	if (!clock)
	{
		m_clock = &Clock::GetSystemClock();
	}
	else
	{
		m_clock = clock;
	}
}

// the default constructor of the timer will create a infinite timer
Timer::Timer()
{
	// Timer(-1.f);
	m_period = -1.f;
	m_clock = &Clock::GetSystemClock();
}

Timer::Timer(const Timer& other)
	: m_period(other.m_period)
	, m_clock(other.m_clock)
	, m_startTime(other.m_startTime)
	, m_isRewinding(other.m_isRewinding)
	, m_rewindStartTime(other.m_rewindStartTime)
{

}

Timer& Timer::operator=(const Timer& other)
{
	m_period = other.m_period;
	m_clock = other.m_clock;
	m_startTime = other.m_startTime;
	m_isRewinding = other.m_isRewinding;
	m_rewindStartTime = other.m_rewindStartTime;

	return *this;
}

void Timer::Start()
{
	m_startTime = m_clock->GetTotalSeconds();
}

void Timer::Stop()
{
	m_startTime = 0.f;
}

void Timer::Restart()
{
	Stop();
	Start();
}

void Timer::Rewind()
{
	m_isRewinding = true;
	m_rewindStartTime = m_clock->GetTotalSeconds();
}

void Timer::RestartAndGoForward()
{
	m_isRewinding = false;
	Restart();
}

bool Timer::IsRewinding()
{
	return m_isRewinding;
}

float Timer::GetElapsedTime() const
{
	if (!m_isRewinding) // go forward
	{
		if (m_startTime > 0.f)
		{
			float totalSeconds = m_clock->GetTotalSeconds();
			return (totalSeconds - m_startTime);
		}
		else
		{
			return 0.f;
		}
	}
	else // go backwards
	{
		if (m_startTime > 0.f)
		{
			float rewindSeconds = m_clock->GetTotalSeconds() - m_rewindStartTime;
			return rewindSeconds;
		}
		else
		{
			return 0.f;
		}	
	}
}

float Timer::GetElapsedFraction() const
{
	if (!m_isRewinding)
	{
		float elapsedTime = GetElapsedTime();
		float fraction = elapsedTime / m_period;
		return fraction;
	}
	else
	{
		float rewindTime = GetElapsedTime();
		float fraction = 1.f - rewindTime / m_period;
		return fraction;
	}
}

bool Timer::IsStopped() const
{
	if (m_startTime == 0.f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Timer::HasPeroidElapsed() const
{
	// if the period is negative means it never pass elapsed the period
	if (m_period < -0.f)
	{
		return false;
	}

	if (IsStopped())
	{
		return false;
	}
	else if (GetElapsedTime() >= m_period )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Timer::HasTimerRewindToStart()
{
	// if the period is negative means it never pass elapsed the period
	if (m_period < -0.f)
	{
		return false;
	}

	if (IsStopped())
	{
		return false;
	}
	else if (GetElapsedTime() >= m_period)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Timer::DecrementPeroidIfElapsed()
{
	if (HasPeroidElapsed())
	{
		m_startTime += m_period;
		return true;
	}
	else
	{
		return false;
	}
}

float Timer::GetDeltaSecondsFromClock() const
{
	if (m_clock)
	{
		return m_clock->GetDeltaSeconds();
	}
	else
	{
		ERROR_AND_DIE("This timer has no clock assigned");
	}
}
