/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Undefined                                                     |
    |    Last Modified:  12/7/2020 5:06:07 PM                                          |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

// TODO think about the safe margin
#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

#define TILES_PER_CHUNK 16

inline world_position
NullPosition()
{
	world_position Result = {};

	Result.ChunkX = TILE_CHUNK_UNINITIALIZED;

	return Result;
}

inline bool32
IsValid(world_position P)
{
	bool32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
	
	return Result;
}

inline bool32
IsCannonical(real32 ChunkDim, real32 TileRel)
{
	// TODO Fix the floating point math so this can be < only
	real32 Epsilon = 0.0001f;
	bool32 Result = ((TileRel >= -(0.5f*ChunkDim + Epsilon)) &&
					 (TileRel <= (0.5f*ChunkDim  + Epsilon)));
	return Result;
}

inline bool32
IsCannonical(world *World, v3 Offset)
{
	bool32 Result = (IsCannonical(World->ChunkDimInMeters.X, Offset.X) &&
					 IsCannonical(World->ChunkDimInMeters.Y, Offset.Y) &&
					 IsCannonical(World->ChunkDimInMeters.Z, Offset.Z));

	return Result;
}

inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
	Assert(IsCannonical(World, A->Offset_));
	Assert(IsCannonical(World, B->Offset_));
	return ((A->ChunkX == B->ChunkX) &&
			(A->ChunkY == B->ChunkY) &&
			(A->ChunkZ == B->ChunkZ));
}

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

internal void
InitializeWorld(world *World, real32 TileSideInMeters)
{
	World->TileSideInMeters = TileSideInMeters;
	World->ChunkDimInMeters = {(real32)TILES_PER_CHUNK*TileSideInMeters,
								(real32)TILES_PER_CHUNK*TileSideInMeters,
								(real32)TileSideInMeters};
	World->TileDepthInMeters = (real32)TileSideInMeters;
	World->FirstFree = 0;

	for(uint32_t TileChunkIndex = 0;
		TileChunkIndex < ArrayCount(World->ChunkHash);
		++TileChunkIndex)
	{
		World->ChunkHash[TileChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
		World->ChunkHash[TileChunkIndex].FirstBlock.EntityCount = 0;
	}
}

inline void
RecanonicalizeCoord(real32 ChunkDim, int32_t *Tile, real32 *TileRel)
{
	// NOTE World is toroidal topology, you go from one place and come to another place.
	int32_t Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
	*Tile += Offset;
	*TileRel -= Offset * ChunkDim;

	Assert(IsCannonical(ChunkDim, *TileRel));
}

inline world_position
MapIntoChunkSpace(world *World, world_position BasePos, v3 Offset)
{
	world_position Result = BasePos;

	Result.Offset_ += Offset;
	RecanonicalizeCoord(World->ChunkDimInMeters.X, &Result.ChunkX, &Result.Offset_.X);
	RecanonicalizeCoord(World->ChunkDimInMeters.Y, &Result.ChunkY, &Result.Offset_.Y);
	RecanonicalizeCoord(World->ChunkDimInMeters.Z, &Result.ChunkZ, &Result.Offset_.Z);
	
	return Result;
}

inline world_position
ChunkPositionFromTilePosition(world *World, int32_t AbsTileX, int32_t AbsTileY, int32_t AbsTileZ)
{
	world_position BasePos = {};

	v3 Offset = Hadamard(World->ChunkDimInMeters, V3((real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ));
	world_position Result = MapIntoChunkSpace(World, BasePos, Offset);

	Assert(IsCannonical(World, Result.Offset_));

	return Result;
}

inline v3
Subtract(world *World, world_position *A, world_position *B)
{
	v3 dTile = {(real32)A->ChunkX - (real32)B->ChunkX,
				(real32)A->ChunkY - (real32)B->ChunkY,
				(real32)A->ChunkZ - (real32)B->ChunkZ};

	v3 Result = Hadamard(World->ChunkDimInMeters, dTile) + (A->Offset_ - B->Offset_);

	return Result;
}

internal world_position
CenteredChunkPoint(uint32_t ChunkX, uint32_t ChunkY, uint32_t ChunkZ)
{
	world_position Result = {};

	Result.ChunkX = ChunkX;
	Result.ChunkY = ChunkY;
	Result.ChunkZ = ChunkZ;

	return Result;
}

inline void
ChangeEntityLocationRaw(memory_arena *Arena, world *World, uint32_t LowEntityIndex,
						world_position *OldP, world_position *NewP)
{
	Assert(!OldP || IsValid(*OldP));
	Assert(!NewP || IsValid(*NewP));

	if(OldP && NewP && AreInSameChunk(World, OldP, NewP))
	{
		// NOTE Do nothing
	}
	else
	{
		if(OldP)
		{
			// NOTE: Pull the entity out of its old entity block
			world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
			Assert(Chunk);
			if(Chunk)
			{
				bool32 NotFound = true;
				world_entity_block *FirstBlock = &Chunk->FirstBlock;
				for(world_entity_block *Block = FirstBlock;
					Block && NotFound;
					Block = Block->Next)
				{
					for(uint32_t Index = 0;
						(Index < Block->EntityCount) && NotFound;
						++Index)
					{
						if(Block->LowEntityIndex[Index] == LowEntityIndex)
						{
							Assert(FirstBlock->EntityCount > 0);
							Block->LowEntityIndex[Index] =
								FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
							if(FirstBlock->EntityCount == 0)
							{
								if(FirstBlock->Next)
								{
									world_entity_block *NextBlock = FirstBlock->Next;
									*FirstBlock = *NextBlock;

									NextBlock->Next = World->FirstFree;
									World->FirstFree = NextBlock;
								}
							}
							
							NotFound = false;
						}
					}
				}
			}
		}

		if(NewP)
		{
			// NOTE: Insert the entity into its new entity block
			world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
			Assert(Chunk);
			world_entity_block *Block = &Chunk->FirstBlock;
			if(Block->EntityCount == ArrayCount(Block->LowEntityIndex))
			{
				// NOTE: We are out of room, get a new block!
				world_entity_block *OldBlock = World->FirstFree;
				if(OldBlock)
				{
					World->FirstFree = OldBlock->Next;
				}
				else
				{
					OldBlock = PushStruct(Arena, world_entity_block);
				}
				*OldBlock = *Block;
				Block->Next = OldBlock;
				Block->EntityCount = 0;
			}

			Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
			Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
		}
	}
}

internal void
ChangeEntityLocation(memory_arena *Arena, world *World,
					 uint32_t LowEntityIndex, low_entity *EntityLow,
					 world_position NewPInit)
{
	world_position *OldP = 0;
	world_position *NewP = 0;

	if(!IsSet(&EntityLow->Sim, EntityFlag_Nonspatial) && IsValid(EntityLow->P))
	{
		OldP = &EntityLow->P;
	}

	if(IsValid(NewPInit))
	{
		NewP = &NewPInit;
	}
	ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldP, NewP);

	if(NewP)
	{
		EntityLow->P = *NewP;
		ClearFlag(&EntityLow->Sim, EntityFlag_Nonspatial);
	}
	else
	{
		EntityLow->P = NullPosition();
		AddFlag(&EntityLow->Sim, EntityFlag_Nonspatial);
	}
}
