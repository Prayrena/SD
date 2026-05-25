#include "Game/Unit.hpp"
#include "Game/Model.hpp"
#include "Game/Game.hpp"
#include "Game/Map.hpp"
#include "Game/App.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/Timer.hpp"
#include "Engine/VFX/EffectSystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"

extern Game* g_theGame;
extern Renderer* g_theRenderer;
extern Clock* g_theGameClock;

extern Game* g_theGame;
extern SpriteAnimDefinition* g_spriteAnims[NUM_SPRITEANIMS];
extern EffectSystem* g_theEffectSystem;
extern AudioSystem* g_theAudio;
extern SoundID g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::NUM_SOUNDEFFECTS)];

std::vector<UnitDefinition> UnitDefinition::s_unitDefs;

UnitDefinition* UnitDefinition::GetDefBySymbol(char symbol)
{
	for (int i = 0; i < (int)s_unitDefs.size(); ++i)
	{
		if (symbol == s_unitDefs[i].m_unitSymbol)
		{
			return &s_unitDefs[i];
		}
	}

	// if not found, return nullptr
	return nullptr;
}

void UnitDefinition::InitializeUnitDefs()
{
	XmlDocument unitDefXml;
	char const* filePath = "Data/Definitions/UnitDefinitions.xml";
	XmlResult result = unitDefXml.LoadFile(filePath);
	GUARANTEE_OR_DIE(result == tinyxml2::XML_SUCCESS, Stringf("failed to load unit definitions xml file"));

	XmlElement* rootElement = unitDefXml.RootElement();
	GUARANTEE_OR_DIE(rootElement, "actor definition root Element is nullPtr");

	XmlElement* unitDefElement = rootElement->FirstChildElement();

	while (unitDefElement)
	{
		// read map info
		std::string elementName = unitDefElement->Name();
		GUARANTEE_OR_DIE(elementName == "UnitDefinition", Stringf("root cant matchup with the name of \"UnitDefinition\""));
		UnitDefinition* newUnitDef = new UnitDefinition(unitDefElement);// calls the constructor function of TileTypeDefinition
		s_unitDefs.push_back(*newUnitDef);

		unitDefElement = unitDefElement->NextSiblingElement();
	}
}

UnitDefinition::UnitDefinition(XmlElement* unitDefElement)
{
	m_actorName = ParseXmlAttribute(*unitDefElement, "name", "Unit name not defined");
	if (m_actorName == "Unit name not defined")
	{
		ERROR_AND_DIE("Unit name not defined");
	}

	m_unitSymbol = ParseXmlAttribute(*unitDefElement, "symbol", ' ');

	std::string imageFilePath = ParseXmlAttribute(*unitDefElement, "imageFilename", "image file name undefined");
	if (imageFilePath == "image file name undefined")
	{
		ERROR_AND_DIE(Stringf("%s image file name undefined", m_actorName.c_str()));
	}
	m_texture_UI = g_theRenderer->CreateTextureFromFile(imageFilePath.c_str());

	std::string modelFilePath = ParseXmlAttribute(*unitDefElement, "modelFilename", "model file name undefined");
	if (modelFilePath == "model file name undefined")
	{
		ERROR_AND_DIE(Stringf("%s model file name undefined", m_actorName.c_str()));
	}
	m_model = new Model(g_theGame, modelFilePath);

	std::string typeName = ParseXmlAttribute(*unitDefElement, "type", "type undefined");
	if (ToLower(typeName) == "tank")
	{
		m_type = UnitType::TANK;
	}
	else if (ToLower(typeName) == "artillery")
	{
		m_type = UnitType::ARTILLERY;
	}

	m_groundAttackDamage = ParseXmlAttribute(*unitDefElement, "groundAttackDamage", 0);
	m_groundAttackRangeMin = ParseXmlAttribute(*unitDefElement, "groundAttackRangeMin", 0);
	m_groundAttackRangeMax = ParseXmlAttribute(*unitDefElement, "groundAttackRangeMax", 0);

	m_movementRange = ParseXmlAttribute(*unitDefElement, "movementRange", 0);

	m_defense = ParseXmlAttribute(*unitDefElement, "defense", 0);
	m_health = ParseXmlAttribute(*unitDefElement, "health", 0);
}

