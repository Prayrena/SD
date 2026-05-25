#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/IntVec2.hpp"

enum TextDrawMode
{
	SHRINK_TO_FIT,
	OVERRUN
};

struct GlyphData
{
	int		m_glyphSheetIndex = 0;
	float	m_lineHeightFractionBeforeGlyph = 0.f; // A value: xOffset / lineHeight
	float	m_lineHeightFractionDuringGlyph = 1.f; // B value: characterHeight / lineHeight
	float	m_lineHeightFractionAfterGlyph = 0.f;  // C value: xAdvance	/ lineHeight
};

class BitmapFont
{
	friend class Renderer; // Only the Renderer can create new BitmapFont objects!
	friend class XRRenderer;

public:
	BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture, IntVec2 rowAndColumns = IntVec2(16,16));

	Texture& GetTexture();

	// assume that the alignment means the where is the center 
	void AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& verts,
		float cellHeight, std::string const& text,
		Rgba8 const& tint = Rgba8::WHITE, float cellAspect = 1.f,
		Vec2 const& alignment = Vec2(.5f, .5f), int maxGlyphsToDraw = 99999999, float lineSpace = 0.2f);

	void AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textLeftBottomPos,
		float cellHeight, std::string const& text, Rgba8 const& tint = Rgba8::WHITE, float cellAspect = 1.f);

	void AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, std::string const& text, AABB2 const& box, float cellHeight, Vec2 const& alignment = Vec2(0.5f, 0.5f), Rgba8 const& tint = Rgba8::WHITE,
		float lineSpacingMultiplier = 0.5f, float cellAspect = 0.6f,  TextDrawMode mode = TextDrawMode::SHRINK_TO_FIT, int maxGlyphsToDraw=999999);// take cellAspect = width / height	
		
	float GetTextWidth(float cellHeight, std::string const& text, float cellAspect = 1.f);// take cellAspect = width / height

protected:
	float GetGlyphAspect(int glyphUnicode) const; // For now this will always return 1.0f!!!
	AABB2 GetUVForCharIndex(int index);

protected:
	std::string	m_fontFilePathNameWithNoExtension;
	std::string m_fontName;
	SpriteSheet	m_fontGlyphsSpriteSheet;
};
