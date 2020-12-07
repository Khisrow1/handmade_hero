/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Undefined                                                     |
    |    Last Modified:  12/7/2020 5:22:02 PM                                          |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

#if !defined(HANDMADE_SIM_REGION_H)
#define HIT_POINT_SUB_COUNT 4

struct move_spec
{
	bool32 UnitMaxAccelVector;
	real32 Speed;
	real32 Drag;
};

struct hit_point
{
	// TODO Bake this down into one variable
	uint8_t Flags;
	uint8_t FilledAmount;
};

enum entity_type
{
	EntityType_Null,

	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monstar,
	EntityType_Sword,
};

struct sim_entity;
union entity_reference
{
	sim_entity *Ptr;
	uint32_t Index;
};

enum sim_entity_flags
{
	EntityFlag_Collides = (1 << 0),
	EntityFlag_Nonspatial = (1 << 1),

	EntityFlag_Simming = (1 << 30),
};

struct sim_entity
{
	// NOTE These are only for the sim region
	uint32_t StorageIndex;
	bool32 Updatable;

	//

	entity_type Type;
	uint32_t Flags;

	v3 P;
	v3 dP;

	real32 DistanceLimit;

	real32 Height, Width;

	uint32_t FacingDirection;
	real32 tBob;

	int32_t dAbsTileZ;

	// TODO Should hit points themselves be entities?
	uint32_t HitPointMax;
	hit_point HitPoint[16];

	entity_reference Sword;
};

struct sim_entity_hash
{
	sim_entity *Ptr;
	uint32_t Index;
};

struct sim_region
{
	world *World;

	world_position Origin;
	rectangle3 Bounds;
	rectangle3 UpdatableBounds;

	uint32_t MaxEntityCount;
	uint32_t EntityCount;
	sim_entity *Entities;

	sim_entity_hash Hash[4096];
};
#define HANDMADE_SIM_REGION_H
#endif