// todo: is this the correct way to delete definitions ??
void UnitDefinition::ShutDown()
{
	if (m_model)
	{
		delete m_model;
	}
}

UnitDefinition::~UnitDefinition()
{
}

Unit::Unit(UnitDefinition const* unitDef, Map* map, Player* player, int startHexIndex)
	: Entity(g_theGame, map->m_tiles[startHexIndex].m_centerWorldPos)
	, m_unitDef(unitDef)
	, m_map(map)
	, m_player(player)
	, m_currentHexIndex(startHexIndex)
{
	m_unitHealth = m_unitDef->m_health;
}

Unit::~Unit()
{
	GenerateExplosionForDeath();
	if (g_theGame->m_selectedUnit == this)
	{
		g_theGame->m_selectedUnit = nullptr;
		g_theGame->m_currentTurnState = TurnState::NO_SELECTION;
	}
}

void Unit::Startup()
{
	m_startHexIndex = m_currentHexIndex;

	// initial orientation
	if (m_player == g_theGame->m_players[1])
	{
		m_orientation = EulerAngles(180.f, 0.f, 0.f);
	}
	else
	{
		m_orientation = EulerAngles(0.f, 0.f, 0.f);
	}

	m_movingTimer = new Timer(1.f, g_theGameClock);
	m_smokeTimer = new Timer(2.f, g_theGameClock);
	// m_smokeTimer = new Timer(0.9f, g_theGameClock);
	m_smokeTimer->Start();

	m_hitTimer = new Timer(0.4f, g_theGameClock);
}

void Unit::Render() const
{
	// say the scene has a lot of similar model, should I only have one model but render differently
	// or create different model instance

	// set the model color
	Rgba8 modelColor;
	for (int i = 0; i < (int)g_theGame->m_players.size(); ++i)
	{
		if (m_player == g_theGame->m_players[i])
		{
			if (this == g_theGame->m_selectedUnit || (this == m_map->GetUnitCurrentlyOnThisTile(g_theGame->m_selectedHexIndex) && !this->HasFinishedMoveAndAttackThisTurn()))
			{
				modelColor = g_theGame->m_playerSelectedUnitColorMap[m_player];
			}
			else
			{
				modelColor = g_theGame->m_playerUnitColorMap[m_player];
			}
		}
	}

	Model*	model = m_unitDef->m_model;
	model->m_position = m_map->m_tiles[m_currentHexIndex].m_centerWorldPos;

	g_theRenderer->SetBlendMode(BlendMode::OPAQUE);

	// if this is a unit belongs to a player, we need to change its color
	g_theRenderer->SetModelConstants(GetModelMatrix(), modelColor);

	g_theRenderer->BindDiffuseSpecularNormalTextures(g_theRenderer->m_defaultWhiteTexture, g_theRenderer->m_defaultBlackTexture, g_theRenderer->m_defaultWhiteTexture);
	g_theRenderer->BindShader(g_theRenderer->m_PhongShader);
	PhongLightingConstants lightingConstant = *g_theGame->m_phongLighinting;
	lightingConstant.lighingDebug.UseNormalMap = false;
	g_theRenderer->SetPhongLightingConstants(lightingConstant);

	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	model->m_gpuMesh->Render(g_theRenderer);

	if (g_theGame->m_debugMode && !model->m_debugVertexes.empty())
	{
		g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->SetModelConstants(model->GetModelMatrix());

		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexBuffer(model->m_debugVertexBuffer, (int)model->m_debugVertexes.size());
	}

}

