#pragma once
#include "Engine/core/Vertex_PCU.hpp"
#include "Engine/core/Vertex_PCUTBN.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/core/Clock.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Math/Splines.hpp"
#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Entity.hpp"
#include <string>
#include <map>

class Game;
class Player;

enum class UnitType
{
	TANK,
	ARTILLERY,
	NUM_TYPE
};

struct UnitDefinition
{
	UnitDefinition() = default;
	~UnitDefinition();
	UnitDefinition(XmlElement* unitDefElement);

	void			ShutDown();

	std::string		m_actorName;
	char			m_unitSymbol = ' ';
	Texture*		m_texture_UI = nullptr;

	UnitType		m_type = UnitType::NUM_TYPE;

	int				m_groundAttackDamage = 0;
	int				m_groundAttackRangeMin = 0;
	int				m_groundAttackRangeMax = 0;

	int				m_movementRange = 0;

	int				m_defense = 0;
	int				m_health = 0;

	Model*			m_model = nullptr;

	static UnitDefinition* GetDefBySymbol(char symbol);

	static void InitializeUnitDefs();
	static std::vector<UnitDefinition> s_unitDefs;
};

class Unit: public Entity
{
public:
	Unit(UnitDefinition const* unitDef, Map* map, Player* player, int startHexIndex);
	~Unit();
	virtual void Startup() override;
	virtual void Render() const override;
	virtual void Update() override;

	Map*					m_map = nullptr;
	UnitDefinition const*	m_unitDef = nullptr;
	Player*					m_player = nullptr;

	bool					IsMoved() const;
	bool					m_hasMoved = false;
	bool					m_hasAttacked = false;
	bool					HasFinishedMoveAndAttackThisTurn() const;	// move/stay is done, hold fire/attack is also done

	void					MoveAndRotateUnitAccordingtoMapSpline();
	int						m_startHexIndex = INVALID_HEX_INDEX; // the current hex index that the unit is on
	int						m_currentHexIndex = INVALID_HEX_INDEX; // the current hex index that the unit is on
	bool					m_moving = false;
	Timer*					m_movingTimer = nullptr;

	void					GenerateEffectForAttacking();
	bool					m_isAttacking = false;
	int						m_attackingAtHexIndex = 90;

	int						m_unitHealth = 0;

	void					GenerateSmokeForMoving();
	void					GenerateExplosionForDeath();
	Vec3					m_exhaustOffset = Vec3(-0.6f, 0.f, 0.3f);
	Vec3					GetExhaustPosition() const;
	Timer*					m_smokeTimer = nullptr;

	void					GenerateHitEffectForAttackTarget();
	void					GenerateHitNumberOnAttackTarget();
	int						m_damage = 0;
	Timer*					m_hitTimer = nullptr;

	Vec3					m_muzzleOffset = Vec3(0.6f, 0.f, 0.3f);
	Vec3					GetMuzzlePosition() const;

	bool					RotateUnitTowardsTargetPosition(Vec3 const& targetPos);
	bool					AlignUnitWithDirectionVector(Vec3 const& direction);
	float					m_tankTurnRate = 180.f;
	float					m_tankMovingSpeed = 3.f;
};