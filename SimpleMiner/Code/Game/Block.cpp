#include "Game/Block.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/core/StringUtils.hpp"

std::vector<BlockDef> BlockDef::s_BlockDefs;

bool BlockDef::operator!=(BlockDef compare)
{
	return m_name != compare.m_name;
}

bool BlockDef::operator==(BlockDef compare)
{
	return m_name == compare.m_name;
}

extern Renderer* g_theRenderer;

BlockDef::BlockDef(std::string name, bool visible, bool solid, bool opaque, bool emissive,
	uint8_t outdoorLightLevel, uint8_t indoorLightLevel,
	IntVec2 topSpriteCoords, IntVec2 sidesSpriteCoords, IntVec2 bottomSpriteCoords)
	: m_name(name)
	, m_isVisible(visible)
	, m_isSolid(solid)
	, m_isOpaque(opaque)
	, m_isEmissive(emissive)
	, m_outdoorLight(outdoorLightLevel)
	, m_indoorLight(indoorLightLevel)
	, m_topSpriteCoords(topSpriteCoords)
	, m_sideSpriteCoords(sidesSpriteCoords)
	, m_bottomSpriteCoords(bottomSpriteCoords)
{

}

BlockDef::~BlockDef()
{

}

AABB2 BlockDef::GetTileTextureUVsOnSpriteSheet(IntVec2 const& spriteCoords) const
{
	return m_spriteSheet->GetSpriteUVs(spriteCoords);
}

void BlockDef::InitializeBlockDefs()
{
	// BlockDefs		Name			Visible		solid		opaque		emissive		outdoorLight		indoorLight		topSprite			sidesSprite			bottomeSprite
	CreateNewBlockDef("air",			false,		false,		false,		false, 				15,					0,			IntVec2(0, 0),		IntVec2(0, 0),		IntVec2(0, 0));
	CreateNewBlockDef("water",			true,		true,		true,		false, 				0,					0,			IntVec2(32, 44),	IntVec2(32, 44),	IntVec2(32, 44));
	CreateNewBlockDef("ice",			true,		true,		true,		false, 				0,					0,			IntVec2(36, 35),	IntVec2(36, 35),	IntVec2(36, 35));
	CreateNewBlockDef("sand",			true,		true,		true,		false, 				0,					0,			IntVec2(34, 34),	IntVec2(34, 34),	IntVec2(34, 34));
	CreateNewBlockDef("stone",			true,		true,		true,		false, 				0,					0,			IntVec2(32, 32),	IntVec2(32, 32),	IntVec2(32, 32));
	CreateNewBlockDef("cobbleStone",	true,		true,		true,		false, 				0,					0,			IntVec2(63, 44),	IntVec2(63, 44),	IntVec2(63, 44));
	CreateNewBlockDef("grassBrick",		true,		true,		true,		false, 				0,					0,			IntVec2(61, 41),	IntVec2(61, 41),	IntVec2(61, 41));
	CreateNewBlockDef("dirt",			true,		true,		true,		false, 				0,					0,			IntVec2(32, 34),	IntVec2(32, 34),	IntVec2(32, 34));
	CreateNewBlockDef("grass",			true,		true,		true,		false, 				0,					0,			IntVec2(32, 33),	IntVec2(33, 33),	IntVec2(32, 34));
	CreateNewBlockDef("snowyGrass",		true,		true,		true,		false,				0,					0,			IntVec2(36, 35),	IntVec2(33, 35),	IntVec2(32, 34));
	CreateNewBlockDef("coal",			true,		true,		true,		false,				0,					0,			IntVec2(63, 34),	IntVec2(63, 34),	IntVec2(63, 34));
	CreateNewBlockDef("iron",			true,		true,		true,		false,				0,					0,		    IntVec2(63, 35),	IntVec2(63, 35),	IntVec2(63, 35));
	CreateNewBlockDef("gold",			true,		true,		true,		false,				0,					0,			IntVec2(63, 36),	IntVec2(63, 36),	IntVec2(63, 36));
	CreateNewBlockDef("diamond",		true,		true,		true,		false,				0,					0,			IntVec2(63, 37),	IntVec2(63, 37),	IntVec2(63, 37));
	CreateNewBlockDef("glowStone",		true,		true,		true,		true,				0,					15,			IntVec2(46, 34),	IntVec2(46, 34),	IntVec2(46, 34));
	CreateNewBlockDef("oakLog",			true,		true,		true,		false,				0,					0,			IntVec2(38, 33),	IntVec2(36, 33),	IntVec2(38, 33));
	CreateNewBlockDef("oakLeaf",		true,		true,		true,		false,				0,					0,			IntVec2(32, 35),	IntVec2(32, 35),	IntVec2(32, 35));
	CreateNewBlockDef("spruceLog",		true,		true,		true,		false,				0,					0,			IntVec2(38, 33),	IntVec2(35, 33),	IntVec2(38, 33));
	CreateNewBlockDef("spruceLeaf",		true,		true,		true,		false,				0,					0,			IntVec2(34, 35),	IntVec2(34, 35),	IntVec2(34, 35));
	CreateNewBlockDef("cactus",			true,		true,		true,		false,				0,					0,			IntVec2(38, 36),	IntVec2(37, 36),	IntVec2(39, 36));
}

