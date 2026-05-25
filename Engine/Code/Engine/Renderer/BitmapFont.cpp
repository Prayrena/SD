#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/core/EngineCommon.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/core/StringUtils.hpp"

using namespace std;

BitmapFont::BitmapFont(char const* fontFilePathNameWithNoExtension, Texture& fontTexture, IntVec2 rowAndColumns /*= IntVec2(16,16)*/)
	:m_fontFilePathNameWithNoExtension(fontFilePathNameWithNoExtension),
	m_fontGlyphsSpriteSheet(fontTexture, rowAndColumns)
{

}

Texture& BitmapFont::GetTexture()
{
	return m_fontGlyphsSpriteSheet.GetTexture();
}

void BitmapFont::AddVertsForText3DAtOriginXForward(std::vector<Vertex_PCU>& verts, float cellHeight, std::string const& text, 
		Rgba8 const& tint /*= Rgba8::WHITE*/, float cellAspect /*= 1.f*/, Vec2 const& alignment /*= Vec2(.5f, .5f)*/, 
		int maxGlyphsToDraw /*= 99999999*/, float lineSpace /*= 0.2f*/)
{
	// split text into multiple lines
	Strings textLines = SplitStringOnDelimiter(text, '\n');
	int numLines = (int)textLines.size();

	// calculate the center and the bottom left pos
	float cellWidth = cellHeight * cellAspect;
	// assume that the bottom left corner is at the origin at first
	float maxLineLength = 0.f;
	float fullLineHeight = cellHeight * (1.f + lineSpace);

	for (int i = 0; i < numLines; ++i)
	{
		float lineLength = textLines[i].size() * cellWidth;
		maxLineLength = max(lineLength, maxLineLength);
	}
	float shiftRight = maxLineLength * 0.5f; // make the whole block at the center of the HMD
	
	Vec3 textBoardOrigin = Vec3::ZERO;
	float totalHeight = (cellHeight * numLines + (numLines - 1) * cellHeight * lineSpace);
	float shiftUp = totalHeight * alignment.y;
	textBoardOrigin += Vec3(0.f, 0.f, shiftUp);

	for (int lineIndex = 0; lineIndex < numLines; ++lineIndex)
	{
		// calculate padding to decide where to start
		int numChar = (int)textLines[lineIndex].size();
		float wholeLineLength = numChar * cellWidth;
		float padding = (maxLineLength - wholeLineLength);
		textBoardOrigin.y = 0.f + padding * alignment.x - shiftRight;

		for (int charIndex = 0; charIndex < textLines[lineIndex].size(); ++charIndex)
		{
			if (maxGlyphsToDraw <= 0)
			{
				return;
			}

			Vec3 BL = textBoardOrigin + Vec3(0.f, cellWidth * charIndex, 0.f);
			Vec3 BR = BL + Vec3(0.f, cellWidth, 0.f);
			Vec3 TR = BL + Vec3(0.f, cellWidth, cellHeight);
			Vec3 TL = BL + Vec3(0.f, 0.f, cellHeight);
			AABB2 uvBounds = m_fontGlyphsSpriteSheet.GetSpriteUVs((int)textLines[lineIndex][charIndex]);
			AddVertsForQuad3D(verts, BL, BR, TR, TL, tint, uvBounds);
			--maxGlyphsToDraw;
		}
		 
		// move to write the next line
		textBoardOrigin -= Vec3(0.f, 0.f, fullLineHeight);
	}
}

void BitmapFont::AddVertsForText2D(std::vector<Vertex_PCU>& vertexArray, Vec2 const& textLeftBottomPos, 
	float cellHeight, std::string const& text, Rgba8 const& tint, float cellAspect)
{
	//float charWidth = GetTextWidth(cellHeight, text, cellAspect);
	float cellWidth = cellHeight * cellAspect;

	for (int charIndex = 0; charIndex < (int)text.size(); ++charIndex)
	{
		AABB2 posBounds;
		posBounds.m_mins = textLeftBottomPos + Vec2(cellWidth * charIndex, 0.f);
		posBounds.m_maxs = posBounds.m_mins + Vec2(cellWidth, cellHeight);
		AABB2 uvBounds = m_fontGlyphsSpriteSheet.GetSpriteUVs((int)text[charIndex]);
		AddVertsUVForAABB2D(vertexArray, posBounds, tint, uvBounds);
	}
}

