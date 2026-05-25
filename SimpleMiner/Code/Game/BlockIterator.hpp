#pragma once
#include "Game/Chunk.hpp"

class BlockIter
{
public:
	BlockIter() {}
	explicit BlockIter(Chunk* chunk, int blockIndex);

	// neighbors and myself
	Block* GetBlock() const;
	BlockIter GetEastNeighbor() const;
	BlockIter GetWestNeighbor() const;
	BlockIter GetNorthNeighbor() const;
	BlockIter GetSouthNeighbor() const;
	BlockIter GetUpperLevelNeighbor() const; // always in the same chunk
	BlockIter GetLowerLevelNeighbor() const; // always in the same chunk

	BlockIter GetOffsetBlockIter(IntVec3 offset);

	// Block lighting info
	bool IsNonOpaqueAndNonSky();
	bool IsNonOpaque();
	bool IsNonLightDirty();
	bool IsEmissive();

	Chunk* m_chunk = nullptr;
	int m_blockIndex = -1; // irrelevant if the chunk ptr does not exist
};