void BlockDef::CreateNewBlockDef(std::string name, bool visible, bool solid, bool opaque, bool emissive,
	uint8_t outdoorLightLevel, uint8_t indoorLightLevel,
	IntVec2 topSpriteCoords, IntVec2 sidesSpriteCoords, IntVec2 bottomSpriteCoords)
{
	BlockDef newBlockDef = *new BlockDef(name, visible, solid, opaque, emissive, outdoorLightLevel, indoorLightLevel, topSpriteCoords, sidesSpriteCoords, bottomSpriteCoords);

	// all block def is using the same sprite sheet
	std::string spriteSheetPath = "Data/Images/BasicSprites_64x64.png";
	Texture* spriteSheetTexture = g_theRenderer->CreateOrGetTextureFromFile(spriteSheetPath.c_str());
	IntVec2 cellCount = IntVec2(64, 64);
	newBlockDef.m_spriteSheet = new SpriteSheet(*spriteSheetTexture, cellCount);

	BlockDef::s_BlockDefs.push_back(newBlockDef);
}

BlockDef BlockDef::GetBlockDefByName(std::string name)
{
	for (int i = 0; i < (int)s_BlockDefs.size(); ++i)
	{
		if (s_BlockDefs[i].m_name == name)
		{
			return s_BlockDefs[i];
		}
	}

	ERROR_RECOVERABLE(Stringf("%s is not defined in the block defs", name.c_str()));
	return s_BlockDefs[0];
}

void Block::SetType(std::string tileTypeName)
{
	for (int i = 0; i < BlockDef::s_BlockDefs.size(); ++i)
	{
		if (BlockDef::s_BlockDefs[i].m_name == tileTypeName)
		{
			m_blockDefIndex = unsigned char(i);
			SetIsOpaque(BlockDef::s_BlockDefs[i].m_isOpaque);
			SetIsEmissive(BlockDef::s_BlockDefs[i].m_isEmissive);
			SetVisibility(BlockDef::s_BlockDefs[i].m_isVisible);
			SetSolidity(BlockDef::s_BlockDefs[i].m_isSolid);
			return; // if find the correct tile definition, stop the for loop
		}
	}

	ERROR_RECOVERABLE(Stringf("%s is not defined in the block defs", tileTypeName.c_str()));
}

void Block::SetType(BlockDef const& def)
{
	for (int i = 0; i < BlockDef::s_BlockDefs.size(); ++i)
	{
		if (BlockDef::s_BlockDefs[i] == def)
		{
			m_blockDefIndex = unsigned char(i);
			SetIsOpaque(def.m_isOpaque);
			SetIsEmissive(def.m_isEmissive);
			SetVisibility(def.m_isVisible);
			SetSolidity(def.m_isSolid);
			return; // if find the correct tile definition, stop the for loop
		}
	}

	ERROR_RECOVERABLE(Stringf("%s is not defined in the block defs", def.m_name.c_str()));
}

void Block::UpdateBlockBitFlagsByBlockDef()
{
	BlockDef def = BlockDef::s_BlockDefs[m_blockDefIndex];
	SetIsOpaque(def.m_isOpaque);
	SetIsEmissive(def.m_isEmissive);
	SetVisibility(def.m_isVisible);
}

BlockDef Block::GetBlockDef() const
{
	return BlockDef::s_BlockDefs[int(m_blockDefIndex)];
}

