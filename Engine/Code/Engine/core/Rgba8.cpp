#include "Engine/core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "ThirdParty/Physx/include/foundation/PxSimpleTypes.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------
Rgba8 const Rgba8::RED = Rgba8(255, 0, 0, 255);
Rgba8 const Rgba8::RED_TRANSPARENT = Rgba8(255, 0, 0, 150);
Rgba8 const Rgba8::RED_CLEAR = Rgba8(255, 0, 0, 0);
Rgba8 const Rgba8::BURNT_RED = Rgba8(152, 20, 20, 255);
Rgba8 const Rgba8::WARM_PURPLE = Rgba8(168, 94, 181, 255);
Rgba8 const Rgba8::ROYAL_PURPLE = Rgba8(84, 0, 129, 255);
Rgba8 const Rgba8::GREEN = Rgba8(0, 255, 0, 255);
Rgba8 const Rgba8::NEON_GREEN = Rgba8(218, 255, 2, 255);
Rgba8 const Rgba8::GREEN_TRANSPARENT = Rgba8(0, 255, 0, 120);
Rgba8 const Rgba8::GREENISH_GRAY = Rgba8(148, 190, 180, 255);
Rgba8 const Rgba8::BLUE_LIGHT = Rgba8(200, 220, 240, 255);
Rgba8 const Rgba8::BLUE = Rgba8(0, 0, 255, 255);
Rgba8 const Rgba8::NEON_BLUE = Rgba8(70, 160, 240, 255);
Rgba8 const Rgba8::PASTEL_BLUE = Rgba8(117, 158, 224, 255);
Rgba8 const Rgba8::TEAL_BLUE = Rgba8(10, 90, 175, 255);
Rgba8 const Rgba8::CYSTAL_BLUE = Rgba8(32, 142, 239, 255);
Rgba8 const Rgba8::PURPLE_BLUE = Rgba8(45, 63, 170, 255);
Rgba8 const Rgba8::BLUE_MVT = Rgba8(50, 80, 150, 255);
Rgba8 const Rgba8::BLUE_MVTHL = Rgba8(100, 150, 255, 255);
Rgba8 const Rgba8::CYAN = Rgba8(0, 255, 255, 255);
Rgba8 const Rgba8::MAGENTA = Rgba8(255, 0, 255, 255);
Rgba8 const Rgba8::PALE_PINK = Rgba8(231, 184, 184, 150);
Rgba8 const Rgba8::PALE_PINK_TRANSPARENT = Rgba8(231, 184, 184, 210);
Rgba8 const Rgba8::GRAPEFRUIT_PINK = Rgba8(243, 143, 140, 255);
Rgba8 const Rgba8::GRAPEFRUIT_PINK_TRANSPARENT = Rgba8(243, 143, 140, 210);
Rgba8 const Rgba8::PINK_TRANSPARENT = Rgba8(243, 143, 140, 120);
Rgba8 const Rgba8::NEON_PINK = Rgba8(192, 2, 196, 255);
Rgba8 const Rgba8::BLUSH_PINK = Rgba8(240, 105, 170, 255);
Rgba8 const Rgba8::SALMON_PINK = Rgba8(235, 135, 147, 255);
Rgba8 const Rgba8::YELLOW = Rgba8(255, 255, 0, 255);
Rgba8 const Rgba8::IVORY_YELLOW = Rgba8(242, 233, 210, 255);
Rgba8 const Rgba8::YELLOW_TRANSPARENT = Rgba8(255, 255, 0, 50);
Rgba8 const Rgba8::Naples_Yellow = Rgba8(248, 220, 103, 255);
Rgba8 const Rgba8::CANDLE_YELLOW = Rgba8(247, 235, 0, 255);
Rgba8 const Rgba8::CANDLE_YELLOW_TRANSPARENT = Rgba8(247, 235, 0, 120);
Rgba8 const Rgba8::BRIGHT_ORANGE = Rgba8(252, 112, 0, 255);
Rgba8 const Rgba8::LIGHT_ORANGE = Rgba8(205, 96, 16, 255);
Rgba8 const Rgba8::DEEP_ORANGE = Rgba8(237, 65, 26, 255);
Rgba8 const Rgba8::PEACH_ORANGE = Rgba8(245, 174, 104, 255);
Rgba8 const Rgba8::BLACK = Rgba8(0, 0, 0, 255);
Rgba8 const Rgba8::BLACK_TRANSPARENT = Rgba8(0, 0, 0, 120);
Rgba8 const Rgba8::TOTAL_TRANSPARENT = Rgba8(0, 0, 0, 0);
Rgba8 const Rgba8::WHITE = Rgba8(255, 255, 255, 255);
Rgba8 const Rgba8::WHITE_TRANSPARENT = Rgba8(255, 255, 255, 30);
Rgba8 const Rgba8::GRAY				= Rgba8(100, 100, 100, 255);
Rgba8 const Rgba8::GRAY_BLUE		= Rgba8(115, 140, 166, 255);
Rgba8 const Rgba8::GRAY_LIGHT		= Rgba8(230, 230, 230, 255);
Rgba8 const Rgba8::GRAY_Dark		= Rgba8(50, 50, 50, 255);
Rgba8 const Rgba8::GRAY_TRANSPARENT = Rgba8(100, 100, 100, 100);


