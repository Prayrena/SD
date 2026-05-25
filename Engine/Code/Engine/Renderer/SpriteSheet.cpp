#include "Engine/Renderer/SpriteSheet.hpp"
#include "ThirdParty/stb/stb_image.h"

SpriteDefinition::SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs)
	:m_spriteSheet(spriteSheet), m_spriteIndex(spriteIndex), m_uvAtMins(uvAtMins), m_uvAtMaxs(uvAtMaxs)
{

}

AABB2 SpriteDefinition::GetUVs() const
{
	return AABB2(m_uvAtMins, m_uvAtMaxs);
}

void SpriteDefinition::GetUVs(Vec2& out_uvMins, Vec2& out_uvAtMaxs) const
{
	out_uvMins = m_uvAtMins;
	out_uvAtMaxs = m_uvAtMaxs;
}

SpriteSheet const& SpriteDefinition::GetSpriteSheet() const
{
	return m_spriteSheet;
}

Texture& SpriteDefinition::GetTexture() const
{
	return m_spriteSheet.GetTexture();
}

// todo:???? the default of texture should be square
float SpriteDefinition::GetAspect() const
{
	float width = m_uvAtMaxs.x - m_uvAtMins.x;
	float height = m_uvAtMaxs.y - m_uvAtMins.y;
	return width / height;
}


int SpriteDefinition::GetSpriteIndex() const
{
	return m_spriteIndex;
}

SpriteSheet::SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout)
	:m_texture(texture), m_gridLayout(simpleGridLayout)
{
	float num_height = static_cast<float>(m_gridLayout.y);
	float num_width = static_cast<float>(m_gridLayout.x);
	m_spriteDefs.reserve(m_gridLayout.x * m_gridLayout.y);

	// for the sprite Index, the leftUp corner is (0,0), rightBottom corner is (8,8)
	for (int j = 0; j < m_gridLayout.y; ++j)// column
	{
		for (int i = 0; i < m_gridLayout.x; ++i)// row
		{
			int spriteIndex = (j * simpleGridLayout.x) + i;
			SpriteDefinition* spriteDef = new SpriteDefinition(*this, spriteIndex, 
				Vec2( (1.f / num_width) * i, 1.f - (1.f / num_height) * (j + 1) ),
					Vec2( (1.f / num_width) * (i+1), 1.f - (1.f / num_height) * j )  );

			m_spriteDefs.push_back(*spriteDef);
		}
	}
}

Texture& SpriteSheet::GetTexture() const
{
	return m_texture;
}

int SpriteSheet::GetNumSprites() const
{
	int num_height = m_gridLayout.y;
	int num_width = m_gridLayout.x;
	int numSprites = num_height * num_width;
	return numSprites;
}

SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
{ 
	return m_spriteDefs[spriteIndex];
}

void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
	AABB2 spriteUV = m_spriteDefs[spriteIndex].GetUVs();
	out_uvAtMins = spriteUV.m_mins;
	out_uvAtMaxs = spriteUV.m_maxs;
}

AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
	AABB2 spriteUV = m_spriteDefs[spriteIndex].GetUVs();
	Vec2 const bleed = CalculateSpriteUVBleed();
	spriteUV.m_mins.x += bleed.x;
	spriteUV.m_maxs.x -= bleed.x;
	spriteUV.m_mins.y += bleed.y;
	spriteUV.m_maxs.y -= bleed.y;
	return spriteUV;
}

AABB2 SpriteSheet::GetSpriteUVs(IntVec2 const& spriteCoords) const
{
	int spriteIndex = spriteCoords.x + spriteCoords.y * m_gridLayout.x;
	AABB2 spriteUV = GetSpriteUVs(spriteIndex);
	Vec2 const bleed = CalculateSpriteUVBleed();
	spriteUV.m_mins.x += bleed.x;  
	spriteUV.m_maxs.x -= bleed.x;
	spriteUV.m_mins.y += bleed.y;
	spriteUV.m_maxs.y -= bleed.y;
	return spriteUV;
}
// it is not deleting
Vec2 SpriteSheet::CalculateSpriteUVBleed() const
{
	Texture const& spriteTexture = GetTexture();
	IntVec2 dimension = spriteTexture.GetDimensions();
	Vec2 bleed;
	bleed.x = 1 / (((float)dimension.x) * 4.f);
	bleed.y = 1 / (((float)dimension.y) * 4.f);
	return bleed;
}