uint8_t Block::GetOutdoorLighting() const
{
	if (!this)
	{
		return 0;
	}
	else
	{
		return (m_blockLighting & LIGHT_MASK_OUTDOOR) >> LIGHT_BITS_INDOOR;
	}
}

uint8_t Block::GetIndoorLighting() const
{ 
	if (!this)
	{
		return 0;
	}
	else
	{
		return (m_blockLighting & LIGHT_MASK_INDOOR);
	}
}

// newOutdoorLighting have not been bit shifted
void Block::SetOutdoorLightingInfulence(unsigned char newOutdoorLighting)
{
	newOutdoorLighting = newOutdoorLighting << LIGHT_BITS_OUTDOOR;

	// destroy outdoor lighting bits
	m_blockLighting = m_blockLighting & ~LIGHT_MASK_OUTDOOR;

	// then append the outdoor influence
	m_blockLighting = m_blockLighting | newOutdoorLighting;
}

void Block::SetIndoorLightingInfulence(unsigned char newIndoorLighting)
{
	// destroy indoor lighting bits
	m_blockLighting = m_blockLighting & ~LIGHT_MASK_INDOOR;

	// then append the indoor influence
	m_blockLighting = m_blockLighting | newIndoorLighting;
}

void Block::SetIsBlockSky(bool isSky)
{
	// set is sky bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_SKY;

	if (isSky)
	{
		m_blockBitFlags = m_blockBitFlags | (1 << (BLOCK_BIT_IS_SKY - 1));
	}
}

bool Block::IsBlockSky() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_SKY) >> (BLOCK_BIT_IS_SKY - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else if (bitsValue == 1)
	{
		return true;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("wrong calculation, result is %i", bitsValue));
		return false;
	}
}

void Block::SetVisibility(bool visibility)
{
	// set is sky bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_VISIBLE;

	if (visibility)
	{
		m_blockBitFlags = m_blockBitFlags | (1 << (BLOCK_BIT_IS_VISIBLE - 1));
	}
}

bool Block::IsVisible() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_VISIBLE) >> (BLOCK_BIT_IS_VISIBLE - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else if (bitsValue == 1)
	{
		return true;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("wrong calculation, result is %i", bitsValue));
		return false;
	}
}

void Block::SetSolidity(bool isSolid)
{
	// set is sky bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_SOLID;

	if (isSolid)
	{
		m_blockBitFlags = m_blockBitFlags | BLOCK_BIT_MASK_IS_SOLID;
	}
}

bool Block::IsSolid() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_SOLID) >> (BLOCK_BIT_IS_SOLID - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else if (bitsValue == 1)
	{
		return true;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("wrong calculation, result is %i", bitsValue));
		return false;
	}
}

void Block::SetLightDirty(bool isLightDirty)
{
	// set is light dirty bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_LIGHT_DIRTY;

	if (isLightDirty)
	{
		m_blockBitFlags = m_blockBitFlags | (1 << (BLOCK_BIT_IS_LIGHT_DIRTY - 1));
	}
	else
	{
		return;
	}
}

bool Block::IsLightDirty() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_LIGHT_DIRTY) >> (BLOCK_BIT_IS_LIGHT_DIRTY - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else if (bitsValue == 1)
	{
		return true;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("wrong calculation, result is %i", bitsValue));
		return false;
	}
}

void Block::SetIsOpaque(bool isOpaque)
{
	// set is opaque dirty bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_FULL_OPAQUE;

	if (isOpaque)
	{
		m_blockBitFlags = m_blockBitFlags | (1 << (BLOCK_BIT_IS_FULL_OPAQUE - 1));
	}
	else
	{
		return;
	}
}

void Block::SetIsEmissive(bool isEmissive)
{
	// set is emissive dirty bits to 0
	m_blockBitFlags = m_blockBitFlags & ~BLOCK_BIT_MASK_IS_EMISSIVE;

	if (isEmissive)
	{
		m_blockBitFlags = m_blockBitFlags | (1 << (BLOCK_BIT_MASK_IS_EMISSIVE - 1));
	}
	else
	{
		return;
	}
}

