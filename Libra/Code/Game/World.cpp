#include "Game/World.hpp"

World::World()
{

}

World::~World()
{

}

void World::Startup()
{

}

void World::Update(float deltaSeconds)
{
	if (m_currentMap)
	{
		m_currentMap->Update(deltaSeconds);
	}
}

void World::Render() const
{
	if (m_currentMap)
	{
		m_currentMap->Render();
	}
}