Rgba8::Rgba8(unsigned char initialr, unsigned char initialg, unsigned char initialb, unsigned char initiala)
	:r(initialr),
	g(initialg),
	b(initialb),
	a(initiala)
{
	
}

Rgba8::Rgba8(unsigned char initialr, unsigned char initialg, unsigned char initialb)
	:r(initialr),
	g(initialg),
	b(initialb),
	a(255)
{

}

Rgba8::Rgba8(Rgba8 const& copyColor)
	: r(copyColor.r)
	, g(copyColor.g)
	, b(copyColor.b)
	, a(copyColor.a)
{

}

// [31:24] ->Alpha(A)
// [23:16]->Red(R)
// [15:8]->Green(G)
// [7:0]->Blue(B)
// 0xFF is 0x000000FF
Rgba8::Rgba8(physx::PxU32 physXColor)
{
	// Extract each component from bitwise calculation
	a = ((physXColor >> 24) & 0xFF);	// Alpha
	r = ((physXColor >> 16) & 0xFF);	// Red
	g = ((physXColor >> 8) & 0xFF);	// Green
	b = (physXColor & 0xFF);			// Blue
}

//Rgba8::Rgba8(Vec4 const& float4)
//{
//	r = DenormalizeByte(float4.x);
//	g = DenormalizeByte(float4.y);
//	b = DenormalizeByte(float4.z);
//	a = DenormalizeByte(float4.w);
//}

bool Rgba8::SetFromText(char const* text)
{
	Strings asStrings = SplitStringOnDelimiter(text, ',');
	int numStrings = (int)asStrings.size();
	if (numStrings == 3)
	{
		r = (unsigned char)atoi(asStrings[0].c_str());
		g = (unsigned char)atoi(asStrings[1].c_str());
		b = (unsigned char)atoi(asStrings[2].c_str());
		a = 255;
		return true;
	}
	if (numStrings == 4)
	{
		r = (unsigned char)atoi(asStrings[0].c_str());
		g = (unsigned char)atoi(asStrings[1].c_str());
		b = (unsigned char)atoi(asStrings[2].c_str());
		a = (unsigned char)atoi(asStrings[3].c_str());
		return true;
	}
	else
	{
		return false;
	}
}

void Rgba8::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = RangeMap(r, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats[1] = RangeMap(g, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats[2] = RangeMap(b, 0.f, 255.f, 0.f, 1.f);
	colorAsFloats[3] = RangeMap(a, 0.f, 255.f, 0.f, 1.f);
}

bool Rgba8::operator!=(Rgba8 const& compare) const
{
	if (r != compare.r || g != compare.g || b != compare.b || a != compare.a)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Rgba8::operator==(Rgba8 const& compare) const
{
	if (r == compare.r && g == compare.g && b == compare.b && a == compare.a)
	{
		return true;
	}
	else
	{
		return false;
	}
}