bool Block::IsEmissive() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_EMISSIVE) >> (BLOCK_BIT_IS_EMISSIVE - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Block::IsOpaque() const
{
	uint8_t bitsValue = (m_blockBitFlags & BLOCK_BIT_MASK_IS_FULL_OPAQUE) >> (BLOCK_BIT_IS_FULL_OPAQUE - 1);

	if (bitsValue == 0)
	{
		return false;
	}
	else if (bitsValue == 1)
	{
		return true;
	}
	else
	{
		ERROR_RECOVERABLE(Stringf("wrong calculation, result is %i", bitsValue));
		return false;
	}
}

//void Block::SetBlockCoordsAndType(IntVec3 coords, std::string tileTypeName)
//{
//	m_blockCoords = coords;
//	SetType(tileTypeName);
//}

Block::Block()
{

}

std::vector<BlockTemplate> BlockTemplate::s_blockTemplates;

void BlockTemplate::InitializeBlockTemplates()
{
	BlockTemplate::CreateOakBlockTemplate();
	BlockTemplate::CreateSpruceBlockTemplate();
	BlockTemplate::CreateCactusBlockTemplate();
}

BlockTemplate BlockTemplate::GetBlockTemplateByName(std::string templateName)
{
	for (int i = 0; i < (int)s_blockTemplates.size(); ++i)
	{
		if (s_blockTemplates[i].m_templateName == templateName)
		{
			return s_blockTemplates[i];
		}
	}

	ERROR_RECOVERABLE(Stringf("%s is not defined in the block templates", templateName.c_str()));
	return s_blockTemplates[0];
}

// big leaves, max 2 offset, lower than spruce
void BlockTemplate::CreateOakBlockTemplate()
{
	BlockTemplate* oakTree = new BlockTemplate("oak");

	// trunk
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 1))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 2))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 2))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 3))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLog"), IntVec3(0, 0, 7))));

	// 4nd floor leaves
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-2, 2, 4), 0.3f)));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, 2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 2, 4), 0.3f)));

	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-2, 1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, 1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(2, 1, 4))));

	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-2, 0, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 0, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 0, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(2, 0, 4))));

	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-2, -1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, -1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, -1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, -1, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(2, -1, 4))));

	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-2, -2, 4), 0.3f)));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, -2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, -2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, -2, 4))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(2, -2, 4), 0.3f)));

	// 5nd floor leaves
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 1, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, 1, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 1, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 0, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 0, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, -1, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, -1, 5))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, -1, 5))));

	// 6nd floor leaves
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 1, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, 1, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 1, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 0, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 0, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, -1, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, -1, 6))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, -1, 6))));

	// 7nd floor leaves
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 1, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, 1, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 1, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, 0, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, 0, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(-1, -1, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(0, -1, 7))));
	oakTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("oakLeaf"), IntVec3(1, -1, 7))));

	BlockTemplate::s_blockTemplates.push_back(*oakTree);
}

void BlockTemplate::CreateSpruceBlockTemplate()
{
	BlockTemplate* spruceTree = new BlockTemplate("spruce");
	float possibility = 0.6f;

	// trunk
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 1))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 2))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 2))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 3))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 4))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 5))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 6))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 7))));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLog"), IntVec3(0, 0, 8))));

	// 4nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 4), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1, 4)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1, 4), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 4))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0, 4))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1, 4), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 4))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 4), possibility)));

	// 5nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 5), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1, 5)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1, 5), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 5))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0, 5))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1, 5), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 5))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 5), possibility)));

	// 6nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 6), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1,	6)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1,	6), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 6))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0,	6))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1,6), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 6))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 6), possibility)));

	// 7nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1,	7)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1,	7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 7))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0,	7))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1,7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 7))));// 
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 7), possibility)));	
	
	// 7nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1,	7)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1,	7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 7))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0,	7))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1,7), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 7))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 7), possibility)));	
	
	// 8nd floor leaves
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 1, 8), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, 1,	8)))); //
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 1,	8), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, 0, 8))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, 0,	8))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(-1, -1,8), possibility)));
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(0, -1, 8))));//
	spruceTree->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("spruceLeaf"), IntVec3(1, -1, 8), possibility)));

	BlockTemplate::s_blockTemplates.push_back(*spruceTree);
}

void BlockTemplate::CreateCactusBlockTemplate()
{
	BlockTemplate* cactus = new BlockTemplate("cactus");
	float possibility = 0.6f;

	// trunk
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 1))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 2))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 2))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 3))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 4))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 5))));
	cactus->m_entries.push_back(*(new BlockTemplateEntry(BlockDef::GetBlockDefByName("cactus"), IntVec3(0, 0, 6), possibility)));

	BlockTemplate::s_blockTemplates.push_back(*cactus);
}
