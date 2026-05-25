#pragma once
#include "Game/Map.hpp"

class World
{
public:
	World();
	~World();

	void Startup();
	void Update(float deltaSeconds);
	void Render() const;

	Map* m_currentMap;
private:

};