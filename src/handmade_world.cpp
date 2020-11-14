// TODO think about the safe margin
#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_chunk *
GetWorldChunk(world *World, int32_t ChunkX, int32_t ChunkY, int32_t ChunkZ,
			 memory_arena *Arena = 0)
{
	Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);

	uint32_t HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
	uint32_t HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
	Assert(HashSlot < ArrayCount(World->ChunkHash));

	world_chunk *Chunk = World->ChunkHash + HashSlot;
	do
	{
		if((ChunkX == Chunk->ChunkX) &&
		   (ChunkY == Chunk->ChunkY) && 
		   (ChunkZ == Chunk->ChunkZ))
		{
			break;
		}

		if(Arena && (Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
		{
			Chunk->NextInHash = PushStruct(Arena, world_chunk);
			Chunk = Chunk->NextInHash;
			Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
		}
		if(Arena && (Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED))
		{
			Chunk->ChunkX = ChunkX;
			Chunk->ChunkY = ChunkY;
			Chunk->ChunkZ = ChunkZ;

			Chunk->NextInHash = 0;
			break;
		}

		Chunk = Chunk->NextInHash;
	} while(Chunk);

	return Chunk;
}
#if 0
inline world_chunk_position GetChunkPositionFor(world *World, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	world_chunk_position Result;

	Result.ChunkX = AbsTileX >> World->ChunkShift;
	Result.ChunkY = AbsTileY >> World->ChunkShift;
	Result.ChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;

	return Result;
}
#endif
internal void
InitializeWorld(world *World, real32 TileSideInMeters)
{
	World->ChunkShift = 4;
	World->ChunkMask = (1 << World->ChunkShift) - 1;
	World->ChunkDim = (1 << World->ChunkShift);

	World->TileSideInMeters = TileSideInMeters;

	for(uint32_t TileChunkIndex = 0;
		TileChunkIndex < ArrayCount(World->ChunkHash);
		++TileChunkIndex)
	{
		World->ChunkHash[TileChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
	}
}

// TODO Should they be in some other file, geometry?
inline void
RecanonicalizeCoord(world *World, int32_t *Tile, real32 *TileRel)
{
	// NOTE World is toroidal topology, you go from one place and come to another place.
	int32_t Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);
	*Tile += Offset;
	*TileRel -= Offset * World->TileSideInMeters;

	// TODO Fix the floating point math so this can be < only
	Assert(*TileRel >= -0.5f*World->TileSideInMeters);
	Assert(*TileRel <= 0.5f*World->TileSideInMeters);
}

inline world_position
MapIntoTileSpace(world *World, world_position BasePos, v2 Offset)
{
	world_position Result = BasePos;

	Result.Offset_ += Offset;
	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);
	
	return Result;
}

inline bool32
AreOnSameTile(world_position *A, world_position *B)
{
	return ((A->AbsTileX == B->AbsTileX) &&
			(A->AbsTileY == B->AbsTileY) &&
			(A->AbsTileZ == B->AbsTileZ));
}

inline world_difference
Subtract(world *World, world_position *A, world_position *B)
{
	world_difference Result;

	v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
				  (real32)A->AbsTileY - (real32)B->AbsTileY};
	real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

	Result.dXY = World->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);

	Result.dZ = World->TileSideInMeters*dTileZ;

	return Result;
}

internal world_position
CenteredTilePoint(uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	world_position Result = {};

	Result.AbsTileX = AbsTileX;
	Result.AbsTileY = AbsTileY;
	Result.AbsTileZ = AbsTileZ;

	return Result;
}