void Unit::Update()
{
	if (this == g_theGame->m_selectedUnit && m_moving && g_theGame->m_currentMap->m_unitMovingPathSpline)
	{
		MoveAndRotateUnitAccordingtoMapSpline();
	}
	else if (this == g_theGame->m_selectedUnit && 
				m_currentHexIndex == m_startHexIndex && // has not moved yet
				g_theGame->m_currentTurnState == TurnState::UNIT_SELECTED_MOVE &&
				g_theGame->m_selectedHexIndex != INVALID_HEX_INDEX)
	{
		RotateUnitTowardsTargetPosition(g_theGame->m_currentMap->m_tiles[g_theGame->m_selectedHexIndex].m_centerWorldPos);
	}
	else if (m_isAttacking && m_attackingAtHexIndex != INVALID_HEX_INDEX)
	{
		if (RotateUnitTowardsTargetPosition(g_theGame->m_currentMap->m_tiles[m_attackingAtHexIndex].m_centerWorldPos))
		{
			GenerateEffectForAttacking();
			m_isAttacking = false;
			m_hitTimer->Restart();
		}		
	}

	// testing VFX
	// if (m_smokeTimer->HasPeroidElapsed())
	// {
	// 	// GenerateHitEffectForAttackTarget();
	// 	// GenerateEffectForAttacking();
	// 	// GenerateExplosionForDeath();
	// 	// GenerateExplosionForDeath();
	// 	GenerateHitNumberOnAttackTarget();
	// 	m_smokeTimer->Restart();
	// }
	 
	if (m_hitTimer->HasPeroidElapsed() && !m_hitTimer->IsStopped())
	{
		GenerateHitEffectForAttackTarget();
		GenerateHitNumberOnAttackTarget();
		m_hitTimer->Stop();

		// causing damage
		Unit* targetUnit = g_theGame->m_currentMap->GetUnitCurrentlyOnThisTile(m_attackingAtHexIndex);
		targetUnit->m_unitHealth -= m_damage;

		g_theGame->m_currentMap->CheckAndRemoveTheUnitWhenItDies(targetUnit);
		g_theGame->CheckForWinner();

		// change game status
		if (this == g_theGame->m_selectedUnit)
		{
			g_theGame->m_currentTurnState = TurnState::NO_SELECTION;
			g_theGame->m_selectedUnit = nullptr;
		}		
		
		if (this == g_theGame->m_defensingUnit)
		{
			g_theGame->m_defensingUnit = nullptr;
		}
	}
}

bool Unit::IsMoved() const
{
	return (m_startHexIndex != m_currentHexIndex);
}

bool Unit::HasFinishedMoveAndAttackThisTurn() const
{
	return(m_hasMoved && m_hasAttacked);
}

void Unit::MoveAndRotateUnitAccordingtoMapSpline()
{
	if (m_position == g_theGame->m_currentMap->m_tiles[m_currentHexIndex].m_centerWorldPos)
	{
		m_moving = false;
		g_theGame->m_currentTurnState = TurnState::UNIT_SELECTED_ATTACK;
		g_theGame->m_currentMap->UpdateTilesInAttackRange();
	}
	else
	{
		float elapsedTime = m_movingTimer->GetElapsedTime();
		float pastDist = elapsedTime * m_tankMovingSpeed;

		float splineLength = g_theGame->m_currentMap->m_unitMovingPathSpline->GetApproximateLength();
		if (pastDist > splineLength)	// finish moving align the spline
		{
			m_moving = false;
			g_theGame->m_currentTurnState = TurnState::UNIT_SELECTED_ATTACK;
			g_theGame->m_currentMap->UpdateTilesInAttackRange();
		}
		else
		{
			m_position = g_theGame->m_currentMap->m_unitMovingPathSpline->EvaluateAtApproximateDistance(pastDist);
			AlignUnitWithDirectionVector(g_theGame->m_currentMap->m_unitMovingPathSpline->GetTangentDirectionAtApproximateDistance(pastDist));

			if (m_smokeTimer)
			{
				GenerateSmokeForMoving();
				m_smokeTimer->Restart();
			}
		}
	}
}

