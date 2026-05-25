#pragma once
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/core/XmlUtils.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include <vector>

struct BlockDef
{
public:
	// BlockDef(XmlElement const& tileDefElement);
	BlockDef() = default;
	BlockDef(std::string name, bool visible, bool solid, bool opaque, bool emissive, 
		uint8_t outdoorLightLevel, uint8_t indoorLightLevel,
		IntVec2 topSpriteCoords, IntVec2 sidesSpriteCoords, IntVec2 bottomSpriteCoords);
	~BlockDef();

	std::string		m_name;
	bool			m_isVisible = true;
	bool			m_isSolid = false;
	bool			m_isOpaque = false;
	bool			m_isEmissive = false;

	Rgba8			m_tint = Rgba8::WHITE;
	uint8_t			m_outdoorLight = 0;
	uint8_t			m_indoorLight = 0;

	// IntVec2(999, 999) means is not designated
	IntVec2 m_topSpriteCoords = IntVec2(999, 999);
	IntVec2 m_sideSpriteCoords = IntVec2(999, 999);
	IntVec2 m_bottomSpriteCoords = IntVec2(999, 999);

	SpriteSheet* m_spriteSheet = nullptr;

	AABB2   GetTileTextureUVsOnSpriteSheet(IntVec2 const& spriteCoords) const;

	static void InitializeBlockDefs(); // call defineTileType to define each tile type definition
	static void CreateNewBlockDef(std::string name, bool visible, bool solid, bool opaque, bool emissive,
		uint8_t outdoorLightLevel, uint8_t indoorLightLevel, 
		IntVec2 topSpriteCoords, IntVec2 sidesSpriteCoords, IntVec2 bottomSpriteCoords);
	static BlockDef GetBlockDefByName(std::string name);
	static std::vector<BlockDef> s_BlockDefs;

	bool	operator==(BlockDef compare);
	bool	operator!=(BlockDef compare);
};

struct Block
{
public:
	Block();
	~Block() = default;

	// void SetChunkIndex(unsigned int index);
	// unsigned int GetChunkIndex() const;
	void SetType(std::string tileTypeName);
	void SetType(BlockDef const& def);
	void UpdateBlockBitFlagsByBlockDef();
	BlockDef GetBlockDef() const;

	// lighting
	uint8_t GetOutdoorLighting() const;
	uint8_t GetIndoorLighting() const;
	void SetOutdoorLightingInfulence(unsigned char newOutdoorLighting);
	void SetIndoorLightingInfulence(unsigned char newIndoorLighting);

	void SetIsBlockSky(bool isSky);
	bool IsBlockSky() const;

	void SetVisibility(bool visibility);
	bool IsVisible() const;

	void SetSolidity(bool isSolid);
	bool IsSolid() const;

	void SetLightDirty(bool isLightDirty);
	bool IsLightDirty() const;

	void SetIsOpaque(bool isOpaque);
	bool IsOpaque() const;

	void SetIsEmissive(bool isEmissive);
	bool IsEmissive() const;

	unsigned char m_blockDefIndex = 0;
	uint8_t m_blockLighting = 0; // low 4 bits of indoor lighting, high 4 bits of outdoor lighting, 0100 | 1101
	uint8_t m_blockBitFlags = 0; // uses each bit to store the block def bool info for fast access

private:
	// BlockDef const* m_blockDef = nullptr; // that is a interge which is too large and we don't need it
};

struct BlockTemplateEntry
{
	BlockTemplateEntry(BlockDef def, IntVec3 coords, float possibility = 1.f)
		: blockDef(def)
		, chanceToSpawn(possibility)
		, offset(coords)
	{}
	float		chanceToSpawn = 1.f;
	BlockDef	blockDef;
	IntVec3		offset;
};

struct BlockTemplate
{
	BlockTemplate(std::string name)
		: m_templateName(name)
	{}
	std::string m_templateName;
	std::vector<BlockTemplateEntry> m_entries;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	static void InitializeBlockTemplates();
	static BlockTemplate GetBlockTemplateByName(std::string templateName);
	static void CreateOakBlockTemplate();
	static void CreateSpruceBlockTemplate();
	static void CreateCactusBlockTemplate();
	static std::vector<BlockTemplate> s_blockTemplates;
};