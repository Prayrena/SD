#pragma once
#include "Engine/Renderer/SpriteSheet.hpp"

enum class SpriteAnimPlaybackType
{
	ONCE,		// for 5-frame anim, plays 0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4...
	LOOP,		// for 5-frame anim, plays 0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0...
	PINGPONG,	// for 5-frame anim, plays 0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1...
};

//------------------------------------------------------------------------------------------------
class SpriteAnimDefinition // only one definition for each kind of animation
{
public:
	SpriteAnimDefinition() = default;
	~SpriteAnimDefinition() = default;
	SpriteAnimDefinition(SpriteSheet const& spriteSheet, int startSpriteIndex, int endSpriteIndex,
		float durationSeconds, SpriteAnimPlaybackType playbackType = SpriteAnimPlaybackType::ONCE);

	const SpriteDefinition& GetSpriteDefAtTime(float timer) const; // seconds is the timer from the entity
	static SpriteAnimPlaybackType GetAnimPlaybackModeByString(std::string mode);
	float GetDurationSeconds();

private:
	const SpriteSheet&		m_spriteSheet;

	int						m_startSpriteIndex = -1;
	int						m_endSpriteIndex = -1;
	float					m_durationSeconds = 1.f;// for storing the time that how long the anim should play

	SpriteAnimPlaybackType	m_playbackType = SpriteAnimPlaybackType::LOOP;
	float					m_eachSpritePlayingDuration = 0.f;
	int						m_totalSprites = 0;
};