void Unit::GenerateEffectForAttacking()
{
	Emitter* fireEmitter = new Emitter(*g_spriteAnims[MUZZLEFIRE], GetMuzzlePosition(), Vec3::GetDirectionForYawPitch(m_orientation.m_yawDegrees, 0.f), 10.f, 6, 0.1f);
	fireEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 6.f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(0.f, 0.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::BRIGHT_ORANGE, Rgba8::YELLOW)->
		SetParticle_Size_StartRange(FloatRange(0.45f, 0.9f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.3f, 0.9f))->
		SetParticle_Rotation_StartRange(FloatRange(-80.f + m_orientation.m_yawDegrees, -100.f + m_orientation.m_yawDegrees))->
		SetParticle_RotationSpeed_StartRange(FloatRange(0.f, 0.f))->
		SetParticle_LifeTime_Range(FloatRange(0.6f, 0.9f))->
		SetParticle_Offset_StartRange(FloatRange(0.f, 0.06f));

	g_theEffectSystem->AddEmitter(fireEmitter);	
	
	Emitter* smokeEmitter = new Emitter(*g_spriteAnims[SMOKE], GetMuzzlePosition(), Vec3::GetDirectionForYawPitch(m_orientation.m_yawDegrees, 0.f), 10.f, 24, 0.1f);
	smokeEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 3.f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 2.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::GRAY_Dark, Rgba8::WHITE)->
		SetParticle_Size_StartRange(FloatRange(0.3f, 0.6f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(1.5f, 3.f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(0.f, 30.f))->
		SetParticle_LifeTime_Range(FloatRange(0.9f, 1.8f))->
		SetParticle_Offset_StartRange(FloatRange(0.f, 0.3f));

	g_theEffectSystem->AddEmitter(smokeEmitter);

	g_theAudio->StartSound(g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::FIRING)], false, 1.0f, 0.f, 1.f, false);
}

void Unit::GenerateHitEffectForAttackTarget()
{
	Vec3 targetPosition = g_theGame->m_currentMap->m_tiles[m_attackingAtHexIndex].m_centerWorldPos;
	Emitter* fireEmitter = new Emitter(*g_spriteAnims[MUZZLEFIRE], targetPosition, Vec3::GetDirectionForYawPitch(m_orientation.m_yawDegrees, 0.f) * -1.f, 10.f, 6, 0.1f);
	fireEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 0.3f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(0.f, 0.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::BRIGHT_ORANGE, Rgba8::YELLOW)->
		SetParticle_Size_StartRange(FloatRange(0.45f, 0.9f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.3f, 0.9f))->
		SetParticle_Rotation_StartRange(FloatRange(80.f + m_orientation.m_yawDegrees, 100.f + m_orientation.m_yawDegrees))->
		SetParticle_RotationSpeed_StartRange(FloatRange(0.f, 0.f))->
		SetParticle_LifeTime_Range(FloatRange(0.3f, 0.75f))->
		SetParticle_Offset_StartRange(FloatRange(-0.06f, 0.06f));

	g_theEffectSystem->AddEmitter(fireEmitter);

	Emitter* smokeEmitter = new Emitter(*g_spriteAnims[SMOKE], targetPosition, Vec3::GetDirectionForYawPitch(m_orientation.m_yawDegrees, 0.f) * -1.f, 10.f, 12, 0.1f);
	smokeEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 1.2f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 2.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::WHITE, Rgba8::WHITE_TRANSPARENT)->
		SetParticle_Size_StartRange(FloatRange(0.6f, 1.2f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.6f, 1.8f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(30.f, 60.f))->
		SetParticle_LifeTime_Range(FloatRange(0.6f, 1.8f))->
		SetParticle_Offset_StartRange(FloatRange(-0.15f, 0.15f));

	g_theEffectSystem->AddEmitter(smokeEmitter);

	g_theAudio->StartSound(g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::HIT)], false, 1.0f, 0.f, 1.f, false);
}

void Unit::GenerateHitNumberOnAttackTarget()
{
	Vec3 pos = g_theGame->m_currentMap->m_tiles[m_attackingAtHexIndex].m_centerWorldPos;
	EventArgs args;
	args.SetValue("damageAmount", Stringf("%i", -m_damage));
	args.SetValue("showDuration", Stringf("%.f", 2.f)); 
	args.SetValue("damagePosition", Stringf("%.2f, %.2f, %.2f", pos.x, pos.y, pos.z + 0.5f));
	FireEvent("generatedamageemitter", args);
}

