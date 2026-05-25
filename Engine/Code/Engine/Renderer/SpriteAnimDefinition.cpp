#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/Renderer/DebugRender.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(const SpriteSheet& spriteSheet, int startSpriteIndex, int endSpriteIndex,
	float durationSeconds, SpriteAnimPlaybackType playbackType /*= SpriteAnimPlaybackType::LOOP*/)
	:m_spriteSheet(spriteSheet), m_startSpriteIndex(startSpriteIndex), m_endSpriteIndex(endSpriteIndex), m_durationSeconds(durationSeconds), m_playbackType(playbackType)
{
	m_eachSpritePlayingDuration = m_durationSeconds / (m_endSpriteIndex - m_startSpriteIndex + 1);
	m_totalSprites = m_endSpriteIndex - m_startSpriteIndex + 1;
}

// what does the seconds mans in the argument: timer or just deltaSeconds
SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float timer) const
{
	// calculate the time needed for playing each sprite
	int		currentSpriteIndex = m_startSpriteIndex;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	switch (m_playbackType)
	{
		case SpriteAnimPlaybackType::ONCE:// for 5-frame anim, plays 0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4...
		{
			if (timer <= m_durationSeconds)
			{
				currentSpriteIndex = (int)(timer / m_eachSpritePlayingDuration) + m_startSpriteIndex;

				return m_spriteSheet.GetSpriteDef(currentSpriteIndex); // m_spriteDefs is protected member of m_spriteSheet
			}
			else
			{
				return m_spriteSheet.GetSpriteDef(m_endSpriteIndex);
			}
			break;
		}
		case SpriteAnimPlaybackType::LOOP:// for 5-frame anim, plays 0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0...
		{
			if (timer <= m_durationSeconds)
			{
				currentSpriteIndex = (int)(timer / m_eachSpritePlayingDuration) + m_startSpriteIndex;
				return m_spriteSheet.GetSpriteDef(currentSpriteIndex); // m_spriteDefs is protected member of m_spriteSheet
			}
			else
			{
				while (timer >= m_durationSeconds)
				{
					timer -= m_durationSeconds;
				}
				currentSpriteIndex = (int)(timer / m_eachSpritePlayingDuration) + m_startSpriteIndex;
				return m_spriteSheet.GetSpriteDef(currentSpriteIndex);
			}
			break;		
		}

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		case SpriteAnimPlaybackType::PINGPONG:// for 5-frame anim, plays 0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1...
		{
			// if we calculate which loop the animation is at, the odd is increasing the sprite index, the even is decreasing index, but this will lead to 0, 1, 2, 3, 4, 4, 3, 2, 1

			// so we will first get the length of duration for animation clip of 0, 1, 2, 3, 4, 3, 2, 1,
			float	newAnimDuration = m_durationSeconds * 2 - m_eachSpritePlayingDuration * 2; // say 8 second for new clip, 5 seconds for old clips, because we are deleting the time for 0 and 4
			int		numAnimPlayed = (int)(timer / newAnimDuration); // calculate how many new animation clips have played
			float	remainingTimer = timer - numAnimPlayed * newAnimDuration; // see how many time is left for the unfinished clip

			currentSpriteIndex = (int)(remainingTimer / m_eachSpritePlayingDuration) + m_startSpriteIndex; // suppose the timer is 6.2, and the result is 6
			if (currentSpriteIndex > m_endSpriteIndex) // current sprite index decreasing 
			{
				currentSpriteIndex = m_endSpriteIndex - (currentSpriteIndex - m_endSpriteIndex); // the result should be showing sprite 2 and the result is 2

				return m_spriteSheet.GetSpriteDef(currentSpriteIndex); // m_spriteDefs is protected member of m_spriteSheet
			}
			else // current sprite index increasing
			{
				currentSpriteIndex = (int)(remainingTimer / m_eachSpritePlayingDuration) + m_startSpriteIndex;

				return m_spriteSheet.GetSpriteDef(currentSpriteIndex);
			}	
			break;
		}
		default:
		{
			ERROR_AND_DIE("Anim mode not supported!");
		}
	}
}

SpriteAnimPlaybackType SpriteAnimDefinition::GetAnimPlaybackModeByString(std::string mode)
{
	if (mode == "Once")
	{
		return SpriteAnimPlaybackType::ONCE;
	}
	if (mode == "Loop")
	{
		return SpriteAnimPlaybackType::LOOP;
	}
	if (mode == "Pingpong")
	{
		return SpriteAnimPlaybackType::PINGPONG;
	}
	ERROR_AND_DIE("Anim mode not supported!");
}

float SpriteAnimDefinition::GetDurationSeconds()
{
	return m_durationSeconds;
}