void BitmapFont::AddVertsForTextInBox2D(std::vector<Vertex_PCU>& vertexArray, std::string const& text, AABB2 const& box, float cellHeight, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/,
	Rgba8 const& tint /*= Rgba8::WHITE*/, float lineSpacingMultiplier /*= 0.5f*/, float cellAspect /*= 0.6f*/,  TextDrawMode mode /*= TextDrawMode::SHRINK_TO_FIT*/, int maxGlyphsToDraw/*=999999*/)
{
	// split text into multiple lines
	Strings textLines = SplitStringOnDelimiter(text, '\n');
	int numLines = (int)textLines.size();

	// get the width and height of the text box and paragraph box
	float lineSpacingHeight = lineSpacingMultiplier * cellHeight;
	float paragraphBoxHeight = (float)numLines * cellHeight + (float)(numLines - 1) * lineSpacingHeight;

	float boxWidth  = box.m_maxs.x - box.m_mins.x;
	float boxHeight = box.m_maxs.y - box.m_mins.y;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// both mode do not modify the cell aspect of each character
	// but in Shrink to fit mode, we have to calculate the uniform scale to modify the size of the paragraph box
	if (mode == TextDrawMode::SHRINK_TO_FIT)
	{
		// get the length of the longest text line
		float paragraphBoxWidth = 0.f;
		for (int lineIndex = 0; lineIndex < numLines; ++lineIndex)
		{
			float lineWidth = GetTextWidth(cellHeight, textLines[lineIndex], cellAspect);
			if (lineWidth > paragraphBoxWidth)
			{
				paragraphBoxWidth = lineWidth;
			}
		}

		// calculate the scale of shrinking / expanding of the paragraph box and the scale center
		float widthScale = boxWidth / paragraphBoxWidth;
		float heightScale = boxHeight / paragraphBoxHeight;
		float uniformScale;
		Vec2  centerOfParagraph = box.GetCenter();
		if (widthScale <= heightScale)
		{
			uniformScale = widthScale;
		}
		else
		{
			uniformScale = heightScale;
		}

		if (uniformScale > 1.f)// the paragraph box does not scale up when it is smaller than the box
		{
			uniformScale = 1.f;
		}
		// update the paragraph dimension after scaling
		cellHeight *= uniformScale;
		paragraphBoxHeight *= uniformScale;
		paragraphBoxWidth *= uniformScale;
	}
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// calculate the position of each text line, from the top line to the bottom line
	for (int lineIndex = 0; lineIndex < numLines; ++lineIndex)
	{
		// calculate the x and y dimension of the padding of each line
		float lineLength = GetTextWidth(cellHeight, textLines[lineIndex], cellAspect);
		Vec2 padding;
		padding.x = boxWidth - lineLength;
		padding.y = boxHeight - paragraphBoxHeight;
		// calculate the bottom left corner position of the text line
		Vec2 textLeftBottomPos;
		textLeftBottomPos.x = padding.x * alignment.x + box.m_mins.x;
		textLeftBottomPos.y = padding.y * alignment.y + box.m_mins.y + (cellHeight + lineSpacingHeight) * (float)(numLines - (lineIndex + 1)); // e.g. 3 lines in total, line 0 has 2 spacing and 2 * cell height, line 2 has 0 cell height and 0 spacing height

		// the maxGlyphsToDraw means the maximum characters that the paragraph could draw
		std::string newTextLine;
		for (int i = 0; i < textLines[lineIndex].size(); ++i)
		{
			if (i < (maxGlyphsToDraw - 1))
			{
				newTextLine.push_back(textLines[lineIndex][i]);
			}
			// the rest of the string character will not be drawn
		}
		AddVertsForText2D(vertexArray, textLeftBottomPos, cellHeight, newTextLine, tint, cellAspect);
		maxGlyphsToDraw -= (int)textLines[lineIndex].length();
	}
}

float BitmapFont::GetTextWidth(float cellHeight, std::string const& text, float cellAspect)
{
	// take cellAspect = width / height
	int numChar = (int)text.length();
	float cellWidth = cellHeight * cellAspect;
	float totalWidth = cellWidth * numChar;
	// float totalWidth = 0.f;
	// for (int i = 0; i < numChar; ++i)
	// {
	// 	unsigned char character = text[i];
	// 	totalWidth += GetGlyphAspect(character);
	// }
	return totalWidth;
}

float BitmapFont::GetGlyphAspect(int glyphUnicode) const
{
	UNUSED(glyphUnicode);
	return 1.f;
}

AABB2 BitmapFont::GetUVForCharIndex(int index)
{
	// get the coords for the index, ! is 33, but it is the 1st in the sprite sheet
	// index -= 32;
	IntVec2 charCoords;
	charCoords.x = (index % 16); // column
	charCoords.y = (index / 16); // row

	// for the sprite Index, the leftUp corner is (0,0), rightBottom corner is (1,1)
	// for openGL, the uv it needs to render, the  the leftBottom corner is (0,0), rightUp corner is (1,1)
	AABB2 spriteUV;
	spriteUV.m_mins = Vec2( (1.f / 16) * (float)(charCoords.x),		1.f - (1.f / 16) * (float)(charCoords.y + 1) );
	spriteUV.m_maxs = Vec2( (1.f / 16) * (float)(charCoords.x + 1), 1.f - (1.f / 16) * (float)(charCoords.y) );
	
	return spriteUV;
}