#include "Game/BlockIterator.hpp"
#include "Engine/core/EngineCommon.hpp"

BlockIter::BlockIter(Chunk* chunk, int blockIndex)
	: m_chunk(chunk)
	, m_blockIndex(blockIndex)
{

}

Block* BlockIter::GetBlock() const
{
	if (!m_chunk)
	{
		return nullptr;
	}

	if (m_blockIndex < 0 || m_blockIndex >= CHUNK_BLOCKS_TOTAL)
	{
		return nullptr;
	}
	else
	{
		return &m_chunk->m_blocks[m_blockIndex];
	}
}

BlockIter BlockIter::GetEastNeighbor() const
{
	if (!m_chunk)
	{
		return  BlockIter(nullptr, -1);
	}

	int localX = m_blockIndex & CHUNK_MASK_X;
	if (localX == CHUNK_MASK_X)
	{
		return BlockIter(m_chunk->m_eastNeighbor, m_blockIndex & ~CHUNK_MASK_X); // turn off all the X bits
	}
	else
	{
		return BlockIter(m_chunk, m_blockIndex + 1);
	}
}

BlockIter BlockIter::GetWestNeighbor() const
{
	if (!m_chunk)
	{
		return BlockIter(nullptr, -1);
	}

	int localX = m_blockIndex & CHUNK_MASK_X;
	if (localX == 0)
	{
		return BlockIter(m_chunk->m_westNeighbor, m_blockIndex | CHUNK_MASK_X); // turn on all the X bits
	}
	else
	{
		return BlockIter(m_chunk, m_blockIndex - 1);
	}
}

BlockIter BlockIter::GetNorthNeighbor() const
{
	if (!m_chunk)
	{
		return BlockIter(nullptr, -1);
	}

	int localY = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	if (localY == (CHUNK_SIZE_Y - 1))
	{
		return BlockIter(m_chunk->m_northNeighbor, m_blockIndex & ~CHUNK_MASK_Y); // turn off all the Y bits
	}
	else
	{
		return BlockIter(m_chunk, m_blockIndex + CHUNK_SIZE_X);
	}
}

BlockIter BlockIter::GetSouthNeighbor() const
{
	if (!m_chunk)
	{
		BlockIter(nullptr, -1);
	}

	int localY = (m_blockIndex & CHUNK_MASK_Y) >> CHUNK_BITS_X;
	if (localY == 0)
	{
		return BlockIter(m_chunk->m_southNeighbor, m_blockIndex | CHUNK_MASK_Y); // turn on all the Y bits
	}
	else
	{

		return BlockIter(m_chunk, m_blockIndex - CHUNK_SIZE_X);
	}
}

BlockIter BlockIter::GetUpperLevelNeighbor() const
{
	if (!m_chunk)
	{
		return BlockIter(nullptr, -1);
	}
	else
	{
		int blockIndex = m_blockIndex + CHUNK_BLOCKS_PER_LAYER; // top of the chunk
		if (blockIndex >= CHUNK_BLOCKS_TOTAL)
		{
			return BlockIter(nullptr, -1);
		}
		else
		{
			return BlockIter(m_chunk, blockIndex);
		}
	}
}

BlockIter BlockIter::GetLowerLevelNeighbor() const
{
	if (!m_chunk)
	{
		return BlockIter(nullptr, -1);
	}
	else
	{
		int blockIndex = m_blockIndex - CHUNK_BLOCKS_PER_LAYER; // top of the chunk
		if (blockIndex <= 0)
		{
			return BlockIter(nullptr, -1);
		}
		else
		{
			return BlockIter(m_chunk, blockIndex);
		}
	}
}

BlockIter BlockIter::GetOffsetBlockIter(IntVec3 offset)
{
	BlockIter newIter;
	// move at x direction
	if (offset.x >= 0)
	{
		for (int i = 0; i < offset.x; ++i)
		{
			newIter = GetEastNeighbor();
		}
	}
	else
	{
		int eastStep = offset.x * (-1);
		for (int i = 0; i < eastStep; ++i)
		{
			newIter = GetWestNeighbor();
		}
	}
	// move at y direction
	if (offset.y >= 0)
	{
		for (int i = 0; i < offset.y; ++i)
		{
			newIter = GetNorthNeighbor();
		}
	}
	else
	{
		int southStep = offset.y * (-1);
		for (int i = 0; i < southStep; ++i)
		{
			newIter = GetSouthNeighbor();
		}
	}	
	// move at z direction
	if (offset.z >= 0)
	{
		for (int i = 0; i < offset.z; ++i)
		{
			newIter = GetUpperLevelNeighbor();
		}
	}
	else
	{
		int downwardsStep = offset.z * (-1);
		for (int i = 0; i < downwardsStep; ++i)
		{
			newIter = GetLowerLevelNeighbor();
		}
	}

	return newIter;
}

bool BlockIter::IsNonOpaqueAndNonSky()
{
	if (GetBlock())
	{
		if (IsNonOpaque() && !GetBlock()->IsBlockSky())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool BlockIter::IsNonOpaque()
{
	if (GetBlock())
	{
		if (!GetBlock()->IsOpaque())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool BlockIter::IsNonLightDirty()
{
	if (GetBlock())
	{
		if (!GetBlock()->IsLightDirty())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool BlockIter::IsEmissive()
{
	if (GetBlock())
	{
		if (GetBlock()->IsEmissive())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
