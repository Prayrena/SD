#pragma once
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include <vector>

class SpriteSheet;

class SpriteDefinition
{
public:
	explicit SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs);
	void GetUVs(Vec2& out_uvMins, Vec2& out_uvAtMaxs) const;
	AABB2 GetUVs() const;
	SpriteSheet const& GetSpriteSheet() const;
	Texture& GetTexture() const;
	float	 GetAspect() const;
	int		 GetSpriteIndex() const;

	friend class SpriteSheet;

protected:
	SpriteSheet const& m_spriteSheet;
	int				   m_spriteIndex = -1;
	Vec2			   m_uvAtMins = Vec2::ZERO;
	Vec2			   m_uvAtMaxs = Vec2::ONE;
};

class SpriteSheet
{
public:
	explicit SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout);
	~SpriteSheet() = default;

	Texture&				 GetTexture() const;
	int						 GetNumSprites() const;
	SpriteDefinition const&  GetSpriteDef(int spriteIndex) const;
	void					 GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int SpriteIndex) const;// use function to rewrite the input value as output
	AABB2					 GetSpriteUVs(int spriteIndex) const;
	AABB2					 GetSpriteUVs(IntVec2 const& spriteCoords) const;
	Vec2					 CalculateSpriteUVBleed() const; // get texture dimension and cut out the bleed

protected:
	Texture&					  m_texture;// todo: ???? reference members must be set in constructor's initializer list
	IntVec2						  m_gridLayout;
	std::vector<SpriteDefinition> m_spriteDefs;
};