void Unit::GenerateSmokeForMoving()
{
	Emitter* smokeEmitter = new Emitter(*g_spriteAnims[SMOKE], GetExhaustPosition(), Vec3(0.f, 0.f, 1.f), 1000'000.f, 3, 0.1f);
	smokeEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 3.f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 2.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(0.6f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::GRAY_TRANSPARENT, Rgba8::WHITE_TRANSPARENT)->
		SetParticle_Size_StartRange(FloatRange(0.15f, 0.45f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.1f, 0.3f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(10.f, 30.f))->
		SetParticle_LifeTime_Range(FloatRange(0.6f, 1.2f))->
		SetParticle_Offset_StartRange(FloatRange(-0.15f, 0.15f));

	g_theEffectSystem->AddEmitter(smokeEmitter);
}

void Unit::GenerateExplosionForDeath()
{
	Emitter* explosionEmitter = new Emitter(*g_spriteAnims[EXPLOSION], m_position, Vec3(0.f, 0.f, 1.f), 10.f, 6, 0.1f);
	explosionEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.6f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 1.1f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(0.f, 0.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::WHITE, Rgba8::YELLOW)->
		SetParticle_Size_StartRange(FloatRange(0.45f, 1.2f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.15f, 0.6f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(10.f, 60.f))->
		SetParticle_LifeTime_Range(FloatRange(1.75f, 2.f))->
		SetParticle_Offset_StartRange(FloatRange(-0.2f, 0.2f));

	g_theEffectSystem->AddEmitter(explosionEmitter);

	Emitter* smokeEmitter = new Emitter(*g_spriteAnims[SMOKE], m_position, Vec3(0.f, 0.f, 1.f), 15.f, 18, 3.f);
	smokeEmitter->SetParticle_SizeScale_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_MovingSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 1.2f))->
		SetParticle_RotationSpeedScale_FromStartToEnd(unsortedFloatRange(1.f, 2.f))->
		SetParticle_Alpha_FromStartToEnd(unsortedFloatRange(1.f, 0.f))->
		SetParticle_Color_FromStartToEnd(Rgba8::GRAY_Dark, Rgba8::WHITE)->
		SetParticle_Size_StartRange(FloatRange(0.9f, 1.5f))->
		SetParticle_MovingSpeed_StartRange(FloatRange(0.15f, 0.9f))->
		SetParticle_Rotation_StartRange(FloatRange(0.f, 360.f))->
		SetParticle_RotationSpeed_StartRange(FloatRange(5.f, 30.f))->
		SetParticle_LifeTime_Range(FloatRange(1.5f, 3.f))->
		SetParticle_Offset_StartRange(FloatRange(-0.3f, 0.3f));

	g_theEffectSystem->AddEmitter(smokeEmitter);

	g_theAudio->StartSound(g_soundEffectsID[static_cast<unsigned long long>(SoundEffectID::EXPLOSION)], false, 1.0f, 0.f, 1.f, false);
}

Vec3 Unit::GetExhaustPosition() const
{
	return (m_position + GetModelMatrix().TransformVectorQuantity3D(m_exhaustOffset));
}

Vec3 Unit::GetMuzzlePosition() const
{
	return (g_theGame->m_currentMap->m_tiles[m_currentHexIndex].m_centerWorldPos + GetModelMatrix().TransformVectorQuantity3D(m_muzzleOffset));
}

bool Unit::RotateUnitTowardsTargetPosition(Vec3 const& targetPos)
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	float maxDeltaDegrees = m_tankTurnRate * deltaSeconds;

	// get the orientation player appointed
	Vec3 targetDirection = (targetPos - 
							g_theGame->m_currentMap->m_tiles[m_currentHexIndex].m_centerWorldPos).GetNormalized();
	float yawDegrees = targetDirection.GetAngleAboutZDegrees();
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, yawDegrees, maxDeltaDegrees);

	// if after turning, the tank is aiming at the correct direction, return true: ready for firing
	if (m_orientation.m_yawDegrees == yawDegrees)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Unit::AlignUnitWithDirectionVector(Vec3 const& direction)
{
	float deltaSeconds = g_theGameClock->GetDeltaSeconds();
	float maxDeltaDegrees = m_tankTurnRate * deltaSeconds;

	// get the orientation player appointed
	float yawDegrees = direction.GetAngleAboutZDegrees();
	m_orientation.m_yawDegrees = GetTurnedTowardDegrees(m_orientation.m_yawDegrees, yawDegrees, maxDeltaDegrees);

	// if after turning, the tank is aiming at the correct direction, return true
	if (m_orientation.m_yawDegrees == yawDegrees)
	{
		return true;
	}
	else
	{
		return false;
	}
}
