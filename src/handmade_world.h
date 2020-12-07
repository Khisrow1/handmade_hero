/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Undefined                                                     |
    |    Last Modified:  12/7/2020 5:04:44 PM                                          |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

struct world_position
{
	int32_t ChunkY;
	int32_t ChunkX;
	int32_t ChunkZ;

	// NOTE This is Tile-relative X and Y
	v3 Offset_;
};

struct world_entity_block
{
	uint32_t EntityCount;
	uint32_t LowEntityIndex[16];
	world_entity_block *Next;
};

struct world_chunk
{
	int32_t ChunkX;
	int32_t ChunkY;
	int32_t ChunkZ;

	// TODO Profile this and determine if a pointer would be better here!
	world_entity_block FirstBlock;

	world_chunk *NextInHash;
};

struct world
{
	real32 TileSideInMeters;
	real32 TileDepthInMeters;
	v3 ChunkDimInMeters;

	world_entity_block *FirstFree;

	world_chunk ChunkHash[4096];
};
