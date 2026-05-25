#include "Game/EnergyBar.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Game/PlayerShip.hpp"

extern Renderer* g_theRenderer;
extern App* g_theApp;

// the children's constructor function need to initialize its parent class constructor first
EnergyBar::EnergyBar(Game* owner, Vec2 const& startPos)
	:UI(owner, startPos)// constructing the entity before the constructing the bullet
{
	m_energyRemainColor = Rgba8(ENERGY_REMAIN_COLOR_R, ENERGY_REMAIN_COLOR_G, ENERGY_REMAIN_COLOR_B, ENERGY_REMAIN_COLOR_A);
}

EnergyBar::~EnergyBar()
{
}

void EnergyBar::Update(float deltaSeconds)
{
	InitializeLocalVerts();

	UpdateEnergyRemain(deltaSeconds);
}


void EnergyBar::UpdateEnergyRemain(float deltaSeconds)
{
	m_age += deltaSeconds;

	m_energyRemain += ENERGY_GENERATE_RATE * deltaSeconds; // energy is recovering every delta second
	m_energyRemain -= m_energyConsuming * deltaSeconds;

	// energy remain would not above 100 percents
	if (m_energyRemain > 1.f)
	{
		m_energyRemain = 1.f;
	}

	//if (m_energyRemain < 0.f)
	//{
	//	m_energyRemain = 0.05f;
	//	m_game->m_playerShip->m_shieldGenerated = false;
	//	m_game->m_playerShip->m_speedBooster = false;
	//	m_game->m_playerShip->m_fireRateBooster = false;
	//}

	// when energy remain is below 20 percents, the color changes and flashes
	if (m_energyRemain < 0.2f)
	{
		m_energyRemainColor = Rgba8(ENERGY_REMAIN_WARNING_COLOR_R, ENERGY_REMAIN_WARNING_COLOR_G, ENERGY_REMAIN_WARNING_COLOR_B, 255);
		m_energyRemainColor.a = static_cast<unsigned char>(   RangeMap(sinf(m_age * m_colorChangingRate), -1.f, 1.f, 100.f, 255.f)  );
	}

	// when energy is above 20 percent, change back its color
	if ( m_energyRemain > 0.2f )
	{
		m_energyRemainColor = Rgba8(ENERGY_REMAIN_COLOR_R, ENERGY_REMAIN_COLOR_G, ENERGY_REMAIN_COLOR_B, ENERGY_REMAIN_COLOR_A);
	}
}


void EnergyBar::Render() const //all called functions must be const
{
	// is current ship is destroyed, do not render
	if (!m_isEnabled)
	{
		return;
	}

	Vertex_PCU tempWorldVerts[ENERGYBAR_TRI_NUM_VERT];

	int vertIndex = 0;
	int& vi = vertIndex;

	for (vi; vi < ENERGYBAR_TRI_NUM_VERT; ++vi)
	{
		tempWorldVerts[vi] = m_localVerts[vi];
	}

	TransformVertexArrayXY3D(ENERGYBAR_TRI_NUM_VERT, tempWorldVerts, 1.f, m_orientationDegrees, m_position);
	g_theRenderer->DrawVertexArray(ENERGYBAR_TRI_NUM_VERT, tempWorldVerts);
}


// Energy bar contains two part, the background and current usage
void EnergyBar::InitializeLocalVerts()
{
	// background
	m_localVerts[0].m_position = Vec3(UI_ENERGYBAR_POS_X, UI_ENERGYBAR_POS_Y, 0.f);
	m_localVerts[1].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT, 0.f);
	m_localVerts[2].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE + UI_ENERGYBAR_LENGTH, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT, 0.f);

	m_localVerts[3].m_position = Vec3(UI_ENERGYBAR_POS_X, UI_ENERGYBAR_POS_Y, 0.f);
	m_localVerts[4].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE + UI_ENERGYBAR_LENGTH, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT, 0.f);
	m_localVerts[5].m_position = Vec3(UI_ENERGYBAR_POS_X + UI_ENERGYBAR_LENGTH, UI_ENERGYBAR_POS_Y, 0.f);

	// the bar that shows how much energy remain
	m_localVerts[6].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYREMAIN_OFFSET + .5f, UI_ENERGYBAR_POS_Y - UI_ENERGYREMAIN_OFFSET, 0.f);
	m_localVerts[7].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE + UI_ENERGYREMAIN_OFFSET + .5f, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT + UI_ENERGYREMAIN_OFFSET, 0.f);
	m_localVerts[8].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE + UI_ENERGYBAR_LENGTH * m_energyRemain, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT + UI_ENERGYREMAIN_OFFSET, 0.f);

	m_localVerts[9].m_position  = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYREMAIN_OFFSET + .5f, UI_ENERGYBAR_POS_Y - UI_ENERGYREMAIN_OFFSET, 0.f);
	m_localVerts[10].m_position = Vec3(UI_ENERGYBAR_POS_X - UI_ENERGYBAR_SLOPE + UI_ENERGYBAR_LENGTH * m_energyRemain, UI_ENERGYBAR_POS_Y - UI_ENERGYBAR_HEIGHT + UI_ENERGYREMAIN_OFFSET, 0.f);
	m_localVerts[11].m_position = Vec3(UI_ENERGYBAR_POS_X + UI_ENERGYBAR_LENGTH * m_energyRemain - UI_ENERGYREMAIN_OFFSET - UI_ENERGYREMAIN_OFFSET, UI_ENERGYBAR_POS_Y - UI_ENERGYREMAIN_OFFSET, 0.f);

	// change color according to vertex index
	for (int vertIndex = 0; vertIndex < 6; ++vertIndex)
	{
		m_localVerts[vertIndex].m_color = Rgba8(ENERGYBAR_COLOR_R, ENERGYBAR_COLOR_G, ENERGYBAR_COLOR_B, ENERGYBAR_COLOR_A);
	}

	// remain energy bar color 
	for (int vertIndex = 6; vertIndex < 12; ++vertIndex)
	{
		m_localVerts[vertIndex].m_color = m_energyRemainColor;
	}
}

