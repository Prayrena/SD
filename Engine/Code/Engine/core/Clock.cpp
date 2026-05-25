#include "Engine/core/Clock.hpp"
#include "Engine/core/Time.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"

static Clock s_theSystemClock; // could not call from other class directly but need to use the function

Clock::Clock()
{
	if (this != &s_theSystemClock)
	{
		m_parent = &s_theSystemClock;
		s_theSystemClock.AddChild(this);
		Tick(); // in case its timer's start time is always zero and judged as stopped all the time
	}
	else
	{
		s_theSystemClock.Tick();
	}
}

Clock::Clock(Clock& parent)
	:m_parent(&parent)
{
	parent.AddChild(this);
}

void Clock::Reset()
{
	// book keeping variables
	m_lastUpdateTimeInSeconds = 0.f;
	m_totalSeconds = 0.f;
	m_deltaSeconds = 0.f;
	m_frameCount = 0;
}

Clock::~Clock()
{
	// delete all its children?
	if (!m_children.empty())
	{
		for (int i = 0; i < m_children.size(); ++i)
		{
			delete m_children[i];
		}
	}

	// delete this clock from its parent
	if (m_parent)
	{
		m_parent->RemoveChild(this);
	}

}

bool Clock::IsPaused() const
{
	return m_isPaused;
}

void Clock::Pause()
{
	m_isPaused = true;
	m_stepSingleFrame = false;
}

void Clock::Unpause()
{
	m_isPaused = false;
	m_stepSingleFrame = false;
}

void Clock::TogglePause()
{
	if (m_isPaused)
	{
		m_isPaused = false;
	}
	else
	{
		m_isPaused = true;
	}
}

void Clock::StepSingleFrame()
{
	if (m_isPaused)
	{
		m_isPaused = false;
	}
	m_stepSingleFrame = true;
}

void Clock::SetTimeScale(float timeScale)
{
	m_timeScale = timeScale;
}

float Clock::GetTimeScale() const
{
	return m_timeScale;
}

float Clock::GetDeltaSeconds() const
{
	return m_deltaSeconds;
}

float Clock::GetTotalSeconds() const
{
	return m_totalSeconds;
}

size_t Clock::GetFrameCount() const
{
	return m_frameCount;
}

bool Clock::HasTotalTimeElapsedPeriod(float period) const
{
	float pastTime = GetTotalSeconds();
	return (pastTime >= period);
}

int Clock::GetFrameRatePerSecond() const
{
	return (int)(1.f / m_deltaSeconds);
}

Clock& Clock::GetSystemClock()
{
	return s_theSystemClock;
}

void Clock::TickSystemClock()
{
	s_theSystemClock.Tick();
}

void Clock::Tick()
{
	// if the clock is paused, it will not proceed
	if (m_isPaused)
	{
		m_deltaSeconds = 0.f;
		return;
	}
	else
	{
		// calculate system delta seconds
		double timeThisFrame = GetCurrentTimeSeconds();
		m_deltaSeconds = (float)timeThisFrame - m_lastUpdateTimeInSeconds;
		m_lastUpdateTimeInSeconds = (float)timeThisFrame;

		// clamp delta seconds because for the first frame of this class
		// the lastUpdateTimeInSeconds is 0 and deltaSeconds might be too big
		m_deltaSeconds = GetClamped(m_deltaSeconds, 0.f, m_maxDeltaSeconds);
	}

	// advance all its children and itself
	Advance(m_deltaSeconds);
}

void Clock::Advance(float deltaSeconds)
{
	// if the clock is paused, the delta Second will be 0
	// affect its children
	if (m_isPaused)
	{
		m_deltaSeconds = 0.f;
		return;
	}

	m_deltaSeconds = deltaSeconds * m_timeScale;
	m_totalSeconds += m_deltaSeconds;
	++m_frameCount;

	// advance all my children clocks
	int numChildren = (int)m_children.size();
	if (numChildren > 0)
	{
		for (int i = 0; i < numChildren; ++i)
		{
			m_children[i]->Advance(m_deltaSeconds);
		}
	}

	// if O is pressed, this frame's deltaSeconds will be calculated as normal
	// but next frame it will be stopped
	if (m_stepSingleFrame)
	{
		m_isPaused = true;
		m_stepSingleFrame = false;
	}
}

void Clock::AddChild(Clock* childClock)
{
	m_children.push_back(childClock);
}

void Clock::RemoveChild(Clock* childClock)
{
	for (int i = 0; i < (int)m_children.size(); ++i)
	{
		if (m_children[i] == childClock)
		{
			m_children.erase(m_children.begin() + i);
			// (*(m_children.begin() + i))->GetDeltaSeconds();
			return;
		}
	}
}

