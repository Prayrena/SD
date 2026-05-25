#include "Game/UI.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

extern RandomNumberGenerator* g_rng;

UI::UI(Game* owner, Vec2 const& startPos)
{
	m_game = owner;
	m_position = startPos;
}

UI::~UI()
{
}

Vec2 UI::GetForwardNormal() const
{
	return Vec2(0.f, 0.f);
